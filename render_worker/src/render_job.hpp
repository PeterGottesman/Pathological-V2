#pragma once
#include <cstdint>
#include <string>
#include "protos/render_server.pb.h"

class RenderJob {
public:
    RenderJob(uint32_t width, uint32_t height, uint32_t samples,
        std::string gltfFile, std::string output, float time, render_server::Status status) :
        width(width), height(height), samples(samples),
        gltfFile(gltfFile), output(output), time(time), status(status) {}
    uint32_t getWidth();
    uint32_t getHeight();
    uint32_t getSamples();
    std::string getGLTF();
    std::string getOutput();
    float getTime();
    render_server::Status getStatus();


private:
    uint32_t width;
    uint32_t height;
    uint32_t samples;
    std::string gltfFile;
    std::string output;
    float time;
    render_server::Status status;
};
