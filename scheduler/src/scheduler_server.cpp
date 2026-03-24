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
public:
SchedulerServer(const std::string& file_uri, const std::string& argument)
        : file_uri_(file_uri), argument_(argument) {}
    Status EstablishConnection(ServerContext *context, const WorkerIP *request, ServerResponse *response) {
        std::cout << "Worker connected: " << request->worker_ip() << std::endl;
        response->set_file_uri(file_uri_);
        response->set_argument(argument_);
        response->set_error(0);
        return Status::OK;
    }

private:
    std::string file_uri_;
    std::string argument_;
};

void RunServer(uint16_t port, const std::string& file_uri, const std::string& argument) {
  std::string server_address = absl::StrFormat("0.0.0.0:%d", port);
  SchedulerServer service(file_uri, argument);

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
    if (argc < 3) {
        std::cerr << "Usage: ./server <file_uri> <argument>" << std::endl;
        return 1;
    }
    RunServer(50051, argv[1], argv[2]);
    return 0;
}
