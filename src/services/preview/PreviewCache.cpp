#include "PreviewCache.h"

namespace cataloger::services::preview {

PreviewCache::LruCache::LruCache(std::size_t capacity) : capacity_(capacity) {}

void PreviewCache::LruCache::store(const PreviewImage& image) {
  if (capacity_ == 0) {
    return;
  }

  auto existing = entries_.find(image.cache_key);
  if (existing != entries_.end()) {
    existing->second.first = image;
    order_.splice(order_.begin(), order_, existing->second.second);
    existing->second.second = order_.begin();
    return;
  }

  order_.push_front(image.cache_key);
  entries_.insert({image.cache_key, {image, order_.begin()}});

  if (entries_.size() > capacity_) {
    const auto last_key = order_.back();
    order_.pop_back();
    entries_.erase(last_key);
  }
}

std::optional<PreviewImage> PreviewCache::LruCache::get(
    const std::string& key) const {
  auto it = entries_.find(key);
  if (it == entries_.end()) {
    return std::nullopt;
  }
  order_.splice(order_.begin(), order_, it->second.second);
  it->second.second = order_.begin();
  return it->second.first;
}

std::size_t PreviewCache::LruCache::size() const {
  return entries_.size();
}

PreviewCache::PreviewCache(std::size_t ram_capacity, std::size_t preload_capacity)
    : ram_cache_(ram_capacity), preload_cache_(preload_capacity) {}

void PreviewCache::put(const PreviewImage& image, CacheTier tier) {
  if (tier == CacheTier::kRam) {
    ram_cache_.store(image);
  } else {
    preload_cache_.store(image);
  }
}

std::optional<PreviewImage> PreviewCache::get(const std::string& key) const {
  if (auto ram = ram_cache_.get(key)) {
    return ram;
  }
  return preload_cache_.get(key);
}

std::size_t PreviewCache::ramSize() const {
  return ram_cache_.size();
}

std::size_t PreviewCache::preloadSize() const {
  return preload_cache_.size();
}

}  // namespace cataloger::services::preview
