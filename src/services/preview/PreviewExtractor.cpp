#include "PreviewExtractor.h"

#include <algorithm>
#include <fstream>

namespace cataloger::services::preview {

namespace {
constexpr std::size_t kMaxReadBytes = 256 * 1024;

int pseudoDimension(std::uintmax_t file_size, int base) {
  const auto span = static_cast<int>(file_size % 2048);
  return std::max(base, base + span);
}

}  // namespace

PreviewImage PreviewExtractor::extract(const PreviewDescriptor& descriptor) const {
  return readPreviewBytes(descriptor);
}

PreviewImage PreviewExtractor::readPreviewBytes(
    const PreviewDescriptor& descriptor) {
  PreviewImage image;
  image.cache_key = descriptor.cacheKey();
  image.source_path = descriptor.absolute_path;

  std::ifstream stream(descriptor.absolute_path, std::ios::binary);
  if (!stream) {
    image.pixels = {0};
  } else {
    std::vector<std::uint8_t> data;
    data.resize(kMaxReadBytes);
    stream.read(reinterpret_cast<char*>(data.data()), kMaxReadBytes);
    const auto read_bytes = static_cast<std::size_t>(stream.gcount());
    data.resize(read_bytes);
    if (data.empty()) {
      data.push_back(0);
    }
    image.pixels = std::move(data);
  }

  image.width = pseudoDimension(descriptor.file_size, 512);
  image.height = pseudoDimension(descriptor.file_size / 2, 256);
  return image;
}

}  // namespace cataloger::services::preview
