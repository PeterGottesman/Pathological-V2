#include "render_client.hpp"

// Assembles the client's payload using getter functions from renderRequest.cpp, sends it and presents the response back
// from the server.
bool RenderWorkerClient::RenderJob(const std::shared_ptr<RenderRequest>& render, const std::string& job_id,
                                   uint32_t frame_index, float time) {
    // Data we are sending and getting back
    RenderJobRequest request;
    RenderJobResponse response;
    ClientContext context;

    // Adds data to request
    request.set_scene_location(render->getSceneFileUrl());
    request.set_output_name(render->getOutputFileName());
    request.set_width(static_cast<uint32_t>(render->getWidth()));
    request.set_height(static_cast<uint32_t>(render->getHeight()));
    request.set_samples(static_cast<uint32_t>(render->getSamplesPerPixel()));
    request.set_time(time);
    request.set_job_id(job_id);
    request.set_frame_index(frame_index);

    // The actual RPC.
    Status status = stub_->RenderJob(&context, request, &response);

    // Act upon its status.
    if (status.ok()) {
        return true;
    } else {
        std::cout << status.error_code() << ": " << status.error_message() << "\n";
        return false;
    }
}

int RenderWorkerClient::RenderStatus(std::string job) {
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
        std::cout << status.error_code() << ": " << status.error_message() << "\n";
        return -1;
    }
}
