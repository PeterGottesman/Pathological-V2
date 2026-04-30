#pragma once

#include <grpcpp/grpcpp.h>
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

// Singleton pattern since it is declared in global scope
class SchedulerClient {
public:
    static SchedulerClient& getInstance(){
        static SchedulerClient instance = SchedulerClient();
        return instance;
    }

    int EstablishConnection();
    void SetMembers(std::shared_ptr<Channel> channel, const std::string& worker_id, const std::string& worker_ip, uint32_t port);
    void Heartbeat();
    void JobCompleted(std::string job_id);
    void Disconnect();

private:
    SchedulerClient() = default;
    SchedulerClient(const SchedulerClient&) = delete;
    SchedulerClient& operator=(const SchedulerClient&) = delete;

    std::unique_ptr<WorkerConnection::Stub> stub_;
    std::string worker_id_;
    std::string worker_ip_;
    uint32_t port_;
};
