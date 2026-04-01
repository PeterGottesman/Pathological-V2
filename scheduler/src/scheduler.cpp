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
                return !pending_jobs_.empty() || !running_;
            });
        }
        if (!running_) break;
        assignJobs();
    }
    std::cout << "Scheduler stopped." << std::endl;
}

void Scheduler::stop() {
    running_ = false;
    job_available_.notify_all();
}

void Scheduler::assignJobs() {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    std::cout << "Assigning jobs. Queue size: " << pending_jobs_.size()  << " | Connected workers: " << workers_.size() << std::endl;
    while (!pending_jobs_.empty()) {
        Worker* worker = findIdleWorker();
        if (worker == nullptr) {
            std::cout << "No idle workers available, jobs in queue: " << pending_jobs_.size() << std::endl;
            break;
        }
        RenderRequest job = pending_jobs_.front();
        pending_jobs_.pop();

        // This connects to the renderer's gRPC server to send job
        RenderWorkerClient client(
            grpc::CreateChannel(
                worker->ip + ":" + std::to_string(worker->port),
                grpc::InsecureChannelCredentials()
            )
        );

        int job_id = client.RenderJob(job);
        if (job_id != -1) {
            worker->status = WorkerStatus::BUSY;
            std::cout << "Job assigned to worker: " << worker->id << " with job ID: " << job_id << std::endl;
        } else {
            std::cout << "Job dispatch failed, requeueing." << std::endl;
            pending_jobs_.push(job);
            worker->status = WorkerStatus::OFFLINE;
            break;
        }
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