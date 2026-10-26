/*
 * Singleton class desgined to track scenes currently uploaded to S3.
 * Uses unordered_map of strings to track uploaded scenes
 */
#include <json/json.h>
#include <string>
#include <unordered_map>

class SceneList {
public:
    static SceneList& getInstance() {
        static SceneList instance;
        return instance;
    }

    SceneList(const SceneList&) = delete;
    SceneList& operator=(const SceneList&) = delete;

    bool addScene(const std::string& sceneName, const std::string& sceneFileName);
    bool deleteScene(const std::string& sceneName);
    std::string findSceneByKey(const std::string& sceneName);
    Json::Value scenesToJson();

private:
    SceneList() = default;
    ~SceneList() = default;

    std::unordered_map<std::string, std::string> scenes;
};
