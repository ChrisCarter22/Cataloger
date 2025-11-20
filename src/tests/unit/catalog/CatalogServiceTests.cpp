#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "services/catalog/CatalogService.h"

namespace {

std::string uniqueSuffix() {
  const auto now =
      std::chrono::steady_clock::now().time_since_epoch().count();
  std::stringstream ss;
  ss << now;
  return ss.str();
}

void writeFile(const std::filesystem::path& path) {
  std::ofstream stream(path);
  stream << "catalog-test";
}

}  // namespace

class CatalogServiceTest : public ::testing::Test {
protected:
  void SetUp() override {
    const auto suffix = uniqueSuffix();
    db_path_ = std::filesystem::temp_directory_path() /
               ("cataloger_test_db_" + suffix + ".db");
    root_path_ = std::filesystem::temp_directory_path() /
                 ("cataloger_root_" + suffix);
    std::filesystem::create_directories(root_path_);
    service_.configureDatabase(db_path_);
    service_.initializeSchema();
  }

  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove(db_path_, ec);
    std::filesystem::remove_all(root_path_, ec);
  }

  cataloger::services::catalog::CatalogService service_;
  std::filesystem::path db_path_;
  std::filesystem::path root_path_;
};

TEST_F(CatalogServiceTest, SchemaInitializationCreatesTables) {
  const auto root_id = service_.registerRoot(root_path_);
  EXPECT_GT(root_id, 0);
  const auto stored = service_.listFiles(root_id);
  EXPECT_TRUE(stored.empty());
}

TEST_F(CatalogServiceTest, ScanAndIngestBuildsStacks) {
  writeFile(root_path_ / "IMG_0001.CR3");
  writeFile(root_path_ / "IMG_0001.JPG");
  writeFile(root_path_ / "RANDOM.TXT");

  const auto root_id = service_.registerRoot(root_path_);
  const auto records = service_.scanRoot(root_path_);
  EXPECT_EQ(records.size(), 3);
  service_.ingestRecords(root_id, records);

  const auto stored = service_.listFiles(root_id);
  ASSERT_EQ(stored.size(), 3);
  std::size_t stacked = 0;
  for (const auto& file : stored) {
    if (file.stack_group_id.has_value()) {
      ++stacked;
    }
  }
  EXPECT_EQ(stacked, 2);
}

TEST_F(CatalogServiceTest, SyncQueuePersistsEvents) {
  const auto root_id = service_.registerRoot(root_path_);
  service_.enqueueSyncEvent(root_id, "IMG_0001.CR3", "created", "{}");
  auto events = service_.pendingSyncEvents();
  ASSERT_EQ(events.size(), 1);
  EXPECT_FALSE(events.front().processed);
  service_.markSyncEventProcessed(events.front().id);
  events = service_.pendingSyncEvents();
  EXPECT_TRUE(events.empty());
}
