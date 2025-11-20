add_library(
  cataloger_preview
  STATIC
    DirectoryScanner.cpp
    GpuBridge.cpp
    PreviewCache.cpp
    PreviewExtractor.cpp
    PreviewService.cpp)
target_include_directories(
  cataloger_preview
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src)
target_compile_features(cataloger_preview PUBLIC cxx_std_20)
target_link_libraries(
  cataloger_preview
  PUBLIC
    cataloger_catalog)
