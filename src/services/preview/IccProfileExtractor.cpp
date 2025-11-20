#include "IccProfileExtractor.h"

#include <array>
#include <cstring>
#include <fstream>
#include <map>

namespace cataloger::services::preview::icc {

namespace {

bool IsJpeg(const std::filesystem::path& path) {
  const auto ext = path.extension().string();
  return ext == ".jpg" || ext == ".JPG" || ext == ".jpeg" || ext == ".JPEG";
}

bool ReadSegment(std::ifstream& file,
                 std::vector<std::uint8_t>& buffer,
                 std::uint16_t length) {
  buffer.resize(length);
  file.read(reinterpret_cast<char*>(buffer.data()), length);
  return static_cast<std::uint16_t>(file.gcount()) == length;
}

std::vector<std::uint8_t> ExtractFromJpeg(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return {};
  }

  auto read_byte = [&]() -> int {
    return file.get();
  };

  // Validate SOI marker (0xFFD8).
  if (read_byte() != 0xFF || read_byte() != 0xD8) {
    return {};
  }

  std::map<int, std::vector<std::uint8_t>> chunks;
  int chunk_count = -1;

  while (file && file.good()) {
    int marker_prefix = read_byte();
    if (marker_prefix != 0xFF) {
      continue;
    }
    int marker = read_byte();
    if (marker == 0xD9 || marker == 0xDA || marker == EOF) {
      break;  // EOI or SOS, stop scanning headers.
    }

    if (marker == 0xE2) {  // APP2
      const auto high = read_byte();
      const auto low = read_byte();
      const std::uint16_t length = (high << 8) | low;
      if (length < 2) {
        break;
      }
      std::vector<std::uint8_t> payload;
      if (!ReadSegment(file, payload, length - 2)) {
        break;
      }
      static constexpr char kICCSignature[] = "ICC_PROFILE";
      if (payload.size() < sizeof(kICCSignature)) {
        continue;
      }
      if (std::memcmp(payload.data(), kICCSignature, sizeof(kICCSignature) - 1) !=
          0) {
        continue;
      }
      if (payload.size() < sizeof(kICCSignature) + 2) {
        continue;
      }
      const int chunk_number = payload[sizeof(kICCSignature)];
      const int chunk_total = payload[sizeof(kICCSignature) + 1];
      if (chunk_total <= 0 || chunk_number <= 0) {
        continue;
      }
      chunk_count = std::max(chunk_count, chunk_total);
      std::vector<std::uint8_t> chunk(payload.begin() + sizeof(kICCSignature) + 2,
                                      payload.end());
      chunks[chunk_number] = std::move(chunk);
    } else {
      const int high = read_byte();
      const int low = read_byte();
      const std::uint16_t length = (high << 8) | low;
      if (length < 2) {
        break;
      }
      file.seekg(length - 2, std::ios::cur);
    }
  }

  if (chunks.empty()) {
    return {};
  }

  std::vector<std::uint8_t> profile;
  const int total = (chunk_count == -1) ? static_cast<int>(chunks.size()) : chunk_count;
  for (int i = 1; i <= total; ++i) {
    auto it = chunks.find(i);
    if (it == chunks.end()) {
      return {};
    }
    profile.insert(profile.end(), it->second.begin(), it->second.end());
  }
  return profile;
}

}  // namespace

std::vector<std::uint8_t> ExtractEmbeddedProfile(
    const std::filesystem::path& image_path) {
  if (IsJpeg(image_path)) {
    return ExtractFromJpeg(image_path);
  }
  // RAW formats are not yet parsed; rely on sidecars for now.
  return {};
}

}  // namespace cataloger::services::preview::icc
