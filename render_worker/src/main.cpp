#include "render_server.hpp"
#include "render_worker.hpp"
#include "scheduler_client.hpp"
#include <CLI/CLI.hpp>

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    CLI::App app{"Pathological - Vulkan Path Tracer"};


    std::string gltfFile;
    uint32_t width = 1024;
    uint32_t height = 1024;
    uint32_t samples = 256;
    std::string output = "output.png";
    float time = 0.0f;


    /*
    app.add_option("gltf", gltfFile, "glTF scene file")->required();
    app.add_option("-W,--width", width, "Image width")->default_val(1024);
    app.add_option("-H,--height", height, "Image height")->default_val(1024);
    app.add_option("-s,--samples", samples, "Samples per pixel")->default_val(256);
    app.add_option("-o,--output", output, "Output filename")->default_val("output.png");
    app.add_option("-t,--time", time, "Animation time in seconds")->default_val(0.0f);
    */

    generateScene(width, height, samples, "../test_scenes/cornell_box.gltf", "output.png", time);

    std::string schedulerAddress = "localhost:50051";
    app.add_option("address", schedulerAddress, "Address and port to find scheduler at")->required();

    RunServer(50051);

    std::string target_str = "localhost:50051";
    // We indicate that the channel isn't authenticated (use of
    // InsecureChannelCredentials()).
    SchedulerClient client(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

    // Giving sample connection address
    std::string connection_address = "127.0.0.1:50051";
    int response = client.EstablishConnection(connection_address);
    std::cout << "Greeter received: " << response << std::endl;

    return 0;
}
