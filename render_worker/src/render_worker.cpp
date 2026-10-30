#include "render_worker.hpp"

#include <cstdlib>
#include <iostream>

#include "job.hpp"
#include "path_tracer.hpp"
#include "s3_manager.hpp"
#include "scene_graph.hpp"
#include "vulkan_context.hpp"

namespace {
std::string envOr(const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    return (value != nullptr && value[0] != '\0') ? std::string(value) : std::string(fallback);
}
}  // namespace

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

        S3Config config{
            .bucketName = envOr("S3_bucket", "pathological-capstone-s3-bucket"),
            .region = envOr("S3_region", "us-east-2"),
            .profileName = envOr("AWS_PROFILE", "default"),
            .endpointOverride = envOr("S3_ENDPOINT", ""),
            .usePathStyle = std::getenv("S3_ENDPOINT") != nullptr,
        };
        S3Manager s3Manager(config);

        std::string s3Key = std::filesystem::path(gltfFile).filename().string();
        std::string localGltfPath = "temp_" + s3Key;

        std::cout << "Downloading scene '" << s3Key << "' from S3..." << "\n";
        if (!s3Manager.getObject(s3Key, localGltfPath)) {
            throw std::runtime_error("failed to download scene file from S3: " + s3Key);
        }
        {
            VulkanContext ctx;
            // 3. Load from the local downloaded file instead of the raw key
            SceneGraph sceneGraph = SceneGraph::fromGltf(ctx, localGltfPath);
            sceneGraph.updateAnimation(time);
            Scene scene = sceneGraph.build(ctx);
            PathTracer tracer(ctx, scene, width, height);

            tracer.render(samples, tileSize, verbose);
            tracer.saveImage(output);

            // Wait for all GPU work to complete before cleanup
            ctx.device().waitIdle();
        }

        // 4. Clean up temporary downloaded glTF file
        if (std::filesystem::exists(localGltfPath)) {
            std::filesystem::remove(localGltfPath);
            std::cout << "Cleaned up temporary file: " << localGltfPath << "\n";
        }

        std::cout << "\n";
        std::cout << "Done!" << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        throw;  // Re-throw so caller (RenderJob) knows it failed
    }
}