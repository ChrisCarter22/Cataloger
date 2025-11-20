#include "DirectoryScanner.h"

#include <chrono>
#include <stdexcept>

namespace cataloger::services::preview {

std::vector<PreviewDescriptor> DirectoryScanner::scan(
    const std::filesystem::path& root_path) const {
  if (!std::filesystem::exists(root_path)) {
    throw std::runtime_error("Preview root missing: " + root_path.string());
  }

  std::vector<PreviewDescriptor> descriptors;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(root_path)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    PreviewDescriptor descriptor;
    descriptor.absolute_path = entry.path();
    descriptor.relative_path =
        std::filesystem::relative(entry.path(), root_path).generic_string();
    descriptor.file_size = entry.file_size();
    std::error_code ec;
    const auto ts = std::filesystem::last_write_time(entry.path(), ec);
    if (!ec) {
      descriptor.capture_ts = std::chrono::duration_cast<std::chrono::seconds>(
                                  ts.time_since_epoch())
                                  .count();
    }
    descriptors.push_back(std::move(descriptor));
  }
  return descriptors;
}

}  // namespace cataloger::services::preview
