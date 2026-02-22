#include "requestController.hpp"
#include "renderRequest.hpp"

void RequestController::getStatus(const HttpRequestPtr &req,
                                  std::function<void(const HttpResponsePtr &)> &&callback,
                                  int id) const
{
    // Search scheduler queue/s3 bucket by ID
    // Return
    // If ID exists return json else 404
    RenderRequest render; // Default placeholder
    auto ret = render.toJson();

    auto resp = HttpResponse::newHttpJsonReponse(ret);
    callback(resp);
}

void RequestController::createRenderRequest(const HttpRequestPtr &req,
                                            std::function<void(const HttpResponsePtr &)> &&callback,
                                            RenderRequest::RenderRequest &&pRequest)
{
    try
    {
    }
    catch (const std::exception &e)
    {
        Json::Value error;
        error["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k404InternalServerError);
        callback(resp);
    }
}