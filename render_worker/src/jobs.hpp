#pragma once
#include "protos/render_server.pb.h"

using render_server::Status;

class RenderJobs {
public:
    void AddJob(std::string uuid, Status status);
    void UpdateJobStatus(std::string uuid, Status status);
    Status FetchJobStatus(std::string uuid);

private:
    std::map<std::string, Status> jobMap;
};
