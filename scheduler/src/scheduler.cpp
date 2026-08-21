#include "scheduler.hpp"

#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "renderStatus.hpp"
#include "render_history.hpp"
#include "s3_manager.hpp"

namespace {
constexpr const char* kBucketName = "pathological-capstone-s3-bucket";
constexpr const char* kBucketRegion = "us-east-2";

std::optional<std::string> buildDownloadLink(const std::string& key) {
    static S3Manager s3Manager({
        .bucketName = kBucketName,
        .region = kBucketRegion,
        .profileName = "default",
        .presignedUrlTimeout = 1200,
    });

    auto presigned = s3Manager.createLink(key);
    if (!presigned.empty()) {
        return presigned;
    }

    return std::string("s3://") + kBucketName + "/" + key;
}
}  // namespace

void Scheduler::addJob(const std::shared_ptr<RenderRequest>& job) {
    const int totalFrames = job->getAnimationRuntimeInFrames();
    const int fps = job->getFramesPerSecond();
    if (totalFrames <= 0) {
        std::cout << "Render request has no frames to schedule, skipping." << "\n";
        return;
    }

    const std::string renderId = boost::uuids::to_string(job->getId());
    {
        std::lock_guard<std::mutex> lock(progress_mutex_);
        RenderProgress progress;
        progress.totalFrames = static_cast<uint32_t>(totalFrames);
        progress.frameLinks.resize(totalFrames);
        render_progress_[renderId] = std::move(progress);
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        for (int i = 0; i < totalFrames; ++i) {
            const float time = fps > 0 ? static_cast<float>(i) / static_cast<float>(fps) : static_cast<float>(i);
            pending_jobs_.push(FrameJob{job, static_cast<uint32_t>(i), time});
        }
        std::cout << "Queued " << totalFrames << " frame(s) for render " << renderId
                  << ". Queue size: " << pending_jobs_.size() << "\n";
    }
    job_available_.notify_all();
}

void Scheduler::registerWorker(const std::string& id, const std::string& ip, uint32_t port) {
    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        Worker worker;
        worker.id = id;
        worker.ip = ip;
        worker.port = port;
        worker.status = WorkerStatus::IDLE;
        workers_.push_back(worker);
        std::cout << "Worker registered: " << id << " at " << ip << "\n";
    }
    job_available_.notify_one();
}

void Scheduler::reconnectWorker(const std::string& id) {
    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        Worker* worker = findWorkerByID(id);
        if (worker != nullptr) {
            worker->status = WorkerStatus::IDLE;
            std::cout << "Worker reconnected: " << id << "\n";
        }
    }
    job_available_.notify_one();
}

void Scheduler::markWorkerOffline(const std::string& id) {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    Worker* worker = findWorkerByID(id);
    if (worker != nullptr) {
        worker->status = WorkerStatus::OFFLINE;
        std::cout << "Worker is offline: " << id << "\n";
    }
}

void Scheduler::markWorkerIdle(const std::string& id) {
    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        Worker* worker = findWorkerByID(id);
        if (worker != nullptr) {
            worker->status = WorkerStatus::IDLE;
            std::cout << "Worker is idle: " << id << "\n";
        }
    }
    job_available_.notify_one();
}

bool Scheduler::markRenderCompleted(const std::string& workerJobId) {
    FrameJobContext ctx;
    {
        std::lock_guard<std::mutex> lock(job_map_mutex_);
        auto it = worker_job_to_render_id_.find(workerJobId);
        if (it == worker_job_to_render_id_.end()) {
            return false;
        }
        ctx = it->second;
        worker_job_to_render_id_.erase(it);
    }

    boost::uuids::string_generator stringGen;
    const boost::uuids::uuid uuid = stringGen(ctx.renderId);
    auto render = RenderHistory::getInstance().getRenderRequest(uuid);
    if (!render) {
        return false;
    }

    // Frames are named by index, not by float-formatted time, so uploads are
    // deterministic and the collection can be reassembled in order.
    const std::string key = render->getOutputFileName() + "_" + std::to_string(ctx.frameIndex);
    const auto link = buildDownloadLink(key);

    bool allFramesDone = false;
    uint32_t framesCompleted = 0;
    uint32_t totalFrames = 0;
    std::vector<std::string> orderedLinks;
    {
        std::lock_guard<std::mutex> lock(progress_mutex_);
        auto it = render_progress_.find(ctx.renderId);
        if (it == render_progress_.end()) {
            // Already finalized (e.g. a duplicate completion notification).
            return false;
        }
        RenderProgress& progress = it->second;
        if (ctx.frameIndex < progress.frameLinks.size() && !progress.frameLinks[ctx.frameIndex]) {
            progress.frameLinks[ctx.frameIndex] = link;
            progress.framesCompleted++;
        }
        framesCompleted = progress.framesCompleted;
        totalFrames = progress.totalFrames;
        allFramesDone = framesCompleted >= totalFrames;
        if (allFramesDone) {
            orderedLinks.reserve(progress.frameLinks.size());
            for (auto& frameLink : progress.frameLinks) {
                orderedLinks.push_back(frameLink.value_or(""));
            }
            render_progress_.erase(it);
        }
    }

    render->setFramesCompleted(framesCompleted);

    if (allFramesDone) {
        render->setDownloadLinks(orderedLinks);
        if (!orderedLinks.empty()) {
            render->setDownloadLink(orderedLinks.front());
        }
        render->setStatus(RenderStatus::COMPLETED);
        std::cout << "Render " << ctx.renderId << " completed with " << totalFrames << " frame(s)." << "\n";
    }

    return true;
}

