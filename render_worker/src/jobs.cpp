#include "jobs.hpp"

using render_server::Status;

void RenderJobs::AddJob(std::string uuid, render_server::Status status){
    this->jobMap[uuid] = status;
}

void RenderJobs::UpdateJobStatus(std::string uuid, render_server::Status status){
    this->jobMap[uuid] = status;
}

Status RenderJobs::FetchJobStatus(std::string uuid){
    return this->jobMap[uuid];
}
