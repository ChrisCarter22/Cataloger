#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "platform/gpu/GpuBridge.h"
#include "services/catalog/CatalogService.h"
#include "services/preview/PreviewService.h"
#include "services/preview/PreviewTypes.h"
#include <lcms2.h>

namespace {

std::string uniqueSuffix() {
  const auto ticks =
      std::chrono::steady_clock::now().time_since_epoch().count();
  return std::to_string(ticks);
}

void writeFile(const std::filesystem::path& path, std::size_t size) {
  std::ofstream stream(path, std::ios::binary);
  for (std::size_t i = 0; i < size; ++i) {
    const auto value = static_cast<char>('A' + (i % 26));
    stream.write(&value, 1);
  }
}

void writeICCProfile(const std::filesystem::path& path) {
  cmsHPROFILE profile = cmsCreate_sRGBProfile();
  if (!profile) {
    return;
  }
  cmsSaveProfileToFile(profile, path.string().c_str());
  cmsCloseProfile(profile);
}

}  // namespace

class PreviewServiceTest : public ::testing::Test {
protected:
  void SetUp() override {
    const auto suffix = uniqueSuffix();
    root_path_ = std::filesystem::temp_directory_path() /
                 ("preview_unit_root_" + suffix);
    db_path_ = std::filesystem::temp_directory_path() /
               ("preview_unit_db_" + suffix + ".db");
    std::filesystem::create_directories(root_path_);

    relative_files_ = {"IMG_0001.CR3", "IMG_0001.JPG", "IMG_0002.JPG",
                       "IMG_0003.CR3", "IMG_0003.JPG"};
    for (std::size_t i = 0; i < relative_files_.size(); ++i) {
      writeFile(root_path_ / relative_files_[i], 1024 + i * 16);
    }

    catalog_.configureDatabase(db_path_);
    catalog_.initializeSchema();
    root_id_ = catalog_.registerRoot(root_path_);
    const auto records = catalog_.scanRoot(root_path_);
    catalog_.ingestRecords(root_id_, records);

    preview_.setCatalogService(&catalog_);
    preview_.primeCaches(2);
  }

  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove(db_path_, ec);
    std::filesystem::remove_all(root_path_, ec);
  }

  cataloger::services::catalog::CatalogService catalog_;
  cataloger::services::preview::PreviewService preview_{8, 4, 1};
  std::filesystem::path root_path_;
  std::filesystem::path db_path_;
  int root_id_{};
  std::vector<std::string> relative_files_;
};

TEST_F(PreviewServiceTest, WarmRootCachesFiles) {
  std::vector<cataloger::services::preview::CacheEvent> events;
  preview_.setEventSink(
      [&](const cataloger::services::preview::CacheEvent& event) {
        events.push_back(event);
      });
  preview_.warmRoot(root_id_, root_path_);
  preview_.waitUntilIdle();

  ASSERT_GE(events.size(), relative_files_.size());
  const auto key = relative_files_.front() + "#" + std::to_string(root_id_);
  const auto cached = preview_.cachedPreview(key);
  ASSERT_TRUE(cached.has_value());
  EXPECT_TRUE(cached->color_managed);
  EXPECT_FALSE(cached->color_profile.empty());

  const auto files = catalog_.listFiles(root_id_);
  ASSERT_EQ(files.size(), relative_files_.size());
  EXPECT_NE(files.front().preview_state,
            static_cast<int>(
                cataloger::services::preview::PreviewState::kIdle));
}

TEST_F(PreviewServiceTest, RequestPreviewPreloadsNeighbors) {
  preview_.warmRoot(root_id_, root_path_);
  preview_.waitUntilIdle();

  std::vector<cataloger::services::preview::CacheEvent> events;
  preview_.setEventSink(
      [&](const cataloger::services::preview::CacheEvent& event) {
        events.push_back(event);
      });

  preview_.requestPreview(root_id_, relative_files_[1]);
  preview_.waitUntilIdle();

  // Expect at least the requested preview plus one neighbor to be processed.
  EXPECT_GE(events.size(), 2);
  std::unordered_set<std::string> relative_paths;
  for (const auto& event : events) {
    relative_paths.insert(event.relative_path);
  }
  EXPECT_TRUE(relative_paths.contains(relative_files_[1]));
}

class FailingBridge : public cataloger::platform::gpu::GpuBridge {
public:
  bool upload(const cataloger::services::preview::PreviewImage&,
              std::string& error) override {
    error = "forced failure";
    return false;
  }

  cataloger::platform::gpu::Backend backend() const noexcept override {
    return cataloger::platform::gpu::Backend::kStub;
  }
};

TEST_F(PreviewServiceTest, EmitsErrorWhenGpuUploadFails) {
  preview_.setGpuBridgeForTesting(std::make_unique<FailingBridge>());

  std::vector<cataloger::services::preview::CacheEvent> events;
  preview_.setEventSink(
      [&](const cataloger::services::preview::CacheEvent& event) {
        events.push_back(event);
      });
  preview_.warmRoot(root_id_, root_path_);
  preview_.waitUntilIdle();

  const auto it =
      std::find_if(events.begin(), events.end(),
                   [](const cataloger::services::preview::CacheEvent& event) {
                     return event.error;
                   });
  ASSERT_NE(it, events.end());
  EXPECT_TRUE(it->error);
  EXPECT_FALSE(it->error_message.empty());
  EXPECT_EQ(it->backend, "Stub");
}

TEST_F(PreviewServiceTest, AppliesExternalProfileWhenPresent) {
  const auto icc_path = (root_path_ / relative_files_.front()).replace_extension(".icc");
  writeICCProfile(icc_path);

  preview_.warmRoot(root_id_, root_path_);
  preview_.waitUntilIdle();

  const auto key = relative_files_.front() + "#" + std::to_string(root_id_);
  const auto cached = preview_.cachedPreview(key);
  ASSERT_TRUE(cached.has_value());
  EXPECT_NE(cached->color_profile.find("sRGB"), std::string::npos);
}
