#include "scheduler.hpp"

void Scheduler::addJob(const RenderRequest& job) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        pending_jobs_.push(job);
        std::cout << "Job added to queue. Queue size: " << pending_jobs_.size() << std::endl;
    }
    job_available_.notify_one();
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
        std::cout << "Worker registered: " << id << " at " << ip << std::endl;
    }
    job_available_.notify_one();
}

void Scheduler::reconnectWorker(const std::string& id) {
    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        Worker* worker = findWorkerByID(id);
        if (worker != nullptr) {
            worker->status = WorkerStatus::IDLE;
            std::cout << "Worker reconnected: " << id << std::endl;
        }
    }
    job_available_.notify_one();
}

void Scheduler::markWorkerOffline(const std::string& id) {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    Worker* worker = findWorkerByID(id);
    if (worker != nullptr) {
        worker->status = WorkerStatus::OFFLINE;
        std::cout << "Worker is offline: " << id << std::endl;
    }
}

void Scheduler::markWorkerIdle(const std::string& id) {
    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        Worker* worker = findWorkerByID(id);
        if (worker != nullptr) {
            worker->status = WorkerStatus::IDLE;
            std::cout << "Worker is idle: " << id << std::endl;
        }
    }
    job_available_.notify_one();
}

void Scheduler::run() {
    running_ = true;
    std::cout << "Scheduler running." << std::endl;
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
                std::cout << "Wait check — jobs: " << pending_jobs_.size() << " | idle worker: " << has_idle_worker << " | running: " << running_ << std::endl;
                return (!pending_jobs_.empty() && has_idle_worker) || !running_;
            });
        }
        if (!running_) break;
        assigning_ = true;
        assignJobs();
        assigning_ = false;
        job_available_.notify_all();
    }
    std::cout << "Scheduler stopped." << std::endl;
}

void Scheduler::stop() {
    running_ = false;
    job_available_.notify_all();
}

void Scheduler::assignJobs() {
    std::unique_lock<std::mutex> workers_lock(workers_mutex_);
    std::unique_lock<std::mutex> queue_lock(queue_mutex_);
    std::cout << "Assigning jobs. Queue size: " << pending_jobs_.size()  << " | Connected workers: " << workers_.size() << std::endl;
    while (!pending_jobs_.empty()) {
        Worker* worker = findIdleWorker();
        if (worker == nullptr) {
            std::cout << "No idle workers available, jobs in queue: " << pending_jobs_.size() << std::endl;
            break;
        }
        RenderRequest job = pending_jobs_.front();
        pending_jobs_.pop();

        std::string worker_id = worker->id;
        std::string worker_address = worker->ip + ":" + std::to_string(worker->port);
        worker->status = WorkerStatus::BUSY;

        workers_lock.unlock();
        queue_lock.unlock();

        // This connects to the renderer's gRPC server to send job
        std::cout << "Dispatching to: " << worker_address << std::endl;
        RenderWorkerClient client(
            grpc::CreateChannel(worker_address, grpc::InsecureChannelCredentials())
        );

        std::string job_id = client.RenderJob(job);
        if (job_id == "ERROR") {
            queue_lock.lock();
            pending_jobs_.push(job);
            queue_lock.unlock();

            workers_lock.lock();
            Worker* worker_ = findWorkerByID(worker_id);
            if (worker_) worker_->status = WorkerStatus::OFFLINE;
            workers_lock.unlock();
            break;
        }
        workers_lock.lock();
        queue_lock.lock();
    }
}

Worker* Scheduler::findIdleWorker() {
    Worker* least_busy = nullptr;
    for (auto& worker : workers_) {
        if (worker.status == WorkerStatus::IDLE) {
            least_busy = &worker;
            std::cout << "Worker found: " << worker.id << " | Status: " << static_cast<int>(worker.status) << std::endl;
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
