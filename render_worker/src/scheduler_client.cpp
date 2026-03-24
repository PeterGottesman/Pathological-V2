#include "scheduler_client.hpp"

#include <iostream>
#include <memory>
#include <string>

#include "../build/protos/scheduler_server.grpc.pb.h"
#include "protos/scheduler_server.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using scheduler_server::WorkerConnection;
using scheduler_server::RegistrationStatus;
using scheduler_server::WorkerInfo;
using scheduler_server::WorkerID;
using scheduler_server::ServerResponse;
using scheduler_server::JobCompletedRequest;

class SchedulerClient {
public:
  SchedulerClient(std::shared_ptr<Channel> channel, const std::string& worker_id, const std::string& worker_ip, uint32_t port) : stub_(WorkerConnection::NewStub(channel)),
    worker_id_(worker_id),
    worker_ip_(worker_ip),
    port_(port) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.

  void EstablishConnection() {
    
    // This populates a request from the server with the worker's info.

    WorkerInfo request;
    request.set_worker_ip(worker_ip_);
    request.set_worker_id(worker_id_);
    request.set_port(port_);

    ServerResponse response;
    ClientContext context;

    // The actual RPC.

    Status status = stub_->EstablishConnection(&context, request, &response);

    // If the RPC call succeeds, an output message is displayed detailing the worker's current status with the scheduler.

    if (status.ok()) {
      if (response.status() == RegistrationStatus::OK) {
        worker_id_ = response.assigned_id();
          std::cout << "Registered with scheduler, assigned ID: " << worker_id_ << std::endl;
        } else if (response.status() == RegistrationStatus::RECONNECTED) {
          std::cout << "Reconnected to scheduler with ID: " << worker_id_ << std::endl;
        } else if (response.status() == RegistrationStatus::REJECTED) {
          std::cout << "Registration rejected by scheduler." << std::endl;
        }
    } else {
      std::cerr << "EstablishConnection failed: " << status.error_message() << std::endl;
    }
  }

  void Heartbeat() {
    WorkerID request;
    request.set_worker_id(worker_id_);

    ServerResponse response;
    ClientContext context;

    Status status = stub_->Heartbeat(&context, request, &response);
    if (status.ok()) {
      if (response.status() == RegistrationStatus::UNKNOWN) {
        std::cout << "Scheduler does not recognise this worker, " << "re-registration required." << std::endl;
      }
    } else {
      std::cerr << "Heartbeat failed: " << status.error_message() << std::endl;
    }
  }

  void JobCompleted(int32_t job_id) {
    JobCompletedRequest request;
    request.set_worker_id(worker_id_);
    request.set_job_id(job_id);

    ServerResponse response;
    ClientContext context;

    Status status = stub_->JobCompleted(&context, request, &response);
    if (status.ok()) {
      std::cout << "Job " << job_id << " completion acknowledged." << std::endl;
    } else {
      std::cerr << "JobCompleted failed: " << status.error_message() << std::endl;
      }
  }

  void Disconnect() {
    WorkerID request;
    request.set_worker_id(worker_id_);

    ServerResponse response;
    ClientContext context;

    Status status = stub_->Disconnect(&context, request, &response);
    if (status.ok()) {
      std::cout << "Disconnected from scheduler." << std::endl;
    } else {
      std::cerr << "Disconnect failed: " << status.error_message() << std::endl;
    }
  }

private:
  std::unique_ptr<WorkerConnection::Stub> stub_;
  std::string worker_id_;
  std::string worker_ip_;
  uint32_t port_;
};

int main(int argc, char** argv) {

  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created.

  std::string scheduler_address = "localhost:50051";
  std::string worker_id = "worker-1"; // **** TEST VARIABLE; EVENTUALLY THE SCHEDULER WILL AUTO ASSIGN IDs ****
  std::string worker_ip = "127.0.0.1";
  uint32_t port = 50051;

  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).

  SchedulerClient client(
    grpc::CreateChannel(scheduler_address, grpc::InsecureChannelCredentials()), worker_id, worker_ip, port);

  // **** Client testing ****
  client.EstablishConnection();
  client.Heartbeat();
  client.JobCompleted(1);
  client.Disconnect();

  return 0;
}
