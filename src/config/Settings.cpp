#include "Settings.h"

namespace cataloger::config {

Settings::Settings() = default;

void Settings::loadDefaults() {
  active_profile_ = "development";
}

const std::string& Settings::activeProfile() const noexcept {
  return active_profile_;
}

}  // namespace cataloger::config
