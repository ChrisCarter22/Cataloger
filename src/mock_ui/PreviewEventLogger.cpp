#include "PreviewEventLogger.h"

#include <iostream>

namespace cataloger::mock_ui {

void PreviewEventLogger::handle(
    const cataloger::services::preview::CacheEvent& event) {
  std::lock_guard lock(mutex_);
  events_.push_back(event);
  std::cout << "[MockUI] " << event.relative_path << " hit=" << event.hit
            << " backend=" << event.backend;
  if (event.error) {
    std::cout << " ERROR=" << event.error_message;
  }
  if (event.gpu_upload_ms > 0) {
    std::cout << " gpu=" << event.gpu_upload_ms << "ms";
  }
  std::cout << std::endl;
}

void PreviewEventLogger::renderSummary() const {
  std::lock_guard lock(mutex_);
  std::size_t hits = 0;
  std::size_t misses = 0;
  std::size_t errors = 0;
  double gpu_total = 0;
  for (const auto& event : events_) {
    if (event.hit) {
      ++hits;
    } else {
      ++misses;
    }
    if (event.error) {
      ++errors;
    }
    gpu_total += event.gpu_upload_ms;
  }
  const auto total = events_.size();
  std::cout << "[MockUI] Summary hits=" << hits << " misses=" << misses
            << " errors=" << errors;
  if (total > 0) {
    std::cout << " avg_gpu=" << (gpu_total / static_cast<double>(total)) << "ms";
  }
  std::cout << std::endl;
}

std::size_t PreviewEventLogger::errorCount() const {
  std::lock_guard lock(mutex_);
  std::size_t errors = 0;
  for (const auto& event : events_) {
    if (event.error) {
      ++errors;
    }
  }
  return errors;
}

}  // namespace cataloger::mock_ui
