#pragma once

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <string>
#include <vector>
#include <mutex>

#include "../build/protos/scheduler_server.grpc.pb.h"
#include "../build/protos/scheduler_server.pb.h"

#include "worker.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using scheduler_server::WorkerConnection;
using scheduler_server::RegistrationStatus;
// using scheduler_server::WorkerStatus; **** Used internally ****
using scheduler_server::WorkerInfo;
using scheduler_server::WorkerID;
using scheduler_server::ServerResponse;
using scheduler_server::JobCompletedRequest;

class SchedulerServer final : public WorkerConnection::Service {
public:
    Status EstablishConnection(ServerContext* context, const WorkerInfo* request, ServerResponse* response) override;

    // **** 'override' is used to match this function with the virtual method in the parent class made by gRPC. ****

    Status Heartbeat(ServerContext* context, const WorkerID* request, ServerResponse* response) override;

    Status JobCompleted(ServerContext* context, const JobCompletedRequest* request, ServerResponse* response) override;

    Status Disconnect(ServerContext* context, const WorkerID* request, ServerResponse* response) override;

private:
    std::vector<Worker> workers_; // This is the list of all registered workers in the scheduler.
    std::mutex workers_mutex_; // This is used to prevent issues caused by workers connecting simultaneously.
    Worker* findWorkerByID(const std::string& id);
};

void RunServer(uint16_t port);