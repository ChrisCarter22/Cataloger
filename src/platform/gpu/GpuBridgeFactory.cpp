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

}  // namespace

#if defined(__APPLE__)
std::unique_ptr<GpuBridge> CreateMetalBridge();
#endif

std::unique_ptr<GpuBridge> CreateBridge() {
#if defined(__APPLE__)
  return CreateMetalBridge();
#else
  return std::make_unique<StubBridge>();
#endif
}

}  // namespace cataloger::platform::gpu
