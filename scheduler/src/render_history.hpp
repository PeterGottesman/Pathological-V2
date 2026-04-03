/*
 * Singleton class desgined to track all renders for status updates
 * Holds unordered map to track the status of all renders in the
 * system. Could implement a datbase later on for more persistent data tracking.
 */
#include "renderRequest.hpp"
#include "renderStatus.hpp"
#include <boost/uuid.hpp>
#include <optional>
#include <unordered_map>

class RenderHistory {
public:
  static RenderHistory &getInstance() {
    static RenderHistory instance;
    return instance;
  }

  RenderHistory(const RenderHistory &) = delete;
  RenderHistory &operator=(const RenderHistory &) = delete;

  bool addRender(std::shared_ptr<RenderRequest> req);
  bool deleteRender(const boost::uuids::uuid &id);
  bool updateStatus(const boost::uuids::uuid &id, RenderStatus status);
  std::shared_ptr<RenderRequest>
  getRenderRequest(const boost::uuids::uuid &id) const;

private:
  RenderHistory() = default;
  ~RenderHistory() = default;

  std::unordered_map<boost::uuids::uuid, std::shared_ptr<RenderRequest>>
      history;
};
