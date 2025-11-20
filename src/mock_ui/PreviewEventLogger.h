#pragma once

#include <mutex>
#include <vector>

#include "services/preview/PreviewTypes.h"

namespace cataloger::mock_ui {

class PreviewEventLogger {
public:
  void handle(const cataloger::services::preview::CacheEvent& event);
  void renderSummary() const;
  [[nodiscard]] std::size_t total() const;

private:
  mutable std::mutex mutex_;
  std::vector<cataloger::services::preview::CacheEvent> events_;
};

}  // namespace cataloger::mock_ui
