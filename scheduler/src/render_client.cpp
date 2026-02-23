#include <cstdint>
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

#include "../build/protos/render_server.grpc.pb.h"
#include "protos/render_server.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using render_server::RenderWorker;
using render_server::RenderJobRequest;
using render_server::RenderJobResponse;
using render_server::RenderStatusRequest;
using render_server::RenderStatusResponse;

// foobar struct for sending data
struct myData {
    std::string scene_location;
    uint32_t seconds;
    uint32_t frames;
};

class RenderWorkerClient {
 public:
  RenderWorkerClient(std::shared_ptr<Channel> channel)
      : stub_(RenderWorker::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  int RenderJob(myData data) {
    // Data we are sending and getting back
    RenderJobRequest request;
    RenderJobResponse response;
    ClientContext context;

    // Adds data to request
    request.set_scene_location(data.scene_location);
    request.set_seconds(data.seconds);
    request.set_frames(data.frames);

    // The actual RPC.
    Status status = stub_->RenderJob(&context, request, &response);

    // Act upon its status.
    if (status.ok()) {
      return response.job_identifier();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
  }

  int RenderStatus(int job) {
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
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
  }

 private:
  std::unique_ptr<RenderWorker::Stub> stub_;
};

int main(int argc, char** argv) {
  // Sample data
  struct myData testStruct;
  testStruct.scene_location = "testing";
  testStruct.frames = 60;
  testStruct.seconds = 2;

  int job = 100;

  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created.
  std::string target_str = "localhost:50051";

  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  RenderWorkerClient client(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  int response = client.RenderJob(testStruct);
  int response2 = client.RenderStatus(job);
  std::cout << "Greeter received: " << response << std::endl;
  std::cout << "Greeter received: " << response2 << std::endl;
  return 0;
}
