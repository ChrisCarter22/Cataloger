#pragma once

#include "config/Settings.h"
#include "platform/PlatformContext.h"

namespace cataloger::app {

class Application {
public:
  Application();

  int run();

private:
  void bootstrap();

  config::Settings settings_;
  platform::PlatformContext platform_;
};

}  // namespace cataloger::app
