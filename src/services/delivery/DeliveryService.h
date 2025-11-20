#pragma once

#include <string>

namespace cataloger::services::delivery {

class DeliveryService {
public:
  void configureEndpoint(std::string endpoint);
  [[nodiscard]] const std::string& endpoint() const noexcept;

private:
  std::string endpoint_;
};

}  // namespace cataloger::services::delivery
