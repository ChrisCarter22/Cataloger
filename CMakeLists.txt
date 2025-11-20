cmake_minimum_required(VERSION 3.24)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds are disabled. Please run cmake from a separate build directory."
  )
endif()

project(
  Cataloger
  VERSION 0.1.0
  LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CATALOGER_QT_COMPONENTS Core Gui Widgets)

find_package(Qt6 COMPONENTS ${CATALOGER_QT_COMPONENTS} QUIET)
find_package(SQLite3 QUIET)
find_package(Vulkan QUIET)
find_package(ICU COMPONENTS i18n uc QUIET)

if(NOT Qt6_FOUND)
  message(STATUS "Qt6 not found; UI targets will remain in stub mode until Qt6 is available.")
endif()

if(NOT SQLite3_FOUND)
  message(
    FATAL_ERROR
      "SQLite3 development files are required for the catalog service. Install the SQLite3 SDK and retry."
  )
endif()

if(NOT Vulkan_FOUND)
  message(
    STATUS
      "Vulkan SDK not found; GPU preview pipeline targets will be enabled once the SDK is installed."
  )
endif()

if(NOT ICU_FOUND)
  message(STATUS "ICU not found; i18n features are disabled in the bootstrap build.")
endif()

option(CATALOGER_ENABLE_TESTS "Enable building Cataloger tests" ON)

if(CATALOGER_ENABLE_TESTS)
  include(CTest)
  enable_testing()
  find_package(GTest CONFIG QUIET)
  if(NOT GTest_FOUND)
    message(
      FATAL_ERROR
        "GoogleTest not found. Install it via Homebrew (`brew install googletest`) or provide a CMake package."
    )
  endif()
endif()

add_subdirectory(src)

if(CATALOGER_ENABLE_TESTS)
  add_subdirectory(src/tests)
endif()
