#include "scene_list.hpp"

bool SceneList::addScene(const std::string& sceneName, const std::string& sceneFileName) {
    if (sceneName.empty() || sceneFileName.empty()) {
        return false;
    }
    this->scenes.emplace(sceneName, sceneFileName);
    return true;
}

bool SceneList::deleteScene(const std::string& sceneName) { return this->scenes.erase(sceneName) > 0; }

std::string SceneList::findSceneByKey(const std::string& sceneName) {
    auto req = scenes.find(sceneName);
    if (req == scenes.end()) {
        return {};
    }
    return req->second;
}

Json::Value SceneList::scenesToJson() {
    Json::Value scenes_json;
    for (const auto& [key, value] : scenes) {
        scenes_json[key] = value;
    }
    return scenes_json;
};
