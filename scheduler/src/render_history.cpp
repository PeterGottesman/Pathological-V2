#include "render_history.hpp"

bool RenderHistory::addRender(std::shared_ptr<RenderRequest> req) {
  if (!req)
    return false;

  this->history.emplace(req->getId(), req);
  return true;
}

bool RenderHistory::deleteRender(const boost::uuids::uuid &id) {
  return this->history.erase(id) > 0;
}

bool RenderHistory::updateStatus(const boost::uuids::uuid &id,
                                 RenderStatus status) {
  auto req = history.find(id);
  if (req == history.end())
    return false;

  req->second->setStatus(status);
  return true;
}

std::shared_ptr<RenderRequest>
RenderHistory::getRenderRequest(const boost::uuids::uuid &id) const {
  auto req = history.find(id);
  if (req == history.end())
    return nullptr;

  return req->second;
}
