#include "vulkan_context.hpp"

#include <iostream>

int main(int argc, char** argv) {
    try {
        std::cout << "Initializing Vulkan context..." << std::endl;
        VulkanContext ctx;
        std::cout << "Vulkan context initialized successfully!" << std::endl;

        std::cout << "Shader group handle size: "
                  << ctx.rtProperties().shaderGroupHandleSize << std::endl;
        std::cout << "Shader group base alignment: "
                  << ctx.rtProperties().shaderGroupBaseAlignment << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
