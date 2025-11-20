add_library(cataloger_catalog STATIC CatalogService.cpp)
target_include_directories(
  cataloger_catalog
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(cataloger_catalog PUBLIC cxx_std_20)
