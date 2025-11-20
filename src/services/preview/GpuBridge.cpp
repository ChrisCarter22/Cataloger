#include "GpuBridge.h"

namespace cataloger::services::preview {

GpuBridge::GpuBridge(Backend backend) : backend_(backend) {}

void GpuBridge::upload(const PreviewImage& image) {
  last_key_ = image.cache_key;
}

std::string GpuBridge::lastUploadedKey() const {
  return last_key_;
}

GpuBridge::Backend GpuBridge::backend() const noexcept {
  return backend_;
}

}  // namespace cataloger::services::preview
