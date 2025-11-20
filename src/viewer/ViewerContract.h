#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace cataloger::viewer {

enum class Backend { kMetal, kVulkan, kStub };

struct TextureHandle {
  Backend backend{Backend::kStub};
  void* native_handle{nullptr};
};

struct FrameContext {
  std::uint32_t width{0};
  std::uint32_t height{0};
  double scale{1.0};  // HiDPI scale factor
  bool vsync{true};
  // Optional color space tag (e.g., "sRGB IEC61966-2.1", "Display P3").
  std::string color_space;
  TextureHandle texture;
};

class Viewer {
public:
  virtual ~Viewer() = default;

  // Called before rendering each frame. Returns false on failure.
  virtual bool beginFrame(FrameContext& context) = 0;

  // Records copy/blit/compute work to present the provided texture.
  virtual bool submitFrame(const FrameContext& context, std::string& error) = 0;

  // Presents the drawable (vsync-aware). Returns false if presentation fails.
  virtual bool present(std::string& error) = 0;
};

#if defined(__APPLE__)
std::unique_ptr<Viewer> CreateMetalViewer();
#endif

std::unique_ptr<Viewer> CreateVulkanViewerStub();

}  // namespace cataloger::viewer
