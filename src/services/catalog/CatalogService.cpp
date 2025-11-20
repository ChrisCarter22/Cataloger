#include "CatalogService.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <map>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include <sqlite3.h>

namespace cataloger::services::catalog {

namespace {

class Statement {
public:
  Statement(sqlite3* db, const std::string& sql) : db_(db), stmt_(nullptr) {
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
      throw std::runtime_error(sqlite3_errmsg(db_));
    }
  }

  ~Statement() {
    if (stmt_) {
      sqlite3_finalize(stmt_);
    }
  }

  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;

  sqlite3_stmt* get() const { return stmt_; }

  void reset() const {
    sqlite3_reset(stmt_);
    sqlite3_clear_bindings(stmt_);
  }

private:
  sqlite3* db_;
  sqlite3_stmt* stmt_;
};

void exec(sqlite3* db, const std::string& sql) {
  char* err = nullptr;
  if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
    std::string message = err ? err : "sqlite exec failure";
    sqlite3_free(err);
    throw std::runtime_error(message);
  }
}

std::string normalizeExtension(std::string ext) {
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return ext;
}

enum class ExtensionClass { kRaw, kJpeg, kOther };

ExtensionClass classifyExtension(const std::string& ext) {
  const std::string lowercase = normalizeExtension(ext);
  static const std::vector<std::string> raw_exts{
      ".cr2", ".cr3", ".nef", ".arw", ".raf", ".orf", ".rw2", ".dng"};
  static const std::vector<std::string> jpeg_exts{".jpg", ".jpeg"};
  if (std::find(raw_exts.begin(), raw_exts.end(), lowercase) != raw_exts.end()) {
    return ExtensionClass::kRaw;
  }
  if (std::find(jpeg_exts.begin(), jpeg_exts.end(), lowercase) != jpeg_exts.end()) {
    return ExtensionClass::kJpeg;
  }
  return ExtensionClass::kOther;
}

std::string baseName(const std::string& filename) {
  const auto dot = filename.find_last_of('.');
  if (dot == std::string::npos) {
    return filename;
  }
  return filename.substr(0, dot);
}

struct StackAccumulator {
  std::vector<std::int64_t> file_ids;
  bool has_raw{false};
  bool has_jpeg{false};
};

std::string stackType(const StackAccumulator& acc) {
  if (acc.has_raw && acc.has_jpeg) {
    return "pair";
  }
  if (acc.file_ids.size() > 1) {
    return "sequence";
  }
  return "single";
}

constexpr std::string_view kSchemaSql = R"SQL(
CREATE TABLE IF NOT EXISTS root_folders (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  path TEXT UNIQUE NOT NULL,
  created_at INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS files (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  root_id INTEGER NOT NULL REFERENCES root_folders(id) ON DELETE CASCADE,
  relative_path TEXT NOT NULL,
  filename TEXT NOT NULL,
  extension TEXT NOT NULL,
  capture_ts INTEGER,
  rating INTEGER DEFAULT 0,
  color INTEGER DEFAULT 0,
  ingest_seq INTEGER DEFAULT 0,
  file_size INTEGER,
  stack_group_id INTEGER,
  metadata_rev INTEGER DEFAULT 0,
  preview_state INTEGER DEFAULT 0,
  UNIQUE(root_id, relative_path)
);

CREATE TABLE IF NOT EXISTS stacks (
  stack_group_id INTEGER PRIMARY KEY AUTOINCREMENT,
  type TEXT NOT NULL,
  anchor_file_id INTEGER NOT NULL REFERENCES files(id)
);

CREATE TABLE IF NOT EXISTS metadata_blobs (
  file_id INTEGER PRIMARY KEY REFERENCES files(id) ON DELETE CASCADE,
  iptc_json TEXT,
  xmp_json TEXT,
  updated_at INTEGER,
  template_source TEXT
);

CREATE TABLE IF NOT EXISTS sync_queue (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  root_id INTEGER NOT NULL REFERENCES root_folders(id),
  relative_path TEXT NOT NULL,
  event_type TEXT NOT NULL,
  payload TEXT,
  processed_flag INTEGER NOT NULL DEFAULT 0,
  created_at INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_files_root_path ON files(root_id, relative_path);
CREATE INDEX IF NOT EXISTS idx_files_sort ON files(root_id, capture_ts, ingest_seq, id);
CREATE INDEX IF NOT EXISTS idx_sync_queue_processed ON sync_queue(processed_flag, id);
)SQL";

}  // namespace

CatalogService::CatalogService() : db_(nullptr) {}

CatalogService::~CatalogService() {
  std::scoped_lock lock(db_mutex_);
  close();
}

void CatalogService::configureDatabase(const std::filesystem::path& db_path) {
  std::scoped_lock lock(db_mutex_);
  close();
  db_path_ = db_path;
  if (!db_path_.has_parent_path()) {
    // Nothing to create; file lives in working directory.
  } else {
    std::filesystem::create_directories(db_path_.parent_path());
  }
  open();
}

