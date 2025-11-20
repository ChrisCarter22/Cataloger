add_library(
  cataloger_appcore
  STATIC
    Application.cpp)
target_include_directories(
  cataloger_appcore
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(
  cataloger_appcore
  PUBLIC
    cataloger_config
    cataloger_platform
    cataloger_catalog
    cataloger_preview
    cataloger_ingest
    cataloger_metadata
    cataloger_delivery
    cataloger_tasks)
target_compile_features(cataloger_appcore PUBLIC cxx_std_20)

add_executable(cataloger main.cpp)
target_link_libraries(
  cataloger
  PRIVATE
    cataloger_appcore)

if(Qt6_FOUND)
  target_link_libraries(
    cataloger
    PRIVATE
      Qt6::Core
      Qt6::Gui
      Qt6::Widgets)
endif()
