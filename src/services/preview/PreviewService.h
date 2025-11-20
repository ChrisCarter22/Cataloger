#pragma once

#include <atomic>
#include <array>
#include <condition_variable>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "ColorTransformer.h"
#include "DirectoryScanner.h"
#include "IccProfileExtractor.h"
#include "PreviewCache.h"
#include "PreviewExtractor.h"
#include "PreviewTypes.h"
#include "platform/gpu/GpuBridge.h"
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
  void setGpuBridgeForTesting(
      std::unique_ptr<cataloger::platform::gpu::GpuBridge> bridge);

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
  void emitEvent(const PreviewDescriptor& descriptor,
                 CacheTier tier,
                 bool hit,
                 bool error = false,
                 const std::string& message = {},
                 const std::string& backend = {},
                 double gpu_ms = 0.0,
                 double transform_ms = 0.0);
  void storeDescriptorCache(int root_id,
                            std::vector<PreviewDescriptor> descriptors);
  void scheduleNeighbors(int root_id, std::size_t anchor_index);
  std::vector<std::uint8_t> loadEmbeddedProfile(
      const PreviewDescriptor& descriptor) const;
  static std::string backendLabel(
      const cataloger::platform::gpu::GpuBridge* bridge);
  void shutdown();

  services::catalog::CatalogService* catalog_service_;
  CacheEventSink event_sink_;

  DirectoryScanner scanner_;
  PreviewExtractor extractor_;
  ColorTransformer color_transformer_;
  PreviewCache cache_;
  std::unique_ptr<cataloger::platform::gpu::GpuBridge> gpu_bridge_;

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
