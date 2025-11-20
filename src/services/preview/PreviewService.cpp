#include "PreviewService.h"

namespace cataloger::services::preview {

PreviewService::PreviewService() : cached_neighbors_(0) {}

void PreviewService::primeCaches(std::size_t neighborCount) {
  cached_neighbors_ = neighborCount;
}

std::size_t PreviewService::cachedNeighbors() const noexcept {
  return cached_neighbors_;
}

}  // namespace cataloger::services::preview
