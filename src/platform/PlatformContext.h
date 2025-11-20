#pragma once

#include <string>

namespace cataloger::platform {

enum class HostPlatform { kUnknown, kMacOS, kWindows, kLinux };

class PlatformContext {
public:
  PlatformContext();

  void detectHostEnvironment();

  [[nodiscard]] HostPlatform hostPlatform() const noexcept;
  [[nodiscard]] std::string displayName() const;

private:
  HostPlatform platform_;
};

}  // namespace cataloger::platform
