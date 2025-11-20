add_executable(catalog_catalog_tests CatalogServiceTests.cpp)
target_link_libraries(
  catalog_catalog_tests
  PRIVATE
    cataloger_catalog
    GTest::gtest_main)
target_compile_features(catalog_catalog_tests PRIVATE cxx_std_20)

add_test(NAME catalog_catalog_tests COMMAND catalog_catalog_tests)
