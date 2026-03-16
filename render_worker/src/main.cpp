#include "render_server.hpp"
#include "render_worker.hpp"
#include "scheduler_client.hpp"
#include <CLI/CLI.hpp>

#include <cstdint>
#include <iostream>
#include <string>

Status SchedulerServer::RenderJob(ServerContext *context, const RenderJobRequest *request, RenderJobResponse *response) {
    // Sample implementation
    generateScene(request->width(), request->height(), request->samples(),
        request->scene_location(), request->output_name(), request->time());
    response->set_job_identifier(rand() % 10000);
    return Status::OK;
}

int main(int argc, char** argv) {
    CLI::App app{"Pathological - Vulkan Path Tracer"};

    std::string worker_id = "test_client_1";
    std::string schedulerAddress;
    std::string renderServerAddress = "127.0.0.1";
    uint32_t renderServerPort = 50051;

    app.add_option("schedulerAddress", schedulerAddress, "Address and port to find scheduler at")->required();
    app.add_option("-a,--render-address", renderServerAddress, "Address of current machine running program")->default_val("127.0.0.1");
    app.add_option("-p,--port", renderServerPort, "Port to launch server")->default_val(50051);
    CLI11_PARSE(app, argc, argv);

    // Sends scheduler IP and port

    SchedulerClient client(
        grpc::CreateChannel(schedulerAddress, grpc::InsecureChannelCredentials()),
        worker_id,
        renderServerAddress,
        renderServerPort
    );

    client.EstablishConnection();

    RunServer(renderServerPort);

    return 0;
}
