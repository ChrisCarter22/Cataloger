add_library(cataloger_metadata STATIC MetadataService.cpp)
target_include_directories(
  cataloger_metadata
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(cataloger_metadata PUBLIC cxx_std_20)
