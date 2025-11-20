add_library(cataloger_ingest STATIC IngestService.cpp)
target_include_directories(
  cataloger_ingest
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(cataloger_ingest PUBLIC cxx_std_20)
