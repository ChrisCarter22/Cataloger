#include "GpuBridge.h"

#include <memory>
#include <string>

namespace cataloger::platform::gpu {

namespace {

class StubBridge : public GpuBridge {
public:
  bool upload(const cataloger::services::preview::PreviewImage&,
              std::string& error) override {
    error = "GPU backend unavailable on this platform.";
    return false;
  }

  Backend backend() const noexcept override {
    return Backend::kStub;
  }

  std::string textureDebugLabel() const override {
    return "none";
  }
};

#if defined(_WIN32) || defined(__linux__)
class VulkanBridge : public GpuBridge {
public:
  bool upload(const cataloger::services::preview::PreviewImage&,
              std::string& error) override {
    error = "Vulkan backend not implemented yet.";
    return false;
  }

  Backend backend() const noexcept override {
    return Backend::kVulkan;
  }

  std::string textureDebugLabel() const override {
    return "not-available";
  }
};
#endif

}  // namespace

#if defined(__APPLE__)
std::unique_ptr<GpuBridge> CreateMetalBridge();
#endif

std::unique_ptr<GpuBridge> CreateBridge() {
#if defined(__APPLE__)
  return CreateMetalBridge();
#elif defined(_WIN32) || defined(__linux__)
  return std::make_unique<VulkanBridge>();
#else
  return std::make_unique<StubBridge>();
#endif
}

}  // namespace cataloger::platform::gpu
