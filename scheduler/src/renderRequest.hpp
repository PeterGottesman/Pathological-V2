#pragma once

#include <string>
#include <optional>
#include <json/json.h>

#include "renderStatus.hpp"

class RenderRequest
{
public:
    RenderRequest();
    RenderRequest(long long id,
                  RenderStatus status,
                  int width,
                  int height,
                  int frameCount,
                  const std::string &uploadFileUrl,
                  const std::string &createdAt,
                  int executionTime,
                  const std::string &outputFileName,
                  int samplesPerPixel);

    long long getId() const;
    void setId(long long id);

    int getWidth() const;
    void setWidth(int w);

    int getHeight() const;
    void setHeight(int h);

    int getFrameCount() const;
    void setFrameCount(int fc);

    const std::string &getGLTFFileUrl() const;
    void setFileUrl(const std::string &url);

    const std::string &getCreatedAtTimestamp() const;
    void setCreatedAtTimestamp(const std::string &time);

    int getExecutionTime() const;
    void setExecutionTime(int seconds);

    const std::string &getOutputFileName() const;
    void setOutputFileName(const std::string &name);

    int getSamplesPerPixel() const;
    void setSamplesPerPixel(int s);

    const std::optional<std::string> &getDownloadLink() const;
    void setDownloadLink(const std::optional<std::string> &link);

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
    std::string gltfFileUrl;
    std::string createdAt;
    std::string outputFileName;
    std::optional<std::string> downloadLink;
};