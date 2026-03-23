#pragma once

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>
#include <iostream>

#include <grpcpp/grpcpp.h>

#include "worker.hpp"
#include "renderRequest.hpp"
#include "render_client.hpp"

class Scheduler {
public:
    Scheduler();
    ~Scheduler();

	Worker* findWorkerByID(const std::string& id);

    // Adds job to queue
    void addJob(const RenderRequest& job);

    // Worker list management
    void registerWorker(const std::string& id, const std::string& ip, uint32_t port);
    void reconnectWorker(const std::string& id);
    void markWorkerOffline(const std::string& id);
    void markWorkerIdle(const std::string& id);

    void run();
    void stop();

private:
    // Job queue
    std::queue<RenderRequest> pending_jobs_;
    std::mutex queue_mutex_;
    std::condition_variable job_available_;

    // Worker list
    std::vector<Worker> workers_;
    std::mutex workers_mutex_;

    // Used to prevent multiple threads from doing shenanigans
    std::atomic<bool> running_{false};

    void assignJobs();
    Worker* findIdleWorker();
};