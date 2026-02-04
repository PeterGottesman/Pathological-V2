#include "render_worker.hpp"
#include "vulkan_context.hpp"
#include "scene_graph.hpp"
#include "path_tracer.hpp"

#include <iostream>

void generateScene(uint32_t width, uint32_t height, uint32_t samples,
    std::string gltfFile, std::string output, float time, uint32_t tileSize, bool verbose){
    try {
        std::cout << "Pathological - Vulkan Path Tracer" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "glTF File: " << gltfFile << std::endl;
        std::cout << "Resolution: " << width << "x" << height << std::endl;
        std::cout << "Samples: " << samples << std::endl;
        std::cout << "Animation Time: " << time << "s" << std::endl;
        std::cout << "Tile Size: " << tileSize << "x" << tileSize << std::endl;
        std::cout << "Output: " << output << std::endl;
        std::cout << std::endl;

        {
            VulkanContext ctx;
            SceneGraph sceneGraph = SceneGraph::fromGltf(ctx, gltfFile);
            sceneGraph.updateAnimation(time);
            Scene scene = sceneGraph.build(ctx);
            PathTracer tracer(ctx, scene, width, height);

            tracer.render(samples, tileSize, verbose);
            tracer.saveImage(output);

            // Wait for all GPU work to complete before cleanup
            ctx.device().waitIdle();
        } // Explicit scope to ensure cleanup order

        std::cout << std::endl;
        std::cout << "Done!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
