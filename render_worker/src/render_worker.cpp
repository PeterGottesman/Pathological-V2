#include "render_worker.hpp"

#include <iostream>

#include "path_tracer.hpp"
#include "scene_graph.hpp"
#include "vulkan_context.hpp"

// Peter's implementation. Refer to him when it comes to
// any questions about the VULKAN path tracer
void generateScene(uint32_t width, uint32_t height, uint32_t samples, const std::string& gltfFile,
                   const std::string& output, float time, uint32_t tileSize, bool verbose) {
    try {
        std::cout << "Pathological - Vulkan Path Tracer" << "\n";
        std::cout << "==================================" << "\n";
        std::cout << "glTF File: " << gltfFile << "\n";
        std::cout << "Resolution: " << width << "x" << height << "\n";
        std::cout << "Samples: " << samples << "\n";
        std::cout << "Animation Time: " << time << "s" << "\n";
        std::cout << "Tile Size: " << tileSize << "x" << tileSize << "\n";
        std::cout << "Output: " << output << "\n";
        std::cout << "\n";

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
        }  // Explicit scope to ensure cleanup order

        std::cout << "\n";
        std::cout << "Done!" << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}
