/*
 * Singleton class desgined to track scenes currently uploaded to S3.
 * Uses vector of strings to track uploaded scenes
 */
#include <string>
#include <vector>

class SceneList {
public:
    static SceneList& getInstance() {
        static SceneList instance;
        return instance;
    }

    SceneList(const SceneList&) = delete;
    SceneList& operator=(const SceneList&) = delete;

    bool addScene(const std::string& sceneUrl);
    bool deleteScene(const std::string& sceneUrl);
    int findSceneByUrl(const std::string& sceneUrl);
    std::vector<std::string> getScenes();

private:
    SceneList() = default;
    ~SceneList() = default;

    std::vector<std::string> scenes;
};
