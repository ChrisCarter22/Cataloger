#pragma once

#include <cstddef>
#include <queue>
#include <string>

namespace cataloger::services::tasks {

class TaskScheduler {
public:
  void schedule(std::string taskName);
  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;

private:
  std::queue<std::string> tasks_;
};

}  // namespace cataloger::services::tasks
