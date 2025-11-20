#include "Application.h"

#include <filesystem>
#include <string>
#include <vector>

#include "services/catalog/CatalogService.h"
#include "services/delivery/DeliveryService.h"
#include "services/ingest/IngestService.h"
#include "services/metadata/MetadataService.h"
#include "services/preview/PreviewService.h"
#include "services/tasks/TaskScheduler.h"

namespace cataloger::app {

Application::Application() = default;

void Application::bootstrap() {
  settings_.loadDefaults();
  platform_.detectHostEnvironment();

  services::catalog::CatalogService catalog_service;
  const auto db_path =
      std::filesystem::temp_directory_path() / "cataloger_bootstrap.db";
  catalog_service.configureDatabase(db_path);
  catalog_service.initializeSchema();
  const auto root_path = std::filesystem::current_path();
  const auto root_id = catalog_service.registerRoot(root_path);
  const auto snapshot = catalog_service.scanRoot(root_path);
  catalog_service.ingestRecords(root_id, snapshot);
  catalog_service.enqueueSyncEvent(root_id, "", "bootstrap", "{}");
  auto pending_events = catalog_service.pendingSyncEvents();
  if (!pending_events.empty()) {
    catalog_service.markSyncEventProcessed(pending_events.front().id);
    pending_events = catalog_service.pendingSyncEvents();
  }
  const auto stored_files = catalog_service.listFiles(root_id);

  services::preview::PreviewService preview_service;
  preview_service.primeCaches(2);

  services::ingest::IngestService ingest_service;
  ingest_service.queueSources({});

  services::metadata::MetadataService metadata_service;
  metadata_service.applyTemplate("bootstrap");

  services::delivery::DeliveryService delivery_service;
  delivery_service.configureEndpoint("localhost");

  services::tasks::TaskScheduler scheduler;
  scheduler.schedule("bootstrap");

  [[maybe_unused]] const auto catalog_size = stored_files.size();
  [[maybe_unused]] const auto cached_neighbors = preview_service.cachedNeighbors();
  [[maybe_unused]] const auto queued_sources = ingest_service.sources().size();
  [[maybe_unused]] const auto last_template = metadata_service.lastTemplate();
  [[maybe_unused]] const auto endpoint = delivery_service.endpoint();
  [[maybe_unused]] const auto scheduled_tasks = scheduler.size();
  [[maybe_unused]] const auto sync_backlog = pending_events.size();
  [[maybe_unused]] const auto platform_name = platform_.displayName();
  [[maybe_unused]] const auto profile = settings_.activeProfile();
  (void)catalog_size;
  (void)cached_neighbors;
  (void)queued_sources;
  (void)last_template;
  (void)endpoint;
  (void)scheduled_tasks;
  (void)sync_backlog;
  (void)platform_name;
  (void)profile;
}

int Application::run() {
  bootstrap();
  return 0;
}

}  // namespace cataloger::app
