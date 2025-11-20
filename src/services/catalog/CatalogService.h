#pragma once

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct sqlite3;

namespace cataloger::services::catalog {

struct FileRecord {
  std::filesystem::path absolute_path;
  std::string relative_path;
  std::string filename;
  std::string extension;
  std::uintmax_t file_size{};
  std::int64_t capture_ts{};
};

struct StoredFile {
  std::int64_t id{};
  std::string relative_path;
  std::string extension;
  std::optional<std::int64_t> stack_group_id;
  int preview_state{};
};

struct SyncEvent {
  std::int64_t id{};
  int root_id{};
  std::string relative_path;
  std::string event_type;
  std::string payload;
  bool processed{false};
  std::int64_t created_at{};
};

class CatalogService {
public:
  CatalogService();
  ~CatalogService();

  void configureDatabase(const std::filesystem::path& db_path);
  void initializeSchema();

  int registerRoot(const std::filesystem::path& root_path);
  std::vector<FileRecord> scanRoot(const std::filesystem::path& root_path) const;
  void ingestRecords(int root_id, const std::vector<FileRecord>& files);
  std::vector<StoredFile> listFiles(int root_id) const;

  void enqueueSyncEvent(int root_id,
                        std::string relative_path,
                        std::string event_type,
                        std::string payload);
  std::vector<SyncEvent> pendingSyncEvents() const;
  void markSyncEventProcessed(std::int64_t event_id);
  void updatePreviewState(std::int64_t file_id, int preview_state);

private:
  void open();
  void close();
  void ensureOpen() const;
  void applySchema(const std::string& sql) const;
  static std::int64_t unixTimestampNow();
  static std::int64_t toUnixTimestamp(std::filesystem::file_time_type ts);

  std::filesystem::path db_path_;
  sqlite3* db_;
  mutable std::mutex db_mutex_;
};

}  // namespace cataloger::services::catalog
