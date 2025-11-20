#pragma once

#include <string>

namespace cataloger::config {

class Settings {
public:
  Settings();

  void loadDefaults();

  [[nodiscard]] const std::string& activeProfile() const noexcept;

private:
  std::string active_profile_;
};

}  // namespace cataloger::config
