add_executable(preview_service_tests PreviewServiceTests.cpp)
target_link_libraries(
  preview_service_tests
  PRIVATE
    cataloger_preview
    GTest::gtest_main
    LCMS2::LCMS2)
target_compile_features(preview_service_tests PRIVATE cxx_std_20)

add_test(NAME preview_service_tests COMMAND preview_service_tests)

add_executable(icc_profile_extractor_tests IccProfileExtractorTests.cpp)
target_link_libraries(
  icc_profile_extractor_tests
  PRIVATE
    cataloger_preview
    GTest::gtest_main)
target_compile_features(icc_profile_extractor_tests PRIVATE cxx_std_20)
target_compile_definitions(
  icc_profile_extractor_tests
  PRIVATE
    CATALOGER_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

add_test(NAME icc_profile_extractor_tests COMMAND icc_profile_extractor_tests)
