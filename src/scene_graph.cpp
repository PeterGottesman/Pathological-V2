#include "scene_graph.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>
#include <iostream>
#include <stdexcept>

tinygltf::Model loadGltfFile(const std::string& filename) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;

    bool success = loader.LoadASCIIFromFile(&model, &err, &warn, filename);

    if (!warn.empty()) {
        std::cerr << "glTF warning: " << warn << std::endl;
    }

    if (!success || !err.empty()) {
        throw std::runtime_error("Failed to load glTF file '" + filename + "': " + err);
    }

    if (model.scenes.empty()) {
        throw std::runtime_error("glTF file has no scenes");
    }

    return model;
}

SceneGraph::SceneGraph(SceneBuilder builder,
                       std::optional<tinygltf::Animation> animation)
    : m_builder(std::move(builder))
    , m_animation(std::move(animation))
{
}

Scene SceneGraph::build(const VulkanContext& ctx) {
    return m_builder.flattenToScene(ctx);
}

void SceneGraph::updateAnimation(float time) {
    // TODO: Implement animation sampling
    // For now, animations are not supported
    if (m_animation.has_value()) {
        std::cerr << "Warning: Animation support not yet implemented" << std::endl;
    }
}

SceneGraph SceneGraph::fromGltf(const VulkanContext& ctx,
                                const std::string& filename) {
    tinygltf::Model model = loadGltfFile(filename);

    SceneBuilder builder;

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];

    for (int nodeIdx : scene.nodes) {
        builder.loadGltfNode(model.nodes[nodeIdx], std::nullopt, model);
    }

    std::optional<tinygltf::Animation> anim;
    if (!model.animations.empty()) {
        anim = model.animations[0];
    }

    return SceneGraph(builder, anim);
}

glm::vec3 SceneGraph::sampleVec3Channel(const tinygltf::Animation& anim,
                                        const tinygltf::AnimationChannel& channel,
                                        float time) {
    // TODO
    return glm::vec3(0.0f);
}

glm::quat SceneGraph::sampleQuatChannel(const tinygltf::Animation& anim,
                                       const tinygltf::AnimationChannel& channel,
                                       float time) {
    // TODO
    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}
