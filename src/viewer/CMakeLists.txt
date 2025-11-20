add_library(
  cataloger_viewer
  STATIC
    ViewerContract.h
    metal/MetalViewer.mm
    vulkan/VulkanViewerStub.cpp)
target_include_directories(
  cataloger_viewer
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src)
target_compile_features(cataloger_viewer PUBLIC cxx_std_20)

if(APPLE)
  target_link_libraries(
    cataloger_viewer
    PUBLIC
      "-framework Metal"
      "-framework QuartzCore")
endif()
