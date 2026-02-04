#pragma once

#include <cstdint>
#include <string>

void generateScene(uint32_t width, uint32_t height, uint32_t samples,
    std::string gltfFile, std::string output, float time, uint32_t tileSize, bool verbose);
