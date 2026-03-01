#include "requestController.hpp"
#include "renderRequest.hpp"

void RequestController::getStatus(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, int id) const {
  // Search scheduler queue/s3 bucket by ID
  // Return
  // If ID exists return json else 404
  RenderRequest render;

  render.setId(1234567)
      .setStatus(RenderStatus::IN_QUEUE)
      .setWidth(1024)
      .setHeight(1024)
      .setFramesPerSecond(1)
      .setAnimationRuntimeInFrames(1)
      .setFramesCompleted(0)
      .setExecutionTime(0)
      .setSamplesPerPixel(128)
      .setSceneFileUrl("https://example.com/scene.gltf")
      .setCreatedAtTimestamp("2026-02-28T00:00:00Z")
      .setOutputFileName("output.png");

  auto ret = render.toJson();

  auto resp = HttpResponse::newHttpJsonResponse(ret);
  callback(resp);
}

// This function accepts a raw JSON input for now.
// Eventually JSON will be replaced with DTO object to validate entry.
void RequestController::createRenderRequest(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    Json::Value &&payload) {
  try {
    RenderRequest render;
    render.setId(payload["id"].asInt64())
        .setWidth(payload["width"].asInt())
        .setHeight(payload["height"].asInt())
        .setFramesPerSecond(payload["frames_per_second"].asInt())
        .setAnimationRuntimeInFrames(payload["animation_runtime"].asInt())
        .setSamplesPerPixel(payload["samples_per_pixel"].asInt())
        .setCreatedAtTimestamp("2026-02-28T00:00:00Z")
        .setSceneFileUrl(payload["scene_file_url"].asString())
        .setOutputFileName(payload["output_file_name"].asString());

    auto resp = HttpResponse::newHttpJsonResponse(render.toJson());
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