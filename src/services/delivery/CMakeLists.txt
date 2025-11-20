add_library(cataloger_delivery STATIC DeliveryService.cpp)
target_include_directories(
  cataloger_delivery
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(cataloger_delivery PUBLIC cxx_std_20)
