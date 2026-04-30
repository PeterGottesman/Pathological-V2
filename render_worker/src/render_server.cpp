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
#include <filesystem>

// Used for s3 uploads. Will need to change bucket
// name if using different bucket. Note the default
// profile name
S3Config config {
    .bucketName = "pathological-capstone-s3-bucket",
    .region = "us-east-2",  // can't use other servers in same region. region must match.
    .profileName = "default",
};

// Renders job recieved from scheduler and uploads
// it to s3 bucket
Status RenderServer::RenderJob(ServerContext *context, const RenderJobRequest *request,
    RenderJobResponse *response) {
    std::cout << "Render Recieved" << std::endl;

    // Makes shared_ptr to job instance and creates UUID
    std::shared_ptr<Job> job = std::make_shared<Job>(request->width(), request->height(), request->samples(),
        request->scene_location(), request->output_name(), request->time(), render_server::Status::IN_PROGRESS);
    std::string job_id = boost::uuids::to_string(random_gen());

    // Sends UUID to scheduler for tracking and
    // adds job to instance of RenderJobs class
    response->set_job_identifier(job_id);
    this->jobs.AddJob(job_id, job);

    // Generates scene
    generateScene(job->getWidth(), job->getHeight(), job->getSamples(),
        job->getGLTF(), job->getOutput(), job->getTime(), 512, false);

    // Generates name for s3 upload
    // currently {output}_{time}
    std::stringstream stream;
    std::string string_time;
    stream << job->getTime();
    stream >> string_time;
    std::string job_name = job->getOutput() + "_" + string_time;
    std::cout << job_name << std::endl;

    // Adds render to s3 bucket and removes render from
    // filesystem that host render worker. IF SAME NAME
    // IS IN BUCKET IT WILL OVERWRITE IT. Refer to
    // s3_manager.cpp
    this->manager.putObject(job->getOutput(), job_name, true);
    if(std::filesystem::remove(job->getOutput())){
        std::cout << "Removed: " << job->getOutput() << " locally." << std::endl;
    }else{
        std::cout << "Could not find render to delete" << std::endl;
    }

    // Updates internal RenderJobs instance to note that the job
    // has been completed and alerts scheduler that job has been completed
    this->jobs.UpdateJobStatus(job_id, render_server::Status::COMPLETED);
    this->client.JobCompleted(job_id);
    return Status::OK;
}

// Returns status of specific job to a scheduler
Status RenderServer::RenderStatus(ServerContext *context, const RenderStatusRequest *request, RenderStatusResponse *response) {
    response->set_status(this->jobs.FetchJobStatus(request->job_identifier()));
    return Status::OK;
}

// Mainly a default function for creating a gRPC server
// refer to examples/cpp/helloworld/greeter_server.cc
// at https://github.com/grpc/grpc
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
