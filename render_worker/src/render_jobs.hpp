#pragma once
#include "job.hpp"
#include "protos/render_server.pb.h"
#include <memory>

class RenderJobs {
public:
    void AddJob(std::string uuid, std::shared_ptr<Job> job);
    void UpdateJobStatus(std::string uuid, render_server::Status status);
    render_server::Status FetchJobStatus(std::string uuid);
    void PrintJobs();

private:
    std::unordered_map<std::string, std::shared_ptr<Job>> jobMap;
};
