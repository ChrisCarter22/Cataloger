#pragma once

namespace cataloger::services::catalog {

class CatalogService {
public:
  CatalogService();

  void initialize();
  [[nodiscard]] bool isReady() const noexcept;

private:
  bool ready_;
};

}  // namespace cataloger::services::catalog
