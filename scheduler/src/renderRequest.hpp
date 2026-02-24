#pragma once

#include <json/json.h>
#include <optional>
#include <string>

#include "renderStatus.hpp"

class RenderRequest {
public:
  RenderRequest();

  RenderRequest(long long id, RenderStatus status, int width, int height,
                int framesPerSecond, int animationRuntime, int framesCompleted,
                int executionTime, int samplesPerPixel,
                const std::string &sceneFileUrl, const std::string &createdAt,
                const std::string &outputFileName);

  long long getId() const;
  int getWidth() const;
  int getHeight() const;
  int getFramesPerSecond() const;
  int getAnimationRuntimeInFrames() const;
  int getFramesCompleted() const;
  int getExecutionTime() const;
  int getSamplesPerPixel() const;
  const std::string &getSceneFileUrl() const;
  const std::string &getCreatedAtTimestamp() const;
  const std::string &getOutputFileName() const;
  const std::optional<std::string> &getDownloadLink() const;

  RenderRequest &setId(long long id);
  RenderRequest &setStatus(RenderStatus status);
  RenderRequest &setWidth(int w);
  RenderRequest &setHeight(int h);
  RenderRequest &setFramesPerSecond(int fps);
  RenderRequest &setAnimationRuntimeInFrames(int frames);
  RenderRequest &setFramesCompleted(int frames);
  RenderRequest &setExecutionTime(int seconds);
  RenderRequest &setSamplesPerPixel(int s);
  RenderRequest &setSceneFileUrl(const std::string &url);
  RenderRequest &setCreatedAtTimestamp(const std::string &time);
  RenderRequest &setOutputFileName(const std::string &name);
  RenderRequest &setDownloadLink(const std::optional<std::string> &link);

  Json::Value toJson() const;

private:
  long long id;
  RenderStatus status;
  int width;
  int height;
  int framesPerSecond;
  int animationRuntime;
  int framesCompleted;
  int executionTime;
  int samplesPerPixel;
  std::string sceneFileUrl;
  std::string createdAt;
  std::string outputFileName;
  std::optional<std::string> downloadLink;
};