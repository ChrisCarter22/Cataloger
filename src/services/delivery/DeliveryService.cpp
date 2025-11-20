#include "DeliveryService.h"

namespace cataloger::services::delivery {

void DeliveryService::configureEndpoint(std::string endpoint) {
  endpoint_ = std::move(endpoint);
}

const std::string& DeliveryService::endpoint() const noexcept {
  return endpoint_;
}

}  // namespace cataloger::services::delivery
