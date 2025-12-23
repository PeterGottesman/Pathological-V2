// Shared definitions for all ray tracing shaders

struct RayPayload {
    vec3 color;
    vec3 origin;
    vec3 direction;
    uint seed;
    bool done;
};

struct Material {
    vec3 albedo;
    float pad0;
    vec3 emission;
    float pad1;
};

// PCG random number generator
uint pcg(inout uint state) {
    uint prev = state * 747796405u + 2891336453u;
    uint word = ((prev >> ((prev >> 28u) + 4u)) ^ prev) * 277803737u;
    state = prev;
    return (word >> 22u) ^ word;
}

float randomFloat(inout uint seed) {
    return float(pcg(seed)) / float(0xFFFFFFFFu);
}

vec3 randomInHemisphere(vec3 normal, inout uint seed) {
    // Cosine-weighted hemisphere sampling
    float r1 = randomFloat(seed);
    float r2 = randomFloat(seed);

    float phi = 2.0 * 3.14159265359 * r1;
    float cosTheta = sqrt(r2);
    float sinTheta = sqrt(1.0 - r2);

    vec3 w = normal;
    vec3 a = abs(w.x) > 0.9 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 u = normalize(cross(a, w));
    vec3 v = cross(w, u);

    return normalize(u * cos(phi) * sinTheta + v * sin(phi) * sinTheta + w * cosTheta);
}
