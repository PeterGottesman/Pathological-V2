#version 460
#extension GL_EXT_ray_tracing : require

#include "common.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    // No environment lighting - return black
    payload.color = vec3(0.0);
    payload.done = true;
}
