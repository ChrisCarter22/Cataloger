#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "services/preview/PreviewTypes.h"

namespace cataloger::ui::mock {

class PreviewSubscriber {
public:
  explicit PreviewSubscriber(std::string name);

  void onCacheEvent(const cataloger::services::preview::CacheEvent& event);
  void setNeighborWindow(std::size_t window);

  [[nodiscard]] std::size_t totalEvents() const;
  [[nodiscard]] std::size_t errorEvents() const;
  [[nodiscard]] std::vector<std::string> recentItems() const;

private:
  std::string name_;
  std::size_t neighbor_window_{2};
  mutable std::mutex mutex_;
  std::vector<cataloger::services::preview::CacheEvent> events_;
};

class MockNavigator {
public:
  void bind(PreviewSubscriber* subscriber);
  void handleEvent(const cataloger::services::preview::CacheEvent& event);

private:
  PreviewSubscriber* subscriber_{nullptr};
};

}  // namespace cataloger::ui::mock
