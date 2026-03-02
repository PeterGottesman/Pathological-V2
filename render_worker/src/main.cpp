#include "path_tracer.hpp"
#include "scene_graph.hpp"
#include "vulkan_context.hpp"

#include "render_server.hpp"

#include <CLI/CLI.hpp>

#include <cstdint>
#include <string>

int main(int argc, char **argv) {
  CLI::App app{"Pathological - Vulkan Path Tracer"};

  RunServer(50051);

  std::string gltfFile;
  uint32_t width = 1024;
  uint32_t height = 1024;
  uint32_t samples = 256;
  std::string output = "output.png";
  float time = 0.0f;

  app.add_option("gltf", gltfFile, "glTF scene file")->required();
  app.add_option("-W,--width", width, "Image width")->default_val(1024);
  app.add_option("-H,--height", height, "Image height")->default_val(1024);
  app.add_option("-s,--samples", samples, "Samples per pixel")
      ->default_val(256);
  app.add_option("-o,--output", output, "Output filename")
      ->default_val("output.png");
  app.add_option("-t,--time", time, "Animation time in seconds")
      ->default_val(0.0f);

  app.add_option("schedulerAddress", schedulerAddress,
                 "Address and port to find scheduler at")
      ->required();
  app.add_option("-a,--render-address", renderServerAddress,
                 "Address of current machine running program")
      ->default_val("127.0.0.1");
  app.add_option("-p,--port", renderServerPort, "Port to launch server")
      ->default_val(50051);
  CLI11_PARSE(app, argc, argv);

  // Sends scheduler IP and port

  SchedulerClient client(
      grpc::CreateChannel(schedulerAddress, grpc::InsecureChannelCredentials()),
      worker_id, renderServerAddress, renderServerPort);

  client.EstablishConnection();

  RunServer(renderServerPort, client, worker_id);

  return 0;
}
