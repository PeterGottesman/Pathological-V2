/*
 * Singleton class desgined to track all renders for status updates
 * Holds lightweight unordered map to track the status of all renders in the
 * system. Could implement a datbase later on for more persistent data tracking.
 */
#include "renderRequest.hpp"
#include "renderStatus.hpp"
#include <boost/uuid.hpp>
#include <unordered_map>

class RenderHistory {
public:
  RenderHistory();
  ~RenderHistory();

  bool addRender(std::shared_ptr<RenderRequest> req);
  bool deleteRender(const boost::uuids::uuid &id);
  bool updateStatus(const boost::uuids::uuid &id, RenderStatus status);
  RenderStatus getStatus(const boost::uuids::uuid &id);

private:
  std::unordered_map<boost::uuids::uuid, std::shared_ptr<RenderRequest>>
      history;
};