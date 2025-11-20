add_library(cataloger_config STATIC Settings.cpp)
target_include_directories(
  cataloger_config
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(cataloger_config PUBLIC cxx_std_20)
