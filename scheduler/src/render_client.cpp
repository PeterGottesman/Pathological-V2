#include "render_client.hpp"

/* Sample struct for sending data

struct MyData {
    std::string scene_location;
    std::string output_name;
    uint32_t width;
    uint32_t height;
    uint32_t samples;
    float time;
};

*/

// Assembles the client's payload, sends it and presents the response back
// from the server.
int RenderWorkerClient::RenderJob(const RenderRequest& render) {
  // Data we are sending and getting back
  RenderJobRequest request;
  RenderJobResponse response;
  ClientContext context;

  // Adds data to request
  request.set_scene_location(render.getSceneFileUrl());
  request.set_output_name(render.getOutputFileName());
  request.set_width(static_cast<uint32_t>(render.getWidth()));
  request.set_height(static_cast<uint32_t>(render.getHeight()));
  request.set_samples(static_cast<uint32_t>(render.getSamplesPerPixel()));
  request.set_time(static_cast<float>(render.getAnimationRuntimeInFrames())); // ****I'm ASSUMING this is what is from the frontend

  // The actual RPC.
  Status status = stub_->RenderJob(&context, request, &response);

  // Act upon its status.
  if (status.ok()) {
    return response.job_identifier();
  } else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    return -1;
  }
}

int RenderWorkerClient::RenderStatus(int job) {
  // Data we are sending and getting back
  RenderStatusRequest request;
  RenderStatusResponse response;
  ClientContext context;

  // Adds data to request
  request.set_job_identifier(job);

  // The actual RPC.
  Status status = stub_->RenderStatus(&context, request, &response);

  // Act upon its status.
  if (status.ok()) {
    return response.status();
  } else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    return -1;
  }
}

/*

int main(int argc, char** argv) {
  // Sample data
  struct MyData test_struct;
  test_struct.scene_location = "../test_scenes/cornell_box.gltf";
  test_struct.output_name = "output.png";
  test_struct.width = 1024;
  test_struct.height = 1024;
  test_struct.samples = 256;
  test_struct.time = 0.0;


  // Sample job id for testing
  int job = 100;

  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created.
  std::string target_str = "localhost:50051";

  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  RenderWorkerClient client(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  // Sample server invocations
  int response = client.RenderJob(test_struct);
  int response_two = client.RenderStatus(job);
  std::cout << "Greeter received: " << response << std::endl;
  std::cout << "Greeter received: " << response_two << std::endl;
  return 0;
}

*/