#pragma once

#include <filesystem>
#include <vector>

#include "PreviewTypes.h"

namespace cataloger::services::preview {

class DirectoryScanner {
public:
  std::vector<PreviewDescriptor> scan(const std::filesystem::path& root_path) const;
};

}  // namespace cataloger::services::preview
