#include "ColorTransformer.h"

#include <lcms2.h>

namespace cataloger::services::preview {

ColorTransformer::ColorTransformer()
    : target_profile_(createSRGBProfile()), profile_name_("sRGB IEC61966-2.1") {}

ColorTransformer::~ColorTransformer() = default;

void ColorTransformer::ProfileDeleter::operator()(void* profile) const {
  if (profile) {
    cmsCloseProfile(static_cast<cmsHPROFILE>(profile));
  }
}

ColorTransformer::ProfileHandle ColorTransformer::createSRGBProfile() const {
  return ProfileHandle(cmsCreate_sRGBProfile(), ProfileDeleter());
}

ColorTransformer::ProfileHandle ColorTransformer::openProfileFromMemory(
    const std::vector<std::uint8_t>& bytes) const {
  if (bytes.empty()) {
    return {};
  }
  return ProfileHandle(
      cmsOpenProfileFromMem(bytes.data(),
                            static_cast<cmsUInt32Number>(bytes.size())),
      ProfileDeleter());
}

std::string ColorTransformer::profileDescription(cmsHPROFILE profile) {
  if (!profile) {
    return "Unknown";
  }
  char buffer[256] = {};
  if (cmsGetProfileInfoASCII(profile,
                              cmsInfoDescription,
                              "en",
                              "US",
                              buffer,
                              sizeof(buffer))) {
    return std::string(buffer);
  }
  return "Custom ICC Profile";
}

ColorTransformResult ColorTransformer::apply(
    const PreviewImage& image,
    const std::vector<std::uint8_t>& icc_profile) const {
  ColorTransformResult result;
  if (image.pixels.empty()) {
    result.pixels = image.pixels;
    result.source_profile = "Empty";
    return result;
  }

  auto source_profile = icc_profile.empty() ? createSRGBProfile()
                                            : openProfileFromMemory(icc_profile);
  if (!source_profile) {
    source_profile = createSRGBProfile();
  }

  if (!source_profile || !target_profile_) {
    result.pixels = image.pixels;
    result.source_profile = "Unmanaged";
    return result;
  }

  cmsHTRANSFORM transform =
      cmsCreateTransform(static_cast<cmsHPROFILE>(source_profile.get()),
                         TYPE_RGB_8,
                         static_cast<cmsHPROFILE>(target_profile_.get()),
                         TYPE_RGB_8,
                         INTENT_PERCEPTUAL,
                         0);
  if (!transform) {
    result.pixels = image.pixels;
    result.source_profile = "TransformFailed";
    return result;
  }

  std::vector<std::uint8_t> corrected(image.pixels.size());
  const auto pixel_count = static_cast<int>(image.pixels.size() / 3);
  cmsDoTransform(transform,
                 image.pixels.data(),
                 corrected.data(),
                 pixel_count);
  cmsDeleteTransform(transform);

  result.pixels = std::move(corrected);
  result.source_profile =
      profileDescription(static_cast<cmsHPROFILE>(source_profile.get()));
  return result;
}

std::string ColorTransformer::targetProfileName() const {
  return profile_name_;
}

}  // namespace cataloger::services::preview
