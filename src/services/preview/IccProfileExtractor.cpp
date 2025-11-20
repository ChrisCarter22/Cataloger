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

// Reads length bytes from file into buffer; returns false on failure.
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

  // Verify SOI (0xFFD8).
  if (read_byte() != 0xFF || read_byte() != 0xD8) {
    return {};
  }

  std::map<int, std::vector<std::uint8_t>> chunks;
  int chunk_total = -1;

  while (file && file.good()) {
    int prefix = read_byte();
    if (prefix != 0xFF) {
      continue;
    }
    int marker = read_byte();
    if (marker == EOF || marker == 0xD9 || marker == 0xDA) {
      break;  // EOI or SOS -> stop scanning
    }

    const int high = read_byte();
    const int low = read_byte();
    const std::uint16_t length = static_cast<std::uint16_t>((high << 8) | low);
    if (length < 2) {
      break;
    }

    if (marker == 0xE2) {  // APP2 potential ICC chunk
      std::vector<std::uint8_t> payload;
      if (!ReadSegment(file, payload, length - 2)) {
        break;
      }

      static constexpr char kSignature[] = "ICC_PROFILE";
      if (payload.size() < sizeof(kSignature)) {
        continue;
      }
      if (std::memcmp(payload.data(), kSignature, sizeof(kSignature) - 1) != 0) {
        continue;
      }
      if (payload.size() < sizeof(kSignature) + 2) {
        continue;
      }

      const int chunk_number = payload[sizeof(kSignature)];
      const int total_chunks = payload[sizeof(kSignature) + 1];
      if (chunk_number <= 0 || total_chunks <= 0) {
        continue;
      }
      chunk_total = std::max(chunk_total, total_chunks);

      std::vector<std::uint8_t> chunk(payload.begin() + sizeof(kSignature) + 2,
                                      payload.end());
      chunks[chunk_number] = std::move(chunk);
    } else {
      // Skip non-APP2 segments.
      file.seekg(length - 2, std::ios::cur);
    }
  }

  if (chunks.empty()) {
    return {};
  }

  std::vector<std::uint8_t> profile;
  const int expected = (chunk_total == -1) ? static_cast<int>(chunks.size())
                                           : chunk_total;
  for (int i = 1; i <= expected; ++i) {
    const auto it = chunks.find(i);
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
  // RAW parsing will be added once we standardize the metadata toolkit.
  return {};
}

}  // namespace cataloger::services::preview::icc
