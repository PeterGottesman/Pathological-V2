#include "job.hpp"
#include "protos/render_server.pb.h"

#include <cstdint>

uint32_t Job::getWidth(){
    return this->width;
}

uint32_t Job::getHeight(){
    return this->height;
}

uint32_t Job::getSamples(){
    return this->samples;
}

std::string Job::getGLTF(){
    return this->gltfFile;
}

std::string Job::getOutput(){
    return this->output;
}

float Job::getTime(){
    return this->time;
}

void Job::setStatus(render_server::Status status){
    this->status = status;
}

render_server::Status Job::getStatus(){
    return this->status;
}
