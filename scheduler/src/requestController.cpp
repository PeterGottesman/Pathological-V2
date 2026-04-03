#include <boost/uuid.hpp>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "renderRequest.hpp"
#include "requestController.hpp"
#include "scheduler.hpp"

void RequestController::getStatus(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string id) const {

  boost::uuids::string_generator stringGen;
  boost::uuids::uuid uuid = stringGen(id);

  auto render = RenderHistory::getInstance().getRenderRequest(uuid);

  auto resp = HttpResponse::newHttpJsonResponse(render->toJson());
  callback(resp);
}

// This function accepts a raw JSON input for now.
// Eventually JSON will be replaced with DTO object to validate entry.
void RequestController::createRenderRequest(
    const HttpRequestPtr &req,
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
