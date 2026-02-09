#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require

#include "common.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;
hitAttributeEXT vec2 attribs;

struct Vertex {
    vec3 position;
	float pad;
};

layout(binding = 2, set = 0, scalar) readonly buffer Vertices { Vertex vertices[]; };
layout(binding = 3, set = 0, scalar) readonly buffer Indices { uint indices[]; };
layout(binding = 4, set = 0, scalar) readonly buffer Materials { Material materials[]; };
layout(binding = 5, set = 0, scalar) readonly buffer MaterialIndices { uint materialIndices[]; };

void main() {
    // Get triangle vertices
    uint primId = gl_PrimitiveID;
    uint i0 = indices[primId * 3 + 0];
    uint i1 = indices[primId * 3 + 1];
    uint i2 = indices[primId * 3 + 2];

    vec3 v0 = vertices[i0].position;
    vec3 v1 = vertices[i1].position;
    vec3 v2 = vertices[i2].position;

    // Compute hit position using barycentric coordinates
    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 hitPos = v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;

    // Compute normal (flat shading)
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 normal = normalize(cross(edge1, edge2));

    // Ensure normal faces against ray direction
    if (dot(normal, gl_WorldRayDirectionEXT) > 0.0) {
        normal = -normal;
    }

    // Get material
    uint matIndex = materialIndices[primId];
    Material mat = materials[matIndex];

    // Check if emissive
    if (mat.emission.x > 0.0 || mat.emission.y > 0.0 || mat.emission.z > 0.0) {
        payload.color = mat.emission;
        payload.done = true;
        return;
    }

    // Lambertian diffuse - sample new direction
    vec3 newDir = randomInHemisphere(normal, payload.seed);

    payload.color = mat.albedo;
    payload.origin = hitPos + normal * 0.001;
    payload.direction = newDir;
    payload.done = false;
}
