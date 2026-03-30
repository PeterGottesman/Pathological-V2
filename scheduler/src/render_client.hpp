#pragma once

#include <grpcpp/grpcpp.h>

#include "../build/protos/render_server.grpc.pb.h"
#include "../build/protos/render_server.pb.h"
#include "renderRequest.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using render_server::RenderWorker;
using render_server::RenderJobRequest;
using render_server::RenderJobResponse;
using render_server::RenderStatusRequest;
using render_server::RenderStatusResponse;

class RenderWorkerClient {
public:
    RenderWorkerClient(std::shared_ptr<Channel> channel) : stub_(RenderWorker::NewStub(channel)) {}

    int RenderJob(const RenderRequest& render);
    int RenderStatus(int job);

private:
    std::unique_ptr<RenderWorker::Stub> stub_;
};