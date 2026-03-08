#include <grpcpp/grpcpp.h>
#include "../build/protos/scheduler_server.grpc.pb.h"
#include "../build/protos/scheduler_server.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using scheduler_server::WorkerConnection;
using scheduler_server::RegistrationStatus;
using scheduler_server::WorkerStatus; // **** Used internally ****
using scheduler_server::WorkerInfo;
using scheduler_server::WorkerID;
using scheduler_server::ServerResponse;
using scheduler_server::JobCompletedRequest;

#include <iostream>
#include <string>
#include <vector>
#include <mutex>

// The 'Worker' struct is used for the scheduler to store state information about a render worker.
// The scheduler uses this to refer back to them later, like if it needs to update a specific worker's status.

// **** THIS WILL PROBABLY BE MOVED TO A DIFFERENT SCHEDULER FILE EVENTUALLY ****

struct Worker {
    std::string id;
    std::string ip;
    uint32_t port;
    WorkerStatus status;
};

class SchedulerServer final : public WorkerConnection::Service {
public:
    Status EstablishConnection(ServerContext *context, const WorkerInfo *request, ServerResponse *response) override { // **** 'override' is used to match this function with the virtual method in the parent class made by gRPC. ****
        std::lock_guard<std::mutex> lock(workers_mutex_);
        std::cout << "Worker connected: " << request->worker_ip() << std::endl;
        Worker* existing = findWorkerByID(request->worker_id());
        
        // If a worker connects and is already part of the server's registration, its status is set to 'IDLE'.
        
        if (existing != nullptr) {
            existing->status = WorkerStatus::IDLE;
            response->set_status(RegistrationStatus::RECONNECTED);
            response->set_assigned_id(existing->id);

        // If a worker connects and isn't part of the server's registration, a new entry will be made for it on the list.
        
        } else {
            Worker worker;
            worker.ip = request->worker_ip();
            worker.port = request->port();
            worker.id = request->worker_id();
            worker.status = WorkerStatus::IDLE;
            workers_.push_back(worker);
            response->set_status(RegistrationStatus::OK);
            response->set_assigned_id(worker.id);
        }
        return Status::OK;
    }

    Status Heartbeat(ServerContext* context, const WorkerID* request, ServerResponse* response) override {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        Worker* worker = findWorkerByID(request->worker_id());

        // If a worker can be recognized by the scheduler through a pointer, then the connection is still active.

        if (worker != nullptr) {
            response->set_status(RegistrationStatus::OK);

        // If a worker cannot be recognized, it must reconnect.

        } else {
            response->set_status(RegistrationStatus::UNKNOWN);
        }
        return Status::OK;
    }

    Status JobCompleted(ServerContext* context, const JobCompletedRequest* request, ServerResponse* response) override {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        Worker* worker = findWorkerByID(request->worker_id());

        // A recognized worker will be set to idle once a job is completed.

        if (worker != nullptr) {
            worker->status = WorkerStatus::IDLE;
            std::cout << "Job " << request->job_id()
                      << " completed by worker: " << request->worker_id() << std::endl;
            response->set_status(RegistrationStatus::OK);
        } else {
            response->set_status(RegistrationStatus::UNKNOWN);
        }
        return Status::OK;
    }

    Status Disconnect(ServerContext* context, const WorkerID* request, ServerResponse* response) override {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        Worker* worker = findWorkerByID(request->worker_id());

        // A recognized worker will have its status set to 'OFFLINE' once disconnected.

        if (worker != nullptr) {
            worker->status = WorkerStatus::OFFLINE;
            std::cout << "Worker disconnected: " << request->worker_id() << std::endl;
            response->set_status(RegistrationStatus::OK);
        } else {
            response->set_status(RegistrationStatus::UNKNOWN);
        }
        return Status::OK;
    }

private:
    std::vector<Worker> workers_; // This is the list of all registered workers in the scheduler.
    std::mutex workers_mutex_; // This is used to prevent issues caused by workers connecting simultaneously.
    Worker* findWorkerByID(const std::string& id) { // This checks if a worker is recognized in the scheduler.
        for (auto& worker : workers_) {
            if (worker.id == id) {
                return &worker;
            }
        }
        return nullptr;
    }
};

void RunServer(uint16_t port) {
  std::string server_address = "0.0.0.0:" + std::to_string(port); // **** Used to be 'absl::' but VSCode HATES it. ****
  SchedulerServer service;

  grpc::EnableDefaultHealthCheckService(true);
  ServerBuilder builder;

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  // Register "service" as the instance through which we'll communicate with
  // clients.
  builder.RegisterService(&service);

  // Builds and starts the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Scheduler listening on " << server_address << std::endl;

  // Wait for the server to shutdown
  server->Wait();
}

int main(int argc, char* argv[]) {
    uint16_t port = 50051; // This is the default port.
    if (argc > 1) { // This is included so the port can be changed on the CLI.
        port = static_cast<uint16_t>(std::stoi(argv[1]));
    }
    RunServer(port);
    return 0;
}
