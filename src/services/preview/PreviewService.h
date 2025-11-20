#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <vector>

#include "DirectoryScanner.h"
#include "GpuBridge.h"
#include "PreviewCache.h"
#include "PreviewExtractor.h"
#include "PreviewTypes.h"
#include "services/catalog/CatalogService.h"

namespace cataloger::services::preview {

using CacheEventSink = std::function<void(const CacheEvent&)>;

class PreviewService {
public:
  PreviewService(std::size_t ram_capacity = 64,
                 std::size_t preload_capacity = 8,
                 std::size_t worker_count = 0);
  ~PreviewService();

  void setCatalogService(services::catalog::CatalogService* catalog);
  void setEventSink(CacheEventSink sink);

  void warmRoot(int root_id, const std::filesystem::path& root_path);
  void requestPreview(int root_id, const std::string& relative_path);
  void primeCaches(std::size_t neighborCount);
  [[nodiscard]] std::optional<PreviewImage> cachedPreview(
      const std::string& cache_key) const;
  void waitUntilIdle() const;

private:
  void scheduleJob(const PreviewDescriptor& descriptor);
  void workerLoop(std::stop_token stop_token);
  void processJob(const PreviewDescriptor& descriptor);
  void emitEvent(const PreviewDescriptor& descriptor, CacheTier tier, bool hit);
  void storeDescriptorCache(int root_id,
                            std::vector<PreviewDescriptor> descriptors);
  void scheduleNeighbors(int root_id, std::size_t anchor_index);
  void shutdown();

  services::catalog::CatalogService* catalog_service_;
  CacheEventSink event_sink_;

  DirectoryScanner scanner_;
  PreviewExtractor extractor_;
  PreviewCache cache_;
  GpuBridge gpu_bridge_;

  mutable std::mutex queue_mutex_;
  mutable std::condition_variable queue_cv_;
  mutable std::condition_variable idle_cv_;
  std::queue<PreviewDescriptor> jobs_;
  std::vector<std::jthread> workers_;
  bool stop_;
  std::size_t neighbor_window_;
  mutable std::size_t pending_jobs_;

  mutable std::mutex descriptor_mutex_;
  std::unordered_map<int, std::vector<PreviewDescriptor>> root_descriptors_;
  std::unordered_map<int, std::unordered_map<std::string, std::size_t>>
      descriptor_index_;
};

}  // namespace cataloger::services::preview
