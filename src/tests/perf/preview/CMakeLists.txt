add_executable(preview_pipeline_perf PreviewPipelinePerf.cpp)
target_link_libraries(
  preview_pipeline_perf
  PRIVATE
    cataloger_catalog
    cataloger_preview
    GTest::gtest_main)
target_compile_features(preview_pipeline_perf PRIVATE cxx_std_20)

add_test(NAME preview_pipeline_perf COMMAND preview_pipeline_perf)
