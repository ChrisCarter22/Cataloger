#include "TaskScheduler.h"

namespace cataloger::services::tasks {

void TaskScheduler::schedule(std::string taskName) {
  tasks_.push(std::move(taskName));
}

bool TaskScheduler::empty() const noexcept {
  return tasks_.empty();
}

std::size_t TaskScheduler::size() const noexcept {
  return tasks_.size();
}

}  // namespace cataloger::services::tasks
