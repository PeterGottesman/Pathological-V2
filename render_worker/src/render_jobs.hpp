#pragma once
#include "protos/render_server.pb.h"

class RenderJobs {
public:
    void AddJob(std::string uuid, render_server::Status status);
    void UpdateJobStatus(std::string uuid, render_server::Status status);
    render_server::Status FetchJobStatus(std::string uuid);

private:
    std::unordered_map<std::string, render_server::Status> jobMap;
};
