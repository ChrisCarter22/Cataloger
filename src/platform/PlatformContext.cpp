#include "PlatformContext.h"

namespace cataloger::platform {

namespace {

HostPlatform detectPlatform() {
#if defined(__APPLE__)
  return HostPlatform::kMacOS;
#elif defined(_WIN32)
  return HostPlatform::kWindows;
#elif defined(__linux__)
  return HostPlatform::kLinux;
#else
  return HostPlatform::kUnknown;
#endif
}

}  // namespace

PlatformContext::PlatformContext() : platform_(HostPlatform::kUnknown) {}

void PlatformContext::detectHostEnvironment() {
  platform_ = detectPlatform();
}

HostPlatform PlatformContext::hostPlatform() const noexcept {
  return platform_;
}

std::string PlatformContext::displayName() const {
  switch (platform_) {
    case HostPlatform::kMacOS:
      return "macOS";
    case HostPlatform::kWindows:
      return "Windows";
    case HostPlatform::kLinux:
      return "Linux";
    default:
      return "Unknown";
  }
}

}  // namespace cataloger::platform
