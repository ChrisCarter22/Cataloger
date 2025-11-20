#include "IngestService.h"

namespace cataloger::services::ingest {

void IngestService::queueSources(PathList paths) {
  sources_ = std::move(paths);
}

const IngestService::PathList& IngestService::sources() const noexcept {
  return sources_;
}

}  // namespace cataloger::services::ingest
