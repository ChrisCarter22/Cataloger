#include "PreviewService.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iterator>
#include <unordered_set>

namespace cataloger::services::preview {

namespace {

std::size_t resolveWorkerCount(std::size_t requested) {
  if (requested != 0) {
    return requested;
  }
  const auto hw = std::thread::hardware_concurrency();
  return std::max<std::size_t>(2, hw == 0 ? 2 : hw);
}

}  // namespace

PreviewService::PreviewService(std::size_t ram_capacity,
                               std::size_t preload_capacity,
                               std::size_t worker_count)
    : catalog_service_(nullptr),
      cache_(ram_capacity, preload_capacity),
      gpu_bridge_(cataloger::platform::gpu::CreateBridge()),
      stop_(false),
      neighbor_window_(2),
      pending_jobs_(0) {
  const auto worker_total = resolveWorkerCount(worker_count);
  workers_.reserve(worker_total);
  for (std::size_t i = 0; i < worker_total; ++i) {
    workers_.emplace_back([this](std::stop_token token) { workerLoop(token); });
  }
}

PreviewService::~PreviewService() {
  shutdown();
}

void PreviewService::shutdown() {
  {
    std::lock_guard lock(queue_mutex_);
    stop_ = true;
  }
  queue_cv_.notify_all();
  for (auto& worker : workers_) {
    worker.request_stop();
  }
  workers_.clear();
}

void PreviewService::setCatalogService(services::catalog::CatalogService* catalog) {
  catalog_service_ = catalog;
}

void PreviewService::setEventSink(CacheEventSink sink) {
  event_sink_ = std::move(sink);
}

void PreviewService::setGpuBridgeForTesting(
    std::unique_ptr<cataloger::platform::gpu::GpuBridge> bridge) {
  gpu_bridge_ = std::move(bridge);
}

void PreviewService::warmRoot(int root_id, const std::filesystem::path& root_path) {
  auto descriptors = scanner_.scan(root_path);

  std::unordered_map<std::string, std::int64_t> file_ids;
  if (catalog_service_) {
    for (const auto& file : catalog_service_->listFiles(root_id)) {
      file_ids[file.relative_path] = file.id;
    }
  }

  for (auto& descriptor : descriptors) {
    descriptor.root_id = root_id;
    descriptor.absolute_path = root_path / descriptor.relative_path;
    if (const auto it = file_ids.find(descriptor.relative_path);
        it != file_ids.end()) {
      descriptor.file_id = it->second;
    }
  }

  storeDescriptorCache(root_id, descriptors);
  for (const auto& descriptor : descriptors) {
    scheduleJob(descriptor);
  }
}

void PreviewService::storeDescriptorCache(
    int root_id,
    std::vector<PreviewDescriptor> descriptors) {
  std::lock_guard lock(descriptor_mutex_);
  descriptor_index_[root_id].clear();
  for (std::size_t i = 0; i < descriptors.size(); ++i) {
    descriptor_index_[root_id][descriptors[i].relative_path] = i;
  }
  root_descriptors_[root_id] = std::move(descriptors);
}

void PreviewService::requestPreview(int root_id, const std::string& relative_path) {
  PreviewDescriptor descriptor;
  std::size_t anchor = 0;
  {
    std::lock_guard lock(descriptor_mutex_);
    const auto root_it = root_descriptors_.find(root_id);
    if (root_it == root_descriptors_.end()) {
      return;
    }
    const auto index_it = descriptor_index_[root_id].find(relative_path);
    if (index_it == descriptor_index_[root_id].end()) {
      return;
    }
    anchor = index_it->second;
    descriptor = root_it->second[anchor];
  }

  scheduleJob(descriptor);
  scheduleNeighbors(root_id, anchor);
}

void PreviewService::scheduleNeighbors(int root_id, std::size_t anchor_index) {
  if (neighbor_window_ == 0) {
    return;
  }

  std::vector<PreviewDescriptor> neighbor_jobs;
  {
    std::lock_guard lock(descriptor_mutex_);
    const auto root_it = root_descriptors_.find(root_id);
    if (root_it == root_descriptors_.end()) {
      return;
    }
    const auto start =
        (anchor_index > neighbor_window_) ? anchor_index - neighbor_window_ : 0;
    const auto end =
        std::min(root_it->second.size(), anchor_index + neighbor_window_ + 1);
    for (std::size_t i = start; i < end; ++i) {
      if (i == anchor_index) {
        continue;
      }
      neighbor_jobs.push_back(root_it->second[i]);
    }
  }

  for (const auto& job : neighbor_jobs) {
    scheduleJob(job);
  }
}

void PreviewService::primeCaches(std::size_t neighborCount) {
  neighbor_window_ = neighborCount;
}

std::optional<PreviewImage> PreviewService::cachedPreview(
    const std::string& cache_key) const {
  return cache_.get(cache_key);
}

void PreviewService::waitUntilIdle() const {
  std::unique_lock lock(queue_mutex_);
  idle_cv_.wait(lock, [&] {
    return jobs_.empty() && pending_jobs_ == 0;
  });
}

void PreviewService::scheduleJob(const PreviewDescriptor& descriptor) {
  {
    std::lock_guard lock(queue_mutex_);
    jobs_.push(descriptor);
    ++pending_jobs_;
  }
  queue_cv_.notify_one();
}

void PreviewService::workerLoop(std::stop_token stop_token) {
  while (!stop_token.stop_requested()) {
    PreviewDescriptor descriptor;
    {
      std::unique_lock lock(queue_mutex_);
      queue_cv_.wait(lock, [&] { return stop_ || !jobs_.empty(); });
      if ((stop_ && jobs_.empty()) || stop_token.stop_requested()) {
        return;
      }
      descriptor = jobs_.front();
      jobs_.pop();
    }

    processJob(descriptor);

    {
      std::lock_guard lock(queue_mutex_);
      if (pending_jobs_ > 0) {
        --pending_jobs_;
      }
      if (pending_jobs_ == 0 && jobs_.empty()) {
        idle_cv_.notify_all();
      }
    }
  }
}

void PreviewService::processJob(const PreviewDescriptor& descriptor) {
  const auto key = descriptor.cacheKey();
  if (auto cached = cache_.get(key)) {
    emitEvent(descriptor, CacheTier::kRam, true, false, {}, "cache");
    return;
  }

  const auto transform_start = std::chrono::steady_clock::now();
  auto image = extractor_.extract(descriptor);
  const auto profile_bytes = loadEmbeddedProfile(descriptor);
  auto transform_result = color_transformer_.apply(image, profile_bytes);
  image.pixels = std::move(transform_result.pixels);
  image.color_managed = true;
  image.color_profile = transform_result.source_profile + " -> " +
                        color_transformer_.targetProfileName();
  cache_.put(image, CacheTier::kRam);
  const auto transform_duration =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() -
                                                transform_start)
          .count();

