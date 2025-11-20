#pragma once

#include <memory>
#include <string>

#include "services/preview/PreviewTypes.h"

namespace cataloger::platform::gpu {

enum class Backend { kMetal, kVulkan, kStub };

class GpuBridge {
public:
  virtual ~GpuBridge() = default;
  virtual bool upload(const cataloger::services::preview::PreviewImage& image,
                      std::string& error) = 0;
  [[nodiscard]] virtual Backend backend() const noexcept = 0;
  [[nodiscard]] virtual std::string textureDebugLabel() const { return {}; }
};

std::unique_ptr<GpuBridge> CreateBridge();

}  // namespace cataloger::platform::gpu