void Scheduler::run() {
    running_ = true;
    std::cout << "Scheduler running." << "\n";
    while (running_) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            job_available_.wait(lock, [this]() {
                bool has_idle_worker = false;
                {
                    std::lock_guard<std::mutex> wlock(workers_mutex_);
                    for (auto& w : workers_) {
                        if (w.status == WorkerStatus::IDLE) {
                            has_idle_worker = true;
                            break;
                        }
                    }
                }
                std::cout << "Wait check — jobs: " << pending_jobs_.size() << " | idle worker: " << has_idle_worker
                          << " | running: " << running_ << "\n";
                return (!pending_jobs_.empty() && has_idle_worker) || !running_;
            });
        }
        if (!running_) {
            break;
        }
        assigning_ = true;
        assignJobs();
        assigning_ = false;
        job_available_.notify_all();
    }
    std::cout << "Scheduler stopped." << "\n";
}

void Scheduler::stop() {
    running_ = false;
    job_available_.notify_all();
    joinThreads();
}

void Scheduler::joinThreads() {
    std::lock_guard<std::mutex> lock(threads_mutex_);
    for (auto& t : dispatch_threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    dispatch_threads_.clear();
}

void Scheduler::assignJobs() {
    std::unique_lock<std::mutex> workers_lock(workers_mutex_);
    std::unique_lock<std::mutex> queue_lock(queue_mutex_);
    std::cout << "Assigning jobs. Queue size: " << pending_jobs_.size() << " | Connected workers: " << workers_.size()
              << "\n";
    while (!pending_jobs_.empty()) {
        Worker* worker = findIdleWorker();
        if (worker == nullptr) {
            std::cout << "No idle workers available, jobs in queue: " << pending_jobs_.size() << "\n";
            break;
        }
        FrameJob frameJob = pending_jobs_.front();
        pending_jobs_.pop();

        std::string worker_id = worker->id;
        std::string worker_address = worker->ip + ":" + std::to_string(worker->port);
        std::string render_id = boost::uuids::to_string(frameJob.renderRequest->getId());
        std::string job_id = generateJobId();
        worker->status = WorkerStatus::BUSY;
        RenderHistory::getInstance().updateStatus(frameJob.renderRequest->getId(), RenderStatus::IN_PROGRESS);

        // Registered before dispatch (not after the RPC returns) because the
        // worker's RenderJob handler is synchronous: it calls back with
        // JobCompleted before its own RPC response reaches us. Registering
        // here guarantees the mapping exists by the time that callback
        // arrives instead of racing it.
        {
            std::lock_guard<std::mutex> mapLock(job_map_mutex_);
            worker_job_to_render_id_[job_id] = FrameJobContext{render_id, frameJob.frameIndex};
        }

        // Spin up a thread per dispatch so gRPC calls don't block the loop
        std::lock_guard<std::mutex> tlock(threads_mutex_);
        dispatch_threads_.emplace_back([this, worker_id, worker_address, frameJob, job_id]() {
            std::cout << "Dispatching frame " << frameJob.frameIndex << " to: " << worker_address << "\n";
            RenderWorkerClient client(grpc::CreateChannel(worker_address, grpc::InsecureChannelCredentials()));

            const bool accepted = client.RenderJob(frameJob.renderRequest, job_id, frameJob.frameIndex, frameJob.time);
            if (!accepted) {
                // The worker never got the frame, so no JobCompleted will
                // ever arrive for it — undo the registration and retry the
                // frame elsewhere.
                {
                    std::lock_guard<std::mutex> mapLock(job_map_mutex_);
                    worker_job_to_render_id_.erase(job_id);
                }
                {
                    std::lock_guard<std::mutex> qlock(queue_mutex_);
                    pending_jobs_.push(frameJob);
                }
                {
                    std::lock_guard<std::mutex> wlock(workers_mutex_);
                    Worker* w = findWorkerByID(worker_id);
                    if (w) {
                        w->status = WorkerStatus::OFFLINE;
                    }
                }
                job_available_.notify_all();
            }
            // On success, completion (and flipping the worker back to IDLE)
            // is driven entirely by the worker's JobCompleted RPC.
        });
    }
}

std::string Scheduler::generateJobId() {
    std::lock_guard<std::mutex> lock(job_id_gen_mutex_);
    return boost::uuids::to_string(job_id_gen_());
}

Worker* Scheduler::findIdleWorker() {
    Worker* least_busy = nullptr;
    for (auto& worker : workers_) {
        if (worker.status == WorkerStatus::IDLE) {
            least_busy = &worker;
            std::cout << "Worker found: " << worker.id << " | Status: " << static_cast<int>(worker.status) << "\n";
            break;
        }
    }
    return least_busy;
}

Worker* Scheduler::findWorkerByID(const std::string& id) {
    for (auto& worker : workers_) {
        if (worker.id == id) {
            return &worker;
        }
    }
    return nullptr;
}
