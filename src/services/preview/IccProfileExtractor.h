#pragma once

#include <filesystem>
#include <vector>

namespace cataloger::services::preview::icc {

// Attempts to extract an embedded ICC profile from a JPEG/RAW file.
// Returns the ICC bytes when found; otherwise an empty vector.
std::vector<std::uint8_t> ExtractEmbeddedProfile(
    const std::filesystem::path& image_path);

}  // namespace cataloger::services::preview::icc
