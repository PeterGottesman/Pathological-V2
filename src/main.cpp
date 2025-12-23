#include "vulkan_context.hpp"
#include "scene.hpp"
#include "path_tracer.hpp"

#include <CLI/CLI.hpp>

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    CLI::App app{"Pathological - Vulkan Path Tracer"};

    uint32_t width = 1024;
    uint32_t height = 1024;
    uint32_t samples = 256;
    std::string output = "output.png";

    app.add_option("-W,--width", width, "Image width")->default_val(1024);
    app.add_option("-H,--height", height, "Image height")->default_val(1024);
    app.add_option("-s,--samples", samples, "Samples per pixel")->default_val(256);
    app.add_option("-o,--output", output, "Output filename")->default_val("output.png");

    CLI11_PARSE(app, argc, argv);

    try {
        std::cout << "Pathological - Vulkan Path Tracer" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Resolution: " << width << "x" << height << std::endl;
        std::cout << "Samples: " << samples << std::endl;
        std::cout << "Output: " << output << std::endl;
        std::cout << std::endl;

        VulkanContext ctx;
        Scene scene(ctx);
        PathTracer tracer(ctx, scene, width, height);

        tracer.render(samples);
        tracer.saveImage(output);

        std::cout << std::endl;
        std::cout << "Done!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
