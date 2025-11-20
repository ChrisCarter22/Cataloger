#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cataloger::services::preview {

enum class CacheTier { kRam, kPreload };

struct PreviewDescriptor {
  int root_id{};
  std::optional<std::int64_t> file_id;
  std::filesystem::path absolute_path;
  std::string relative_path;
  std::uintmax_t file_size{};
  std::int64_t capture_ts{};

  [[nodiscard]] std::string cacheKey() const {
    return relative_path + "#" + std::to_string(root_id);
  }
};

struct PreviewImage {
  std::string cache_key;
  std::filesystem::path source_path;
  std::vector<std::uint8_t> pixels;
  bool color_managed{false};
  std::string color_profile;
  int width{};
  int height{};
};

struct CacheEvent {
  int root_id{};
  std::string relative_path;
  CacheTier tier{CacheTier::kRam};
  bool hit{false};
  bool error{false};
  std::string error_message;
  std::string backend;
  double gpu_upload_ms{0.0};
  double color_transform_ms{0.0};
};

enum class PreviewState { kIdle = 0, kCached = 1, kGpuResident = 2 };

}  // namespace cataloger::services::preview
