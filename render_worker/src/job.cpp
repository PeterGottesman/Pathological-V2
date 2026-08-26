#include "job.hpp"

#include <cstdint>

#include "protos/render_server.pb.h"

uint32_t Job::getWidth() { return this->width; }

uint32_t Job::getHeight() { return this->height; }

uint32_t Job::getSamples() { return this->samples; }

std::string Job::getGLTF() { return this->gltfFile; }

std::string Job::getOutput() { return this->output; }

float Job::getTime() { return this->time; }

void Job::setStatus(render_server::Status status) { this->status = status; }

render_server::Status Job::getStatus() { return this->status; }

// Mainly for debugging
void Job::print() {
    std::cout << width << "\n";
    std::cout << height << "\n";
    std::cout << samples << "\n";
    std::cout << gltfFile << "\n";
    std::cout << output << "\n";
    std::cout << time << "\n";
    std::cout << status << "\n";
}
