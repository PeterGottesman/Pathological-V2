#pragma once

#include <grpcpp/grpcpp.h>

#include "protos/render_server.grpc.pb.h"
#include "protos/render_server.pb.h"
#include "renderRequest.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using render_server::RenderJobRequest;
using render_server::RenderJobResponse;
using render_server::RenderStatusRequest;
using render_server::RenderStatusResponse;
using render_server::RenderWorker;

class RenderWorkerClient {
public:
    RenderWorkerClient(const std::shared_ptr<Channel>& channel) : stub_(RenderWorker::NewStub(channel)) {}

    std::string RenderJob(const std::shared_ptr<RenderRequest>& render);
    int RenderStatus(std::string job);

private:
    std::unique_ptr<RenderWorker::Stub> stub_;
};
