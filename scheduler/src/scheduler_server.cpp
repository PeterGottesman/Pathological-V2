#include <grpcpp/grpcpp.h>
#include "../build/protos/scheduler_server.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using scheduler_server::WorkerConnection;
using scheduler_server::WorkerIP;
using scheduler_server::ServerResponse;

#include <iostream>
#include <string>


class SchedulerServer final : public WorkerConnection::Service {
    Status EstablishConnection(ServerContext *context, const WorkerIP *request, ServerResponse *response) {
        std::cout << request->worker_ip() << std::endl;
        response->set_error(9999);
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
