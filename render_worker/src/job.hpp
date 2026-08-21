#pragma once
#include <cstdint>
#include <string>
#include <utility>

#include "protos/render_server.pb.h"

// Holds all the info of a job recieved from the scheduler
class Job {
public:
    Job(uint32_t width, uint32_t height, uint32_t samples, std::string gltfFile, std::string output, float time,
        uint32_t frameIndex, render_server::Status status)
        : width(width),
          height(height),
          samples(samples),
          gltfFile(std::move(gltfFile)),
          output(std::move(output)),
          time(time),
          frameIndex(frameIndex),
          status(status) {}
    uint32_t getWidth();
    uint32_t getHeight();
    uint32_t getSamples();
    std::string getGLTF();
    std::string getOutput();
    float getTime();
    uint32_t getFrameIndex();
    void setStatus(render_server::Status status);  // Only setter on Status as that is the only thing
                                                   // that needs to be changed. The rest are set
                                                   // on construction
    render_server::Status getStatus();
    void print();

private:
    uint32_t width;
    uint32_t height;
    uint32_t samples;
    std::string gltfFile;
    std::string output;
    float time;
    uint32_t frameIndex;
    render_server::Status status;
};
