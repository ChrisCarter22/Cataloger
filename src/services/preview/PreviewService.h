#pragma once

#include <cstddef>

namespace cataloger::services::preview {

class PreviewService {
public:
  PreviewService();

  void primeCaches(std::size_t neighborCount);
  [[nodiscard]] std::size_t cachedNeighbors() const noexcept;

private:
  std::size_t cached_neighbors_;
};

}  // namespace cataloger::services::preview
