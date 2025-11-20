#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "services/catalog/CatalogService.h"
#include "services/preview/PreviewService.h"

using cataloger::services::catalog::CatalogService;
using cataloger::services::preview::PreviewService;

namespace {

std::string uniqueSuffix() {
  const auto ticks =
      std::chrono::steady_clock::now().time_since_epoch().count();
  return std::to_string(ticks);
}

void writeFile(const std::filesystem::path& path, std::size_t size) {
  std::ofstream stream(path, std::ios::binary);
  std::vector<char> payload(size, 'a');
  stream.write(payload.data(), payload.size());
}

}  // namespace

TEST(PreviewPipelinePerf, WarmRootUnderBudget) {
  const auto suffix = uniqueSuffix();
  const auto root_path =
      std::filesystem::temp_directory_path() / ("preview_perf_root_" + suffix);
  const auto db_path =
      std::filesystem::temp_directory_path() / ("preview_perf_db_" + suffix + ".db");
  std::filesystem::create_directories(root_path);

  std::vector<std::string> filenames;
  for (int i = 0; i < 25; ++i) {
    const auto name = "SHOT_" + std::to_string(i).append(".JPG");
    filenames.push_back(name);
    writeFile(root_path / name, 4096 + i * 32);
  }

  CatalogService catalog;
  catalog.configureDatabase(db_path);
  catalog.initializeSchema();
  const auto root_id = catalog.registerRoot(root_path);
  const auto records = catalog.scanRoot(root_path);
  catalog.ingestRecords(root_id, records);

  PreviewService preview(32, 8, 2);
  preview.setCatalogService(&catalog);
  double gpu_total_ms = 0.0;
  double gpu_max_ms = 0.0;
  std::size_t gpu_samples = 0;
  preview.setEventSink([&](const cataloger::services::preview::CacheEvent& event) {
    std::cout << "[perf] cache event path=" << event.relative_path
              << " tier=" << (event.hit ? "RAM-hit" : "RAM-miss")
              << " backend=" << event.backend
              << " gpu=" << event.gpu_upload_ms << "ms\n";
    if (event.error) {
      std::cout << "        error=" << event.error_message << "\n";
    }
    if (event.gpu_upload_ms > 0.0) {
      gpu_total_ms += event.gpu_upload_ms;
      gpu_max_ms = std::max(gpu_max_ms, event.gpu_upload_ms);
      ++gpu_samples;
    }
  });

  const auto start = std::chrono::steady_clock::now();
  preview.warmRoot(root_id, root_path);
  preview.waitUntilIdle();
  const auto warm_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - start)
          .count();

  std::cout << "[perf] warmRoot duration: " << warm_duration << " ms\n";
  EXPECT_LT(warm_duration, 400);

  const auto preview_start = std::chrono::steady_clock::now();
  preview.requestPreview(root_id, filenames.back());
  preview.waitUntilIdle();
  const auto request_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - preview_start)
          .count();
  std::cout << "[perf] request duration: " << request_duration << " ms\n";
  EXPECT_LT(request_duration, 200);

  if (gpu_samples > 0) {
    const auto gpu_avg = gpu_total_ms / static_cast<double>(gpu_samples);
    std::cout << "[perf] gpu upload avg=" << gpu_avg
              << " ms (max=" << gpu_max_ms << " ms)\n";
    EXPECT_LT(gpu_max_ms, 150.0);
  }

  std::error_code ec;
  std::filesystem::remove(db_path, ec);
  std::filesystem::remove_all(root_path, ec);
}
