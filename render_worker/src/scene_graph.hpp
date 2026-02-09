#pragma once

#include "scene_builder.hpp"
#include "scene.hpp"
#include "vulkan_context.hpp"

#include <tiny_gltf.h>
#include <string>
#include <optional>

class SceneGraph {
public:
    // Factory method (students will implement this)
    static SceneGraph fromGltf(const VulkanContext& ctx,
                              const std::string& filename);

    // Sample animation at given time
    void updateAnimation(float time);

    // Build final Scene for rendering
    Scene build(const VulkanContext& ctx);

private:
    SceneGraph(SceneBuilder builder,
               std::optional<tinygltf::Animation> animation,
               std::shared_ptr<tinygltf::Model> model);

    SceneBuilder m_builder;
    std::optional<tinygltf::Animation> m_animation;
    std::shared_ptr<tinygltf::Model> m_model;

    // Animation sampling helpers
    glm::vec3 sampleVec3Channel(const tinygltf::AnimationChannel& channel,
                                 int samplerIdx,
                                 float time);
    glm::quat sampleQuatChannel(const tinygltf::AnimationChannel& channel,
                                int samplerIdx,
                                float time);
};

// Helper function to load glTF file
tinygltf::Model loadGltfFile(const std::string& filename);
