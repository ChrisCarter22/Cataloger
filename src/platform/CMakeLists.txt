add_library(cataloger_platform STATIC PlatformContext.cpp)
target_include_directories(
  cataloger_platform
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(cataloger_platform PUBLIC cxx_std_20)
