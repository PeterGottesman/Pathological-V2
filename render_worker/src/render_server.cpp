#include <grpcpp/grpcpp.h>
#include "../build/protos/render_server.grpc.pb.h"
#include "protos/render_server.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using render_server::RenderWorker;
using render_server::RenderJobRequest;
using render_server::RenderJobResponse;
using render_server::RenderStatusRequest;
using render_server::RenderStatusResponse;

#include <iostream>
#include <string>

class SchedulerServer final : public RenderWorker::Service {
    Status RenderJob(ServerContext *context, const RenderJobRequest *request, RenderJobResponse *response) {
        // Sample implementation
        std::cout << "Scene location: " << request->scene_location() << std::endl;
        std::cout << "Frames: " << request->frames() << std::endl;
        std::cout << "Seconds: " << request->seconds() << std::endl;
        response->set_job_identifier(rand() % 10000);
        return Status::OK;
    }

    Status RenderStatus(ServerContext *context, const RenderStatusRequest *request, RenderStatusResponse *response) {
        // Sample implementation
        response->set_status(render_server::DONE);
        return Status::OK;
    }
};

void RunServer(uint16_t port) {
  std::string server_address = absl::StrFormat("0.0.0.0:%d", port);
  SchedulerServer service;

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

  // Wait for the server to shutdown
  server->Wait();
}

int main(int argc, char *argv[]) {
    RunServer(50051);
    return 0;
}
