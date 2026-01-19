#include "scene_graph.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <glm/gtc/quaternion.hpp>

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
                       std::optional<tinygltf::Animation> animation,
                       std::shared_ptr<tinygltf::Model> model)
    : m_builder(std::move(builder))
    , m_animation(std::move(animation))
    , m_model(std::move(model))
{
}

Scene SceneGraph::build(const VulkanContext& ctx) {
    return m_builder.flattenToScene(ctx);
}

void SceneGraph::updateAnimation(float time) {
    if (!m_animation.has_value() || !m_model) {
        return;
    }

    const tinygltf::Animation& anim = m_animation.value();

    // Apply each animation channel
    for (const auto& channel : anim.channels) {
        int nodeIdx = channel.target_node;
        const std::string& path = channel.target_path;

        if (path == "translation") {
            glm::vec3 translation = sampleVec3Channel(channel, channel.sampler, time);
            m_builder.setNodeTranslation(nodeIdx, translation);
        } else if (path == "rotation") {
            glm::quat rotation = sampleQuatChannel(channel, channel.sampler, time);
            m_builder.setNodeRotation(nodeIdx, rotation);
        } else if (path == "scale") {
            glm::vec3 scale = sampleVec3Channel(channel, channel.sampler, time);
            m_builder.setNodeScale(nodeIdx, scale);
        }
    }
}

SceneGraph SceneGraph::fromGltf(const VulkanContext& ctx,
                                const std::string& filename) {
    auto model = std::make_shared<tinygltf::Model>(loadGltfFile(filename));

    SceneBuilder builder;

    const tinygltf::Scene& scene = model->scenes[model->defaultScene];

    for (int nodeIdx : scene.nodes) {
        builder.loadGltfNode(model->nodes[nodeIdx], std::nullopt, *model);
    }

    std::optional<tinygltf::Animation> anim;
    if (!model->animations.empty()) {
        anim = model->animations[0];
    }

    return SceneGraph(builder, anim, model);
}

glm::vec3 SceneGraph::sampleVec3Channel(const tinygltf::AnimationChannel& channel,
                                         int samplerIdx,
                                         float time) {
    if (!m_animation.has_value() || !m_model) {
        return glm::vec3(0.0f);
    }

    const tinygltf::Animation& anim = m_animation.value();
    const tinygltf::AnimationSampler& sampler = anim.samplers[samplerIdx];

    // Get input (time) accessor
    const tinygltf::Accessor& inputAccessor = m_model->accessors[sampler.input];
    const tinygltf::BufferView& inputBufferView = m_model->bufferViews[inputAccessor.bufferView];
    const tinygltf::Buffer& inputBuffer = m_model->buffers[inputBufferView.buffer];

    const float* times = reinterpret_cast<const float*>(
        &inputBuffer.data[inputBufferView.byteOffset + inputAccessor.byteOffset]);

    // Get output (value) accessor
    const tinygltf::Accessor& outputAccessor = m_model->accessors[sampler.output];
    const tinygltf::BufferView& outputBufferView = m_model->bufferViews[outputAccessor.bufferView];
    const tinygltf::Buffer& outputBuffer = m_model->buffers[outputBufferView.buffer];

    const float* values = reinterpret_cast<const float*>(
        &outputBuffer.data[outputBufferView.byteOffset + outputAccessor.byteOffset]);

    // Find keyframes
    size_t count = inputAccessor.count;

    if (count == 0) {
        return glm::vec3(0.0f);
    }

    // Handle time before first keyframe
    if (time <= times[0]) {
        return glm::vec3(values[0], values[1], values[2]);
    }

    // Handle time after last keyframe
    if (time >= times[count - 1]) {
        size_t lastIdx = (count - 1) * 3;
        return glm::vec3(values[lastIdx], values[lastIdx + 1], values[lastIdx + 2]);
    }

    // Find surrounding keyframes
    for (size_t i = 0; i < count - 1; ++i) {
        if (time >= times[i] && time <= times[i + 1]) {
            float t0 = times[i];
            float t1 = times[i + 1];
            float alpha = (time - t0) / (t1 - t0);

            size_t idx0 = i * 3;
            size_t idx1 = (i + 1) * 3;

            glm::vec3 v0(values[idx0], values[idx0 + 1], values[idx0 + 2]);
            glm::vec3 v1(values[idx1], values[idx1 + 1], values[idx1 + 2]);

            // Linear interpolation
            return glm::mix(v0, v1, alpha);
        }
    }

    return glm::vec3(0.0f);
}

glm::quat SceneGraph::sampleQuatChannel(const tinygltf::AnimationChannel& channel,
                                        int samplerIdx,
                                        float time) {
    if (!m_animation.has_value() || !m_model) {
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    const tinygltf::Animation& anim = m_animation.value();
    const tinygltf::AnimationSampler& sampler = anim.samplers[samplerIdx];

    // Get input (time) accessor
    const tinygltf::Accessor& inputAccessor = m_model->accessors[sampler.input];
    const tinygltf::BufferView& inputBufferView = m_model->bufferViews[inputAccessor.bufferView];
    const tinygltf::Buffer& inputBuffer = m_model->buffers[inputBufferView.buffer];

    const float* times = reinterpret_cast<const float*>(
        &inputBuffer.data[inputBufferView.byteOffset + inputAccessor.byteOffset]);

    // Get output (value) accessor
    const tinygltf::Accessor& outputAccessor = m_model->accessors[sampler.output];
    const tinygltf::BufferView& outputBufferView = m_model->bufferViews[outputAccessor.bufferView];
    const tinygltf::Buffer& outputBuffer = m_model->buffers[outputBufferView.buffer];

    const float* values = reinterpret_cast<const float*>(
        &outputBuffer.data[outputBufferView.byteOffset + outputAccessor.byteOffset]);

    // Find keyframes
    size_t count = inputAccessor.count;

    if (count == 0) {
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    // Handle time before first keyframe
    if (time <= times[0]) {
        return glm::quat(values[3], values[0], values[1], values[2]); // glm uses w,x,y,z
    }

    // Handle time after last keyframe
    if (time >= times[count - 1]) {
        size_t lastIdx = (count - 1) * 4;
        return glm::quat(values[lastIdx + 3], values[lastIdx], values[lastIdx + 1], values[lastIdx + 2]);
    }

    // Find surrounding keyframes
    for (size_t i = 0; i < count - 1; ++i) {
        if (time >= times[i] && time <= times[i + 1]) {
            float t0 = times[i];
            float t1 = times[i + 1];
            float alpha = (time - t0) / (t1 - t0);

            size_t idx0 = i * 4;
            size_t idx1 = (i + 1) * 4;

            glm::quat q0(values[idx0 + 3], values[idx0], values[idx0 + 1], values[idx0 + 2]);
            glm::quat q1(values[idx1 + 3], values[idx1], values[idx1 + 1], values[idx1 + 2]);

            // Spherical linear interpolation
            return glm::slerp(q0, q1, alpha);
        }
    }

    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}
