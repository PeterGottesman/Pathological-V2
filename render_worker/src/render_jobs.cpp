#include "render_jobs.hpp"
#include "job.hpp"
#include <iostream>
#include <memory>

using render_server::Status;

void RenderJobs::AddJob(std::string uuid, std::shared_ptr<Job> job){
    this->jobMap[uuid] = job;
}

void RenderJobs::UpdateJobStatus(std::string uuid, render_server::Status status){
    this->jobMap[uuid]->setStatus(status);
}

Status RenderJobs::FetchJobStatus(std::string uuid){
    return this->jobMap[uuid]->getStatus();
}

void RenderJobs::PrintJobs(){
    for(auto const& job : this->jobMap){
        std::cout << job.first << std::endl;
    }
}
