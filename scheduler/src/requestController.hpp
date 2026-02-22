#pragma once

#include <drogon/HttpController.h>
#include <optional>

#include "renderRequest.hpp"

using namespace drogon;

// Maybe frame render and animation render?

class RequestController : public drogon::HttpController<RequestController>
{
public:
    METHOD_LIST_BEGIN
    // Method sends status of a render back to the client
    ADD_METHOD_TO(RequestController::getStatus, "/renders/{1}/status", Get); // Path is /renders/{id}/status
    // Method receives render from the client and creates a render request.
    ADD_METHOD_TO(RequestController::createRenderRequest, "/renders", Post);
    METHOD_LIST_END

    void getStatus(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   int id) const;

    void createRenderRequest(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback,
                             RenderRequest::RenderRequest &&pRequest);
};