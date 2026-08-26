#include "render_jobs.hpp"

#include <iostream>
#include <memory>
#include <utility>

#include "job.hpp"

using render_server::Status;

void RenderJobs::AddJob(const std::string& uuid, std::shared_ptr<Job> job) { this->jobMap[uuid] = std::move(job); }

void RenderJobs::UpdateJobStatus(const std::string& uuid, render_server::Status status) {
    this->jobMap[uuid]->setStatus(status);
}

Status RenderJobs::FetchJobStatus(const std::string& uuid) { return this->jobMap[uuid]->getStatus(); }

void RenderJobs::PrintJobs() {
    for (auto const& job : this->jobMap) {
        std::cout << job.first << "\n";
    }
}
