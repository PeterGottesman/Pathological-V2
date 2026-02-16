#include "renderRequest.hpp"
#include <stdexcept>

RenderRequest::RenderRequest()
    : id(0), status(RenderStatus::UNKNOWN), width(0), height(0),
      frameCount(0), executionTime(0), samplesPerPixel(0) {}

RenderRequest::RenderRequest(long long id,
                             RenderStatus status,
                             int width,
                             int height,
                             int frameCount,
                             const std::string &uploadFileUrl,
                             const std::string &createdAt,
                             int executionTime,
                             const std::string &outputFileName,
                             int samplesPerPixel)
    : id(id), status(status), width(width), height(height), frameCount(frameCount),
      gltfFileUrl(uploadFileUrl), createdAt(createdAt), executionTime(executionTime),
      outputFileName(outputFileName), samplesPerPixel(samplesPerPixel), downloadLink(std::nullopt) {}

long long RenderRequest::getId() const { return this->id; }

void RenderRequest::setId(long long id)
{
    if (id < 0)
    {
        throw std::invalid_argument("ID value must be set to nonnegative integer.");
    }

    this->id = id;
}

int RenderRequest::getWidth() const { return this->width; }

void RenderRequest::setWidth(int w)
{
    if (w < 0)
    {
        throw std::invalid_argument("Width must be greater than or equal to zero.");
    }

    this->width = w;
}

int RenderRequest::getHeight() const { return this->height; }

void RenderRequest::setHeight(int h)
{
    if (h < 0)
    {
        throw std::invalid_argument("Height must be greater than or equal to zero.");
    }
    this->height = h;
}

int RenderRequest::getFrameCount() const { return this->frameCount; }
void RenderRequest::setFrameCount(int fc)
{
    if (fc < 0)
    {
        throw std::invalid_argument("Frame count must be greater than or equal to zero.");
    }

    this->frameCount = fc;
}

const std::string &RenderRequest::getGLTFFileUrl() const { return this->gltfFileUrl; }

void RenderRequest::setFileUrl(const std::string &url)
{
    this->gltfFileUrl = url;
}

const std::string &RenderRequest::getCreatedAtTimestamp() const { return this->createdAt; }

void RenderRequest::setCreatedAtTimestamp(const std::string &time)
{
    this->createdAt = time;
}

int RenderRequest::getExecutionTime() const { return this->executionTime; }
void RenderRequest::setExecutionTime(int seconds)
{
    if (seconds < 0)
    {
        throw std::invalid_argument("Seconds must be greater than or equal to zero.");
    }
    this->executionTime = seconds;
}

const std::string &RenderRequest::getOutputFileName() const { return this->outputFileName; }
void RenderRequest::setOutputFileName(const std::string &name)
{
    this->outputFileName = name;
}

int RenderRequest::getSamplesPerPixel() const { return this->samplesPerPixel; }
void RenderRequest::setSamplesPerPixel(int s)
{
    this->samplesPerPixel = s;
}

const std::optional<std::string> &RenderRequest::getDownloadLink() const { return this->downloadLink; }
void RenderRequest::setDownloadLink(const std::optional<std::string> &link)
{
    this->downloadLink = link;
}

Json::Value RenderRequest::toJson() const
{
    Json::Value ret;

    ret["id"] = (Json::Int64)this->id;
    ret["status"] = renderStatusToString(this->status);
    ret["width"] = this->width;
    ret["height"] = this->height;
    ret["frame_count"] = this->frameCount;
    ret["created_at"] = this->createdAt;
    ret["execution_time"] = this->executionTime;
    ret["output_filename"] = this->outputFileName;
    ret["samples_per_pixel"] = this->samplesPerPixel;
    if (this->status == RenderStatus::COMPLETED && this->downloadLink)
    {
        // Uses * to reference value from std::optional
        ret["download_link"] = *this->downloadLink;
    }
    else
    {
        ret["download_link"] = Json::nullValue;
    }

    return ret;
}