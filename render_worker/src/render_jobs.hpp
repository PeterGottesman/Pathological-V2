#pragma once
#include <memory>

#include "job.hpp"
#include "protos/render_server.pb.h"

// Holds all of the jobs that a renderer
// has processed
class RenderJobs {
public:
    void AddJob(const std::string& uuid, std::shared_ptr<Job> job);
    void UpdateJobStatus(const std::string& uuid, render_server::Status status);
    render_server::Status FetchJobStatus(const std::string& uuid);
    void PrintJobs();

private:
    std::unordered_map<std::string, std::shared_ptr<Job>> jobMap;
};
