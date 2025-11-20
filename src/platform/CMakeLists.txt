set(PLATFORM_SOURCES
    PlatformContext.cpp
    gpu/GpuBridgeFactory.cpp)

if(APPLE)
  list(APPEND PLATFORM_SOURCES gpu/MetalBridge.mm)
endif()

add_library(
  cataloger_platform
  STATIC
    ${PLATFORM_SOURCES})
target_include_directories(
  cataloger_platform
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src)
target_compile_features(cataloger_platform PUBLIC cxx_std_20)

if(APPLE)
  target_link_libraries(
    cataloger_platform
    PUBLIC
      "-framework Metal"
      "-framework Foundation")
endif()
