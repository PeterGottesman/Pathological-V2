#include "scene_list.hpp"

bool SceneList::addScene(const std::string& sceneUrl) {
    if (sceneUrl.empty()) {
        return false;
    }
    this->scenes.push_back(sceneUrl);
    return true;
}

bool SceneList::deleteScene(const std::string& sceneUrl) {
    if (sceneUrl.empty()) {
        return false;
    }
    int sceneID = findSceneByUrl(sceneUrl);
    if (sceneID < 0) {
        return false;
    }
    this->scenes.erase(scenes.begin() + sceneID + 1);
    return true;
}

int SceneList::findSceneByUrl(const std::string& sceneUrl) {
    for (int i = 0; i < this->scenes.size(); i++) {
        if (this->scenes[i] == sceneUrl) {
            return i;
        }
    }
    return -1;
}

std::vector<std::string> SceneList::getScenes() { return this->scenes; }
