#include "render_server.hpp"
#include "render_worker.hpp"
#include "scheduler_client.hpp"
#include <CLI/CLI.hpp>

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

    std::string schedulerAddress = "localhost:50051";
    app.add_option("address", schedulerAddress, "Address and port to find scheduler at")->required();

    RunServer(50051);

    std::string target_str = "localhost:50051";
    // We indicate that the channel isn't authenticated (use of
    // InsecureChannelCredentials()).
    SchedulerClient client(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

    // Giving sample connection address
    //std::string connection_address = "127.0.0.1:50051";
    //int response = client.EstablishConnection(connection_address);
    //std::cout << "Greeter received: " << response << std::endl;

    return 0;
}
