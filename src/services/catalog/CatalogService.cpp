#include "CatalogService.h"

namespace cataloger::services::catalog {

CatalogService::CatalogService() : ready_(false) {}

void CatalogService::initialize() {
  ready_ = true;
}

bool CatalogService::isReady() const noexcept {
  return ready_;
}

}  // namespace cataloger::services::catalog
