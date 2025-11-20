add_executable(preview_service_tests PreviewServiceTests.cpp)
target_link_libraries(
  preview_service_tests
  PRIVATE
    cataloger_preview
    GTest::gtest_main
    LCMS2::LCMS2)
target_compile_features(preview_service_tests PRIVATE cxx_std_20)

add_test(NAME preview_service_tests COMMAND preview_service_tests)
