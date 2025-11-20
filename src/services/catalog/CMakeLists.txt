add_library(cataloger_catalog STATIC CatalogService.cpp)
target_include_directories(
  cataloger_catalog
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src)
target_compile_features(cataloger_catalog PUBLIC cxx_std_20)
target_link_libraries(
  cataloger_catalog
  PUBLIC
    SQLite::SQLite3)
