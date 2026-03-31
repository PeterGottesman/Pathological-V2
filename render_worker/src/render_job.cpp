#include "render_job.hpp"
#include "protos/render_server.pb.h"
#include <cstdint>

uint32_t RenderJob::getWidth(){
    return this->width;
}

uint32_t RenderJob::getHeight(){
    return this->height;
}

uint32_t RenderJob::getSamples(){
    return this->samples;
}

std::string RenderJob::getGLTF(){
    return this->gltfFile;
}

std::string RenderJob::getOutput(){
    return this->output;
}

float RenderJob::getTime(){
    return this->time;
}

render_server::Status RenderJob::getStatus(){
    return this->status;
}
