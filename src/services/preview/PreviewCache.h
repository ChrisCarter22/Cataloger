#pragma once

#include <list>
#include <optional>
#include <string>
#include <unordered_map>

#include "PreviewTypes.h"

namespace cataloger::services::preview {

class PreviewCache {
public:
  PreviewCache(std::size_t ram_capacity, std::size_t preload_capacity);

  void put(const PreviewImage& image, CacheTier tier);
  [[nodiscard]] std::optional<PreviewImage> get(const std::string& key) const;
  [[nodiscard]] std::size_t ramSize() const;
  [[nodiscard]] std::size_t preloadSize() const;

private:
  class LruCache {
  public:
    explicit LruCache(std::size_t capacity);

    void store(const PreviewImage& image);
    [[nodiscard]] std::optional<PreviewImage> get(const std::string& key) const;
    [[nodiscard]] std::size_t size() const;

  private:
    std::size_t capacity_;
    mutable std::list<std::string> order_;
    mutable std::unordered_map<std::string,
                               std::pair<PreviewImage, std::list<std::string>::iterator>>
        entries_;
  };

  LruCache ram_cache_;
  LruCache preload_cache_;
};

}  // namespace cataloger::services::preview
