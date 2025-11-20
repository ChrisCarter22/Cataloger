#pragma once

#include <string>

#include "PreviewTypes.h"

namespace cataloger::services::preview {

class GpuBridge {
public:
  enum class Backend { kMetal, kVulkan };

  explicit GpuBridge(Backend backend = Backend::kMetal);

  void upload(const PreviewImage& image);
  [[nodiscard]] std::string lastUploadedKey() const;
  [[nodiscard]] Backend backend() const noexcept;

private:
  Backend backend_;
  std::string last_key_;
};

}  // namespace cataloger::services::preview
