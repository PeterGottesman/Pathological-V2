#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include "../build/protos/render_server.grpc.pb.h"
#include "../build/protos/render_server.pb.h"
#include "scheduler_client.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using render_server::RenderWorker;
using render_server::RenderJobRequest;
using render_server::RenderJobResponse;
using render_server::RenderStatusRequest;
using render_server::RenderStatusResponse;

class RenderServer final : public RenderWorker::Service {
public:
    RenderServer(SchedulerClient& client, std::string worker_id) : client(client), worker_id(worker_id){}
    Status RenderJob(ServerContext *context, const RenderJobRequest *request,
        RenderJobResponse *response);
    Status RenderStatus(ServerContext *context, const RenderStatusRequest *request, RenderStatusResponse *response);

private:
    SchedulerClient& client;
    std::string worker_id;
};

std::unique_ptr<Server> BuildServer(uint16_t port, SchedulerClient& client, std::string worker_id);
