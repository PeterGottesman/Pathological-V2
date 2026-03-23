#include "scheduler_server.hpp"
#include "scheduler.hpp"

Status SchedulerServer::EstablishConnection(ServerContext *context, const WorkerInfo *request, ServerResponse *response) {
    std::cout << "Worker connected: " << request->worker_ip() << std::endl;

    // Check if worker is reconnecting by searching scheduler's worker list
    Worker* existing = Scheduler::getInstance().findWorkerByID(request->worker_id());

    if (existing != nullptr) {
        // Worker is reconnecting, reset its status
        Scheduler::getInstance().reconnectWorker(request->worker_id());
        response->set_status(RegistrationStatus::RECONNECTED);
        response->set_assigned_id(existing->id);
    } else {
        // New worker registering
        Scheduler::getInstance().registerWorker(request->worker_id(), request->worker_ip(), request->port());
        response->set_status(RegistrationStatus::OK);
        response->set_assigned_id(request->worker_id());
    }
    return Status::OK;
}

Status SchedulerServer::Heartbeat(ServerContext* context, const WorkerID* request, ServerResponse* response) {
    Worker* worker = Scheduler::getInstance().findWorkerByID(request->worker_id());

    // If a worker can be recognized by the scheduler, the connection is still active.
    if (worker != nullptr) {
        response->set_status(RegistrationStatus::OK);
    // If a worker cannot be recognized, it must reconnect.
    } else {
        response->set_status(RegistrationStatus::UNKNOWN);
    }
    return Status::OK;
}

Status SchedulerServer::JobCompleted(ServerContext* context, const JobCompletedRequest* request, ServerResponse* response) {
    Worker* worker = Scheduler::getInstance().findWorkerByID(request->worker_id());

    // A recognized worker will be set to idle once a job is completed.
    if (worker != nullptr) {
        Scheduler::getInstance().markWorkerIdle(request->worker_id());
        std::cout << "Job " << request->job_id() << " completed by worker: " << request->worker_id() << std::endl;
        response->set_status(RegistrationStatus::OK);
    } else {
        response->set_status(RegistrationStatus::UNKNOWN);
    }
    return Status::OK;
}

Status SchedulerServer::Disconnect(ServerContext* context, const WorkerID* request, ServerResponse* response) {
    Worker* worker = Scheduler::getInstance().findWorkerByID(request->worker_id());

    // A recognized worker will have its status set to 'OFFLINE' once disconnected.
    if (worker != nullptr) {
        Scheduler::getInstance().markWorkerOffline(request->worker_id());
        std::cout << "Worker disconnected: " << request->worker_id() << std::endl;
        response->set_status(RegistrationStatus::OK);
    } else {
        response->set_status(RegistrationStatus::UNKNOWN);
    }
    return Status::OK;
}

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

/* Sample before being incorporated into main
int main(int argc, char *argv[]){
    RunServer(50051);
    return 0;
}
*/