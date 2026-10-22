#include "requestController.hpp"

#include <algorithm>
#include <boost/uuid.hpp>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "renderRequest.hpp"
#include "scene_list.hpp"
#include "scheduler.hpp"

void RequestController::getStatus(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback,
                                  const std::string &id) const {
    boost::uuids::string_generator stringGen;
    boost::uuids::uuid uuid = stringGen(id);

    auto render = RenderHistory::getInstance().getRenderRequest(uuid);

    auto resp = HttpResponse::newHttpJsonResponse(render->toJson());
    callback(resp);
}

// This function accepts a raw JSON input for now.
// Eventually JSON will be replaced with DTO object to validate entry.
void RequestController::createRenderRequest(const HttpRequestPtr &req,
                                            std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto payload = req->getJsonObject();
        if (!payload) {
            Json::Value err;
            err["error"] = req->getJsonError();
            auto resp = HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        auto utc_now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(utc_now);
        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
        auto timestamp = oss.str();

        auto render = std::make_shared<RenderRequest>();
        render->setWidth((*payload)["width"].asInt())
            .setHeight((*payload)["height"].asInt())
            .setStatus(RenderStatus::IN_QUEUE)
            .setFramesPerSecond((*payload)["frames_per_second"].asInt())
            .setAnimationRuntimeInFrames((*payload)["animation_runtime"].asInt())
            .setSamplesPerPixel((*payload)["samples_per_pixel"].asInt())
            .setCreatedAtTimestamp(timestamp)
            .setSceneFileUrl((*payload)["scene_file_url"].asString())
            .setOutputFileName((*payload)["output_filename"].asString());

        RenderHistory::getInstance().addRender(render);

        Scheduler::getInstance().addJob(render);

        auto resp = HttpResponse::newHttpJsonResponse(render->toJson());
        resp->setStatusCode(k201Created);
        callback(resp);
    } catch (const std::exception &e) {
        Json::Value error;
        error["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
    }
}

// This API endpoint is a stub, currently it only returns a dummy response to the frontend.
void RequestController::uploadSceneFile(const HttpRequestPtr &req,
                                        std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto payload = req->getJsonObject();
        if (!payload) {
            throw std::runtime_error("Bad/malformed payload");
        }

        std::string sceneData = (*payload)["scene_data"].asString();
        std::string sceneUrl = (*payload)["scene_file_url"].asString();
        std::string sceneName = (*payload)["scene_name"].asString();

        // Ensures that the scene name translates into a proper gltf file for uploading
        if (sceneName.empty()) {
            sceneName = sceneUrl;
        } else {
            std::replace(sceneName.begin(), sceneName.end(), ' ', '_');
            std::replace(sceneName.begin(), sceneName.end(), '/', '-');
            sceneName.erase(std::remove(sceneName.begin(), sceneName.end(), '.'), sceneName.end());
            sceneName.append(".gltf");
        }

        if (SceneList::getInstance().findSceneByUrl(sceneName) >= 0) {
            throw std::runtime_error("A scene with this name already exists");
        }

        std::stringstream ss(sceneData);
        std::streambuf *buf = ss.rdbuf();

        bool uploaded = Scheduler::getInstance().uploadScene(buf, sceneName);

        if (!uploaded) {
            throw std::runtime_error("Failed to upload to S3 bucket");
        }

        bool listUpdated = SceneList::getInstance().addScene(sceneName);

        if (!listUpdated) {
            throw std::runtime_error("Internal scene list failed to update");
        }

        std::vector scenes = SceneList::getInstance().getScenes();
        std::string scenesStr;

        for (int i = 0; i < scenes.size(); i++) {
            if (i != scenes.size() - 1) {
                scenesStr.append(scenes[i]);
                scenesStr.append(", ");
            } else {
                scenesStr.append(scenes[i]);
            }
        }
        Json::Value successResp;
        successResp["scenes"] = scenesStr;

        auto resp = HttpResponse::newHttpJsonResponse(successResp);
        resp->setStatusCode(k200OK);
        callback(resp);

    } catch (const std::exception &e) {
        Json::Value error;
        error["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
    }
}