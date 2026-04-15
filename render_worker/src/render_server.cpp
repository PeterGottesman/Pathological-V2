#include "render_server.hpp"
#include "render_worker.hpp"
#include "s3_manager.hpp"
#include "scheduler_client.hpp"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>

S3Config config {
    .bucketName = "pathological-capstone-s3-bucket",
    .region = "us-east-2",  // can't use other servers in same region. region must match.
    .profileName = "default",
};

Status RenderServer::RenderJob(ServerContext *context, const RenderJobRequest *request,
    RenderJobResponse *response) {
    std::cout << "Render Recieved" << std::endl;

    std::shared_ptr<Job> job = std::make_shared<Job>(request->width(), request->height(), request->samples(),
        request->scene_location(), request->output_name(), request->time(), render_server::Status::IN_PROGRESS);
    std::string job_id = boost::uuids::to_string(random_gen());

    response->set_job_identifier(job_id);
    this->jobs.AddJob(job_id, job);

    generateScene(job->getWidth(), job->getHeight(), job->getSamples(),
        job->getGLTF(), job->getOutput(), job->getTime(), 512, false);

    std::stringstream stream;
    std::string string_time;
    stream << job->getTime();
    stream >> string_time;
    std::string job_name = job->getOutput() + "_" + string_time;
    std::cout << job_name << std::endl;

    this->manager.putObject(job->getOutput(), job_name, false);
    this->jobs.UpdateJobStatus(job_id, render_server::Status::COMPLETED);
    this->client.JobCompleted(job_id);
    return Status::OK;
}

Status RenderServer::RenderStatus(ServerContext *context, const RenderStatusRequest *request, RenderStatusResponse *response) {
    response->set_status(this->jobs.FetchJobStatus(request->job_identifier()));
    return Status::OK;
}

std::unique_ptr<Server> BuildServer(uint16_t port, SchedulerClient& client, std::string worker_id, random_generator rand) {
  std::string server_address = absl::StrFormat("0.0.0.0:%d", port);

  grpc::EnableDefaultHealthCheckService(true);
  ServerBuilder builder;

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  // Register "service" as the instance through which we'll communicate with
  // clients.
  builder.RegisterService(new RenderServer(client, worker_id, rand));

  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  return server;
}
