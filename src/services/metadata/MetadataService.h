#pragma once

#include <string>
#include <string_view>

namespace cataloger::services::metadata {

class MetadataService {
public:
  void applyTemplate(std::string_view name);
  [[nodiscard]] std::string lastTemplate() const;

private:
  std::string last_template_;
};

}  // namespace cataloger::services::metadata
