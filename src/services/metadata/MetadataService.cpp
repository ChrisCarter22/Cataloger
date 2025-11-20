#include "MetadataService.h"

namespace cataloger::services::metadata {

void MetadataService::applyTemplate(std::string_view name) {
  last_template_.assign(name);
}

std::string MetadataService::lastTemplate() const {
  return last_template_;
}

}  // namespace cataloger::services::metadata
