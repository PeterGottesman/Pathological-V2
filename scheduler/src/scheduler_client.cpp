#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

#include "../build/protos/scheduler_server.grpc.pb.h"
#include "protos/scheduler_server.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using scheduler_server::WorkerConnection;
using scheduler_server::WorkerIP;
using scheduler_server::ServerResponse;

class SchedulerClient {
 public:
  SchedulerClient(std::shared_ptr<Channel> channel)
      : stub_(WorkerConnection::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  int EstablishConnection(const std::string& workerIP) {
    // Data we are sending and getting back
    WorkerIP request;
    request.set_worker_ip(workerIP);
    ServerResponse response;
    ClientContext context;

    // The actual RPC.
    Status status = stub_->EstablishConnection(&context, request, &response);

    // Act upon its status.
    if (status.ok()) {
      return 0;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return response.error();
    }
  }

 private:
  std::unique_ptr<WorkerConnection::Stub> stub_;
};

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  std::string target_str = "localhost:50051";
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  SchedulerClient client(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  std::string connectionAddr = "127.0.0.1:50051";
  int response = client.EstablishConnection(connectionAddr);
  std::cout << "Greeter received: " << response << std::endl;
  return 0;
}
