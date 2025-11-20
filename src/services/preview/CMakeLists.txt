add_library(cataloger_preview STATIC PreviewService.cpp)
target_include_directories(
  cataloger_preview
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(cataloger_preview PUBLIC cxx_std_20)