void CatalogService::initializeSchema() {
  std::scoped_lock lock(db_mutex_);
  ensureOpen();
  applySchema(std::string(kSchemaSql));
}

int CatalogService::registerRoot(const std::filesystem::path& root_path) {
  std::scoped_lock lock(db_mutex_);
  ensureOpen();
  const auto absolute = std::filesystem::weakly_canonical(root_path).string();
  Statement insert(db_, "INSERT INTO root_folders(path, created_at) VALUES(?, ?)"
                         " ON CONFLICT(path) DO NOTHING;");
  sqlite3_bind_text(insert.get(), 1, absolute.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(insert.get(), 2, unixTimestampNow());
  if (sqlite3_step(insert.get()) != SQLITE_DONE) {
    throw std::runtime_error(sqlite3_errmsg(db_));
  }
  insert.reset();

  Statement query(db_, "SELECT id FROM root_folders WHERE path=?;");
  sqlite3_bind_text(query.get(), 1, absolute.c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(query.get()) != SQLITE_ROW) {
    throw std::runtime_error("failed to locate root folder row");
  }
  return sqlite3_column_int(query.get(), 0);
}

std::vector<FileRecord> CatalogService::scanRoot(
    const std::filesystem::path& root_path) const {
  if (!std::filesystem::exists(root_path)) {
    throw std::runtime_error("root path does not exist: " + root_path.string());
  }

  std::vector<FileRecord> files;
  for (const auto& entry :
       std::filesystem::recursive_directory_iterator(root_path)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    FileRecord record;
    record.absolute_path = entry.path();
    record.relative_path =
        std::filesystem::relative(entry.path(), root_path).generic_string();
    record.filename = entry.path().filename().string();
    record.extension = normalizeExtension(entry.path().extension().string());
    record.file_size = entry.file_size();
    std::error_code ec;
    const auto ts = std::filesystem::last_write_time(entry.path(), ec);
    if (ec) {
      record.capture_ts = 0;
    } else {
      record.capture_ts = toUnixTimestamp(ts);
    }
    files.push_back(std::move(record));
  }
  return files;
}

void CatalogService::ingestRecords(int root_id, const std::vector<FileRecord>& files) {
  if (files.empty()) {
    return;
  }
  std::scoped_lock lock(db_mutex_);
  ensureOpen();
  exec(db_, "BEGIN IMMEDIATE TRANSACTION;");

  Statement insert(db_,
                   "INSERT INTO files (root_id, relative_path, filename, extension, "
                   "capture_ts, file_size) "
                   "VALUES (?, ?, ?, ?, ?, ?);");

  std::unordered_map<std::string, StackAccumulator> accumulators;

  for (const auto& record : files) {
    insert.reset();
    sqlite3_bind_int(insert.get(), 1, root_id);
    sqlite3_bind_text(insert.get(), 2, record.relative_path.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(insert.get(), 3, record.filename.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(insert.get(), 4, record.extension.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(insert.get(), 5, record.capture_ts);
    sqlite3_bind_int64(insert.get(), 6, static_cast<std::int64_t>(record.file_size));

    if (sqlite3_step(insert.get()) != SQLITE_DONE) {
      exec(db_, "ROLLBACK;");
      throw std::runtime_error(sqlite3_errmsg(db_));
    }

    const auto row_id = sqlite3_last_insert_rowid(db_);
    auto& bucket = accumulators[baseName(record.filename)];
    bucket.file_ids.push_back(row_id);
    const auto classification = classifyExtension(record.extension);
    bucket.has_jpeg = bucket.has_jpeg || classification == ExtensionClass::kJpeg;
    bucket.has_raw = bucket.has_raw || classification == ExtensionClass::kRaw;
  }

  Statement insert_stack(
      db_, "INSERT INTO stacks(type, anchor_file_id) VALUES(?, ?);");
  Statement update_file(db_, "UPDATE files SET stack_group_id=? WHERE id=?;");

  for (auto& [_, bucket] : accumulators) {
    if (bucket.file_ids.size() < 2) {
      continue;
    }

    insert_stack.reset();
    const auto type = stackType(bucket);
    sqlite3_bind_text(insert_stack.get(), 1, type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(insert_stack.get(), 2, bucket.file_ids.front());
    if (sqlite3_step(insert_stack.get()) != SQLITE_DONE) {
      exec(db_, "ROLLBACK;");
      throw std::runtime_error(sqlite3_errmsg(db_));
    }
    const auto stack_id = sqlite3_last_insert_rowid(db_);

    for (const auto file_id : bucket.file_ids) {
      update_file.reset();
      sqlite3_bind_int64(update_file.get(), 1, stack_id);
      sqlite3_bind_int64(update_file.get(), 2, file_id);
      if (sqlite3_step(update_file.get()) != SQLITE_DONE) {
        exec(db_, "ROLLBACK;");
        throw std::runtime_error(sqlite3_errmsg(db_));
      }
    }
  }

  exec(db_, "COMMIT;");
}

std::vector<StoredFile> CatalogService::listFiles(int root_id) const {
  std::scoped_lock lock(db_mutex_);
  ensureOpen();
  Statement stmt(db_,
                 "SELECT id, relative_path, extension, stack_group_id, preview_state "
                 "FROM files WHERE root_id=? ORDER BY id ASC;");
  sqlite3_bind_int(stmt.get(), 1, root_id);

  std::vector<StoredFile> rows;
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    StoredFile file;
    file.id = sqlite3_column_int64(stmt.get(), 0);
    file.relative_path =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
    file.extension =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
    if (sqlite3_column_type(stmt.get(), 3) != SQLITE_NULL) {
      file.stack_group_id = sqlite3_column_int64(stmt.get(), 3);
    }
    file.preview_state = sqlite3_column_int(stmt.get(), 4);
    rows.push_back(std::move(file));
  }
  return rows;
}

void CatalogService::enqueueSyncEvent(int root_id,
                                      std::string relative_path,
                                      std::string event_type,
                                      std::string payload) {
  std::scoped_lock lock(db_mutex_);
  ensureOpen();
  Statement stmt(db_,
                 "INSERT INTO sync_queue(root_id, relative_path, event_type, payload, "
                 "created_at) VALUES(?, ?, ?, ?, ?);");
  sqlite3_bind_int(stmt.get(), 1, root_id);
  sqlite3_bind_text(stmt.get(), 2, relative_path.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 3, event_type.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 4, payload.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 5, unixTimestampNow());

  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error(sqlite3_errmsg(db_));
  }
}

std::vector<SyncEvent> CatalogService::pendingSyncEvents() const {
  std::scoped_lock lock(db_mutex_);
  ensureOpen();
  Statement stmt(
      db_, "SELECT id, root_id, relative_path, event_type, payload, processed_flag, "
           "created_at FROM sync_queue WHERE processed_flag=0 ORDER BY id ASC;");

  std::vector<SyncEvent> events;
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    SyncEvent event;
    event.id = sqlite3_column_int64(stmt.get(), 0);
    event.root_id = sqlite3_column_int(stmt.get(), 1);
    event.relative_path =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
    event.event_type =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 3));
    if (sqlite3_column_type(stmt.get(), 4) != SQLITE_NULL) {
      event.payload =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 4));
    }
    event.processed = sqlite3_column_int(stmt.get(), 5) != 0;
    event.created_at = sqlite3_column_int64(stmt.get(), 6);
    events.push_back(std::move(event));
  }
  return events;
}

