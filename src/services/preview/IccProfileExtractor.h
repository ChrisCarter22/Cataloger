#pragma once

#include <filesystem>
#include <vector>

namespace cataloger::services::preview::icc {

// Extracts embedded ICC profile bytes from the given image (JPEG/RAW).
// Returns an empty vector if no profile is detected.
std::vector<std::uint8_t> ExtractEmbeddedProfile(
    const std::filesystem::path& image_path);

}  // namespace cataloger::services::preview::icc
