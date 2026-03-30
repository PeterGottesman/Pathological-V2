#include "renderRequest.hpp"
#include <stdexcept>

RenderRequest::RenderRequest()
    : id(gen()), status(RenderStatus::IN_QUEUE), width(0), height(0),
      framesPerSecond(0), animationRuntime(0), framesCompleted(0),
      executionTime(0), samplesPerPixel(0) {}

RenderRequest::RenderRequest(RenderStatus status, int width, int height,
                             int framesPerSecond, int animationRuntime,
                             int framesCompleted, int executionTime,
                             int samplesPerPixel,
                             const std::string &sceneFileUrl,
                             const std::string &createdAt,
                             const std::string &outputFileName)
    : id(gen()), status(status), width(width), height(height),
      framesPerSecond(framesPerSecond), animationRuntime(animationRuntime),
      sceneFileUrl(sceneFileUrl), executionTime(executionTime),
      createdAt(createdAt), outputFileName(outputFileName),
      samplesPerPixel(samplesPerPixel), downloadLink(std::nullopt) {}

const boost::uuids::uuid &RenderRequest::getId() const { return this->id; }

int RenderRequest::getWidth() const { return this->width; }

int RenderRequest::getHeight() const { return this->height; }

int RenderRequest::getFramesPerSecond() const { return this->framesPerSecond; }

int RenderRequest::getAnimationRuntimeInFrames() const {
  return this->animationRuntime;
}

int RenderRequest::getFramesCompleted() const { return this->framesCompleted; }

const std::string &RenderRequest::getSceneFileUrl() const {
  return this->sceneFileUrl;
}

const std::string &RenderRequest::getCreatedAtTimestamp() const {
  return this->createdAt;
}

int RenderRequest::getExecutionTime() const { return this->executionTime; }

const std::string &RenderRequest::getOutputFileName() const {
  return this->outputFileName;
}

int RenderRequest::getSamplesPerPixel() const { return this->samplesPerPixel; }

const std::optional<std::string> &RenderRequest::getDownloadLink() const {
  return this->downloadLink;
}

RenderRequest &RenderRequest::setStatus(RenderStatus status) {
  this->status = status;
  return *this;
}

RenderRequest &RenderRequest::setWidth(int w) {
  if (w < 0) {
    throw std::invalid_argument("Width must be greater than or equal to zero.");
  }

  this->width = w;
  return *this;
}

RenderRequest &RenderRequest::setHeight(int h) {
  if (h < 0) {
    throw std::invalid_argument(
        "Height must be greater than or equal to zero.");
  }
  this->height = h;
  return *this;
}

RenderRequest &RenderRequest::setFramesPerSecond(int fps) {
  if (fps < 0) {
    throw std::invalid_argument("Frames per second must be greater than 0.");
  }
  this->framesPerSecond = fps;
  return *this;
}

RenderRequest &RenderRequest::setAnimationRuntimeInFrames(int frames) {
  if (frames < 0) {
    throw std::invalid_argument("Frames must be greater than zero.");
  }
  this->animationRuntime = frames; // initially was animationRuntime. 
  return *this;
}

RenderRequest &RenderRequest::setFramesCompleted(int frames) {
  if (frames < 0) {
    throw std::invalid_argument("Frames must be greater than zero.");
  }
  this->framesCompleted = frames;
  return *this;
}

RenderRequest &RenderRequest::setSceneFileUrl(const std::string &url) {
  this->sceneFileUrl = url;
  return *this;
}

RenderRequest &RenderRequest::setCreatedAtTimestamp(const std::string &time) {
  this->createdAt = time;
  return *this;
}

RenderRequest &RenderRequest::setExecutionTime(int seconds) {
  if (seconds < 0) {
    throw std::invalid_argument(
        "Seconds must be greater than or equal to zero.");
  }
  this->executionTime = seconds;
  return *this;
}

RenderRequest &RenderRequest::setOutputFileName(const std::string &name) {
  this->outputFileName = name;
  return *this;
}

RenderRequest &RenderRequest::setSamplesPerPixel(int s) {
  this->samplesPerPixel = s;
  return *this;
}

RenderRequest &
RenderRequest::setDownloadLink(const std::optional<std::string> &link) {
  this->downloadLink = link;
  return *this;
}

Json::Value RenderRequest::toJson() const {
  Json::Value ret;

  ret["id"] = boost::uuids::to_string(this->id);
  ret["status"] = renderStatusToString(this->status);
  ret["width"] = this->width;
  ret["height"] = this->height;
  ret["frames_per_second"] = this->framesPerSecond;
  ret["animation_runtime"] = this->animationRuntime;
  ret["frames_completed"] = this->framesCompleted;
  ret["execution_time"] = this->executionTime;
  ret["samples_per_pixel"] = this->samplesPerPixel;
  ret["created_at"] = this->createdAt;
  ret["scene_file_url"] = this->sceneFileUrl;
  ret["output_filename"] = this->outputFileName;
  if (this->status == RenderStatus::COMPLETED && this->downloadLink) {
    // Uses * to reference value from std::optional
    ret["download_link"] = *this->downloadLink;
  } else {
    ret["download_link"] = Json::nullValue;
  }

  return ret;
}
