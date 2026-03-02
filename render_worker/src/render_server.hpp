#pragma once

#include <grpcpp/grpcpp.h>
#include "../build/protos/render_server.grpc.pb.h"
#include "../build/protos/render_server.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using render_server::RenderWorker;
using render_server::RenderJobRequest;
using render_server::RenderJobResponse;
using render_server::RenderStatusRequest;
using render_server::RenderStatusResponse;

class SchedulerServer final : public RenderWorker::Service {
    Status RenderJob(ServerContext *context, const RenderJobRequest *request, RenderJobResponse *response);
    Status RenderStatus(ServerContext *context, const RenderStatusRequest *request, RenderStatusResponse *response);
};

void RunServer(uint16_t port);
