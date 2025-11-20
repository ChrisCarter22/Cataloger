add_executable(viewer_metal_tests MetalViewerTests.mm)
target_link_libraries(
  viewer_metal_tests
  PRIVATE
    cataloger_viewer
    GTest::gtest_main)
target_compile_features(viewer_metal_tests PRIVATE cxx_std_20)

add_test(NAME viewer_metal_tests COMMAND viewer_metal_tests)

