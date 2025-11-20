add_library(cataloger_tasks STATIC TaskScheduler.cpp)
target_include_directories(
  cataloger_tasks
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(cataloger_tasks PUBLIC cxx_std_20)
