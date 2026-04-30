#pragma once
#include <cstdint>
#include <string>
#include "protos/render_server.pb.h"

// Holds all the info of a job recieved from the scheduler
class Job {
public:
    Job(uint32_t width, uint32_t height, uint32_t samples,
        std::string gltfFile, std::string output, float time, render_server::Status status) :
        width(width), height(height), samples(samples),
        gltfFile(gltfFile), output(output), time(time), status(status) {}
    uint32_t getWidth();
    uint32_t getHeight();
    uint32_t getSamples();
    std::string getGLTF();
    std::string getOutput();
    float getTime();
    void setStatus(render_server::Status status);   // Only setter on Status as that is the only thing
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
    render_server::Status status;
};
