#pragma once

#include <grpcpp/grpcpp.h>

#include <atomic>
#include <boost/uuid/random_generator.hpp>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "renderRequest.hpp"
#include "render_client.hpp"
#include "worker.hpp"

// A single frame of an animation, dispatchable to any idle worker
// independently of the rest of the animation's frames.
struct FrameJob {
    std::shared_ptr<RenderRequest> renderRequest;
    uint32_t frameIndex;
    float time;
};

// What a dispatched worker job id refers back to: which render, and which
// frame of it.
struct FrameJobContext {
    std::string renderId;
    uint32_t frameIndex;
};

// Tracks how many of a render's frames have landed and their output links,
// so the parent RenderRequest can be flipped to COMPLETED only once every
// frame is in.
struct RenderProgress {
    uint32_t totalFrames = 0;
    uint32_t framesCompleted = 0;
    std::vector<std::optional<std::string>> frameLinks;
};

class Scheduler {
public:
    static Scheduler& getInstance() {
        static Scheduler instance;
        return instance;
    }

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    Worker* findWorkerByID(const std::string& id);

    // Splits the render into one FrameJob per animation frame and queues them.
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
    std::queue<FrameJob> pending_jobs_;
    std::mutex queue_mutex_;
    std::condition_variable job_available_;

    // Worker list
    std::vector<Worker> workers_;
    std::mutex workers_mutex_;

    // Tracks scheduler-assigned job IDs back to the render/frame they belong
    // to. Entries are added before dispatch so a JobCompleted callback can
    // never arrive before its entry exists.
    std::unordered_map<std::string, FrameJobContext> worker_job_to_render_id_;
    std::mutex job_map_mutex_;

    // Per-render frame completion tracking, keyed by render id.
    std::unordered_map<std::string, RenderProgress> render_progress_;
    std::mutex progress_mutex_;

    // Mints scheduler-assigned job ids.
    boost::uuids::random_generator job_id_gen_;
    std::mutex job_id_gen_mutex_;

    // Dispatch thread tracking
    std::vector<std::thread> dispatch_threads_;
    std::mutex threads_mutex_;

    // Used to prevent multiple threads from doing shenanigans
    std::atomic<bool> running_{false};
    std::atomic<bool> assigning_{false};

    void assignJobs();
    void joinThreads();
    Worker* findIdleWorker();
    std::string generateJobId();
};
