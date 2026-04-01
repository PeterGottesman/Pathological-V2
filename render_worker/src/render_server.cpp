#include "render_server.hpp"
#include "render_worker.hpp"
#include "scheduler_client.hpp"
#include <iostream>
#include <memory>
#include <string>

Status RenderServer::RenderJob(ServerContext *context, const RenderJobRequest *request,
    RenderJobResponse *response) {
    std::cout << "Render Recieved" << std::endl;
    response->set_job_identifier(rand() % 10000);
    generateScene(request->width(), request->height(), request->samples(),
        request->scene_location(), request->output_name(), request->time());
    this->client.JobCompleted(response->job_identifier());
    return Status::OK;
}

Status RenderServer::RenderStatus(ServerContext *context, const RenderStatusRequest *request, RenderStatusResponse *response) {
    // Sample implementation
    response->set_status(render_server::COMPLETED);
    return Status::OK;
}

std::unique_ptr<Server> BuildServer(uint16_t port, SchedulerClient& client, std::string worker_id) {
  std::string server_address = absl::StrFormat("0.0.0.0:%d", port);
  RenderServer service(client, worker_id);

  grpc::EnableDefaultHealthCheckService(true);
  ServerBuilder builder;

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  // Register "service" as the instance through which we'll communicate with
  // clients.
  builder.RegisterService(&service);

  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  return server;
}
