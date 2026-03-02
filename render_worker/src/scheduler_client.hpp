#pragma once

#include <grpcpp/grpcpp.h>
#include "../build/protos/scheduler_server.grpc.pb.h"
#include "../build/protos/scheduler_server.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using scheduler_server::WorkerConnection;
using scheduler_server::WorkerIP;
using scheduler_server::ServerResponse;

class SchedulerClient {
    public:
        SchedulerClient(std::shared_ptr<Channel> channel);
        int EstablishConnection(const std::string& workerIP);
    private:
        std::unique_ptr<WorkerConnection::Stub> stub_;
};
