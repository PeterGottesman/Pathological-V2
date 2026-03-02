#include "scheduler_client.hpp"

#include <iostream>
#include <memory>
#include <string>

SchedulerClient::SchedulerClient(std::shared_ptr<Channel> channel): stub_(WorkerConnection::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
int SchedulerClient::EstablishConnection(const std::string& workerIP) {
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