void CatalogService::markSyncEventProcessed(std::int64_t event_id) {
  std::scoped_lock lock(db_mutex_);
  ensureOpen();
  Statement stmt(db_, "UPDATE sync_queue SET processed_flag=1 WHERE id=?;");
  sqlite3_bind_int64(stmt.get(), 1, event_id);
  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error(sqlite3_errmsg(db_));
  }
}

void CatalogService::updatePreviewState(std::int64_t file_id, int preview_state) {
  std::scoped_lock lock(db_mutex_);
  ensureOpen();
  Statement stmt(db_, "UPDATE files SET preview_state=? WHERE id=?;");
  sqlite3_bind_int(stmt.get(), 1, preview_state);
  sqlite3_bind_int64(stmt.get(), 2, file_id);
  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error(sqlite3_errmsg(db_));
  }
}

void CatalogService::open() {
  if (db_) {
    return;
  }

  if (db_path_.empty()) {
    throw std::runtime_error("database path not configured");
  }

  if (sqlite3_open(db_path_.string().c_str(), &db_) != SQLITE_OK) {
    std::string message = sqlite3_errmsg(db_);
    sqlite3_close(db_);
    db_ = nullptr;
    throw std::runtime_error(message);
  }
}

void CatalogService::close() {
  if (db_) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

void CatalogService::ensureOpen() const {
  if (!db_) {
    throw std::runtime_error("database not open");
  }
}

void CatalogService::applySchema(const std::string& sql) const {
  exec(db_, sql);
}

std::int64_t CatalogService::unixTimestampNow() {
  const auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
      .count();
}

std::int64_t CatalogService::toUnixTimestamp(
    std::filesystem::file_time_type ts) {
  using namespace std::chrono;
  const auto sctp =
      time_point_cast<system_clock::duration>(ts - decltype(ts)::clock::now() +
                                              system_clock::now());
  return duration_cast<seconds>(sctp.time_since_epoch()).count();
}

}  // namespace cataloger::services::catalog
