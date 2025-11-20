#include "PreviewSubscriber.h"

#include <iostream>

namespace cataloger::ui::mock {

PreviewSubscriber::PreviewSubscriber(std::string name)
    : name_(std::move(name)) {}

void PreviewSubscriber::onCacheEvent(
    const cataloger::services::preview::CacheEvent& event) {
  std::lock_guard lock(mutex_);
  events_.push_back(event);
  std::cout << "[MockUI:" << name_ << "] "
            << event.relative_path << " hit=" << event.hit
            << " backend=" << event.backend
            << " gpu=" << event.gpu_upload_ms << "ms"
            << (event.error ? " ERROR" : "") << std::endl;
}

void PreviewSubscriber::setNeighborWindow(std::size_t window) {
  neighbor_window_ = window;
}

std::size_t PreviewSubscriber::totalEvents() const {
  std::lock_guard lock(mutex_);
  return events_.size();
}

std::size_t PreviewSubscriber::errorEvents() const {
  std::lock_guard lock(mutex_);
  std::size_t errors = 0;
  for (const auto& event : events_) {
    if (event.error) {
      ++errors;
    }
  }
  return errors;
}

std::vector<std::string> PreviewSubscriber::recentItems() const {
  std::lock_guard lock(mutex_);
  std::vector<std::string> items;
  const auto start = events_.size() > neighbor_window_
                         ? events_.end() - neighbor_window_
                         : events_.begin();
  for (auto it = start; it != events_.end(); ++it) {
    items.push_back(it->relative_path);
  }
  return items;
}

void MockNavigator::bind(PreviewSubscriber* subscriber) {
  subscriber_ = subscriber;
}

void MockNavigator::handleEvent(
    const cataloger::services::preview::CacheEvent& event) {
  if (subscriber_) {
    subscriber_->onCacheEvent(event);
  }
}

}  // namespace cataloger::ui::mock
