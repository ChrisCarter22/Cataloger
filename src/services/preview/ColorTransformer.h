#pragma once

#include <lcms2.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "PreviewTypes.h"

namespace cataloger::services::preview {

struct ColorTransformResult {
  std::vector<std::uint8_t> pixels;
  std::string source_profile;
};

class ColorTransformer {
public:
  ColorTransformer();
  ~ColorTransformer();

  ColorTransformResult apply(const PreviewImage& image,
                             const std::vector<std::uint8_t>& icc_profile) const;
  [[nodiscard]] std::string targetProfileName() const;

private:
  struct ProfileDeleter {
    void operator()(void* profile) const;
  };

  using ProfileHandle = std::unique_ptr<void, ProfileDeleter>;

  ProfileHandle createSRGBProfile() const;
  ProfileHandle openProfileFromMemory(const std::vector<std::uint8_t>& bytes) const;
  static std::string profileDescription(cmsHPROFILE profile);

  ProfileHandle target_profile_;
  std::string profile_name_;
};

}  // namespace cataloger::services::preview
