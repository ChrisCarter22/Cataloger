#pragma once

#include <string>
#include <vector>

namespace cataloger::services::ingest {

class IngestService {
public:
  using PathList = std::vector<std::string>;

  void queueSources(PathList paths);
  [[nodiscard]] const PathList& sources() const noexcept;

private:
  PathList sources_;
};

}  // namespace cataloger::services::ingest
