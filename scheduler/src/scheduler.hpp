#pragma once

#include <grpcpp/grpcpp.h>

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "renderRequest.hpp"
#include "render_client.hpp"
#include "worker.hpp"

class Scheduler {
public:
    static Scheduler& getInstance() {
        static Scheduler instance;
        return instance;
    }

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    Worker* findWorkerByID(const std::string& id);

    // Adds job to queue
    void addJob(const std::shared_ptr<RenderRequest>& job);

    // Worker list management
    void registerWorker(const std::string& id, const std::string& ip, uint32_t port);
    void reconnectWorker(const std::string& id);
    void markWorkerOffline(const std::string& id);
    void markWorkerIdle(const std::string& id);
    bool markRenderCompleted(const std::string& workerJobId);

    void run();
    void stop();

private:
    // Constructor and destructor
    Scheduler() : running_(false) {}
    ~Scheduler() { stop(); }

    // Job queue
    std::queue<std::shared_ptr<RenderRequest>> pending_jobs_;
    std::mutex queue_mutex_;
    std::condition_variable job_available_;

    // Worker list
    std::vector<Worker> workers_;
    std::mutex workers_mutex_;

    // Tracks worker generated job IDs to scheduler render IDs.
    std::unordered_map<std::string, std::string> worker_job_to_render_id_;
    std::mutex job_map_mutex_;

    // Dispatch thread tracking
    std::vector<std::thread> dispatch_threads_;
    std::mutex threads_mutex_;

    // Used to prevent multiple threads from doing shenanigans
    std::atomic<bool> running_{false};
    std::atomic<bool> assigning_{false};

    void assignJobs();
    void joinThreads();
    Worker* findIdleWorker();
};
