#pragma once

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <boost/uuid.hpp>
#include "../build/protos/render_server.grpc.pb.h"
#include "../build/protos/render_server.pb.h"
#include "scheduler_client.hpp"
#include "render_jobs.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using render_server::RenderWorker;
using render_server::RenderJobRequest;
using render_server::RenderJobResponse;
using render_server::RenderStatusRequest;
using render_server::RenderStatusResponse;
using boost::uuids::random_generator;
using boost::uuids::uuid;
using boost::uuids::to_string;

class RenderServer final : public RenderWorker::Service {
public:
    RenderServer(SchedulerClient& client, std::string worker_id, random_generator random_gen) :
        client(client), worker_id(worker_id), random_gen(random_gen){}
    Status RenderJob(ServerContext *context, const RenderJobRequest *request,
        RenderJobResponse *response);
    Status RenderStatus(ServerContext *context, const RenderStatusRequest *request, RenderStatusResponse *response);

private:
    SchedulerClient& client;
    std::string worker_id;
    random_generator random_gen;
    RenderJobs jobs;
};

std::unique_ptr<Server> BuildServer(uint16_t port, SchedulerClient& client, std::string worker_id, random_generator random_gen);
