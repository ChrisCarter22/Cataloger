#include "viewer/ViewerContract.h"

#include <iostream>

namespace cataloger::viewer {

class VulkanViewerStub : public Viewer {
public:
  bool beginFrame(FrameContext&) override {
    std::cerr << "[VulkanViewer] beginFrame stub" << std::endl;
    return true;
  }

  bool submitFrame(const FrameContext&, std::string& error) override {
    error = "Vulkan viewer not implemented";
    return false;
  }

  bool present(std::string& error) override {
    error = "Vulkan viewer not implemented";
    return false;
  }
};

std::unique_ptr<Viewer> CreateVulkanViewerStub() {
  return std::make_unique<VulkanViewerStub>();
}

}  // namespace cataloger::viewer
