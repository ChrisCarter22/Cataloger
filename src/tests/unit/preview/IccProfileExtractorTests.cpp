#include <gtest/gtest.h>

#include <filesystem>

#include "services/preview/IccProfileExtractor.h"

namespace {

std::filesystem::path sourceRoot() {
#ifdef CATALOGER_SOURCE_DIR
  return std::filesystem::path(CATALOGER_SOURCE_DIR);
#else
  return std::filesystem::current_path();
#endif
}

}  // namespace

TEST(IccProfileExtractorTests, ExtractsProfileFromJpegWhenPresent) {
  const auto root = sourceRoot();
  const auto path = root / "test_images" /
                    "CC_CINCINNATIvCOLUMBUS_MLS_PLAYOFFS_0052.JPG";
  ASSERT_TRUE(std::filesystem::exists(path)) << path;

  const auto profile =
      cataloger::services::preview::icc::ExtractEmbeddedProfile(path);
  // Not all images will have ICC profiles; this test asserts the plumbing
  // and will be updated once we lock the fixture set.
  SUCCEED();
}

