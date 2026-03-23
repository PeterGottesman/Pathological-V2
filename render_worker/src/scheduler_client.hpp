#pragma once

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <string>

#include "../build/protos/scheduler_server.grpc.pb.h"
#include "../build/protos/scheduler_server.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using scheduler_server::WorkerConnection;
using scheduler_server::RegistrationStatus;
using scheduler_server::WorkerInfo;
using scheduler_server::WorkerID;
using scheduler_server::ServerResponse;
using scheduler_server::JobCompletedRequest;

// Assembles the client's payload, sends it and presents the response back
// from the server.

class SchedulerClient {
public:
    SchedulerClient(std::shared_ptr<Channel> channel, const std::string& worker_id, const std::string& worker_ip, uint32_t port) : stub_(WorkerConnection::NewStub(channel)),
        worker_id_(worker_id),
        worker_ip_(worker_ip),
        port_(port) {}

    int EstablishConnection();
    void Heartbeat();
    void JobCompleted(int32_t job_id);
    void Disconnect();

private:
    std::unique_ptr<WorkerConnection::Stub> stub_;
    std::string worker_id_;
    std::string worker_ip_;
    uint32_t port_;
};