  bool gpu_ok = true;
  std::string gpu_error;
  std::string backend_name = backendLabel(gpu_bridge_.get());
  double gpu_duration_ms = 0.0;
  if (gpu_bridge_) {
    const auto gpu_start = std::chrono::steady_clock::now();
    gpu_ok = gpu_bridge_->upload(image, gpu_error);
    gpu_duration_ms =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() -
                                                  gpu_start)
            .count();
  } else {
    gpu_ok = false;
    gpu_error = "GPU bridge unavailable.";
  }

  if (catalog_service_ && descriptor.file_id.has_value()) {
    const auto state =
        gpu_ok ? PreviewState::kGpuResident : PreviewState::kCached;
    catalog_service_->updatePreviewState(*descriptor.file_id,
                                         static_cast<int>(state));
  }

  emitEvent(descriptor,
            CacheTier::kRam,
            false,
            !gpu_ok,
            gpu_error,
            backend_name,
            gpu_duration_ms,
            transform_duration);
}

void PreviewService::emitEvent(const PreviewDescriptor& descriptor,
                               CacheTier tier,
                               bool hit,
                               bool error,
                               const std::string& message,
                               const std::string& backend,
                               double gpu_ms,
                               double transform_ms) {
  if (!event_sink_) {
    return;
  }
  CacheEvent event;
  event.root_id = descriptor.root_id;
  event.relative_path = descriptor.relative_path;
  event.tier = tier;
  event.hit = hit;
  event.error = error;
  event.error_message = message;
  event.backend = backend;
  event.gpu_upload_ms = gpu_ms;
  event.color_transform_ms = transform_ms;
  event_sink_(event);
}

std::vector<std::uint8_t> PreviewService::loadEmbeddedProfile(
    const PreviewDescriptor& descriptor) const {
  if (const auto embedded =
          icc::ExtractEmbeddedProfile(descriptor.absolute_path);
      !embedded.empty()) {
    return embedded;
  }

  static const std::array<const char*, 3> extensions{".icc", ".ICM", ".profile"};
  for (const auto* ext : extensions) {
    auto candidate = descriptor.absolute_path;
    candidate.replace_extension(ext);
    if (std::filesystem::exists(candidate)) {
      std::ifstream stream(candidate, std::ios::binary);
      if (!stream) {
        continue;
      }
      std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(stream)),
                                      std::istreambuf_iterator<char>());
      if (!bytes.empty()) {
        return bytes;
      }
    }
  }
  return {};
}

std::string PreviewService::backendLabel(
    const cataloger::platform::gpu::GpuBridge* bridge) {
  if (!bridge) {
    return "none";
  }
  switch (bridge->backend()) {
    case cataloger::platform::gpu::Backend::kMetal:
      return "Metal";
    default:
      return "Stub";
  }
}

}  // namespace cataloger::services::preview
