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

// (#86): Stub for now
void RequestController::listRenders(const HttpRequestPtr &req,
                                    std::function<void(const HttpResponsePtr &)> &&callback) {
    Json::Value jobs(Json::arrayValue);
    // (#86): Returns every render in RenderHistory as a JSON array.
    for (const auto &render : RenderHistory::getInstance().getAllRenders()) {
        jobs.append(render->toJson());
    }
    callback(HttpResponse::newHttpJsonResponse(jobs));
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
        std::string sceneFileName = (*payload)["scene_file_name"].asString();
        std::string sceneName = (*payload)["scene_name"].asString();

        // Ensures that the scene name translates into a proper gltf file for uploading
        std::string sceneNameS3 = sceneName;
        if (sceneName.empty()) {
            sceneNameS3 = sceneFileName;
        } else {
            std::replace(sceneNameS3.begin(), sceneNameS3.end(), ' ', '_');
            std::replace(sceneNameS3.begin(), sceneNameS3.end(), '/', '-');
            sceneNameS3.erase(std::remove(sceneNameS3.begin(), sceneNameS3.end(), '.'), sceneNameS3.end());
            sceneNameS3.append(".gltf");
        }

        if (SceneList::getInstance().findSceneByKey(sceneName).empty() == false) {
            throw std::runtime_error("A scene with this name already exists");
        }

        std::stringstream ss(sceneData);
        std::streambuf *buf = ss.rdbuf();

        bool uploaded = Scheduler::getInstance().uploadScene(buf, sceneNameS3);

        if (!uploaded) {
            throw std::runtime_error("Failed to upload to S3 bucket");
        }

        bool listUpdated = SceneList::getInstance().addScene(sceneName, sceneNameS3);

        if (!listUpdated) {
            throw std::runtime_error("Internal scene list failed to update");
        }

        Json::Value scenesResp = SceneList::getInstance().scenesToJson();

        auto resp = HttpResponse::newHttpJsonResponse(scenesResp);
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

void RequestController::getSceneList(const HttpRequestPtr &req,
                                     std::function<void(const HttpResponsePtr &)> &&callback) {
    auto scenes = SceneList::getInstance().scenesToJson();

    auto resp = HttpResponse::newHttpJsonResponse(scenes);
    callback(resp);
}