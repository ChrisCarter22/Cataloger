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

if(APPLE)
  set(_cataloger_homebrew_roots
      /opt/homebrew/opt/icu4c
      /opt/homebrew/opt/icu4c@78)
  foreach(_root IN LISTS _cataloger_homebrew_roots)
    if(EXISTS "${_root}")
      list(APPEND CMAKE_PREFIX_PATH "${_root}")
      if(EXISTS "${_root}/lib/pkgconfig")
        if(DEFINED ENV{PKG_CONFIG_PATH} AND NOT "$ENV{PKG_CONFIG_PATH}" STREQUAL "")
          set(ENV{PKG_CONFIG_PATH}
              "$ENV{PKG_CONFIG_PATH}:${_root}/lib/pkgconfig")
        else()
          set(ENV{PKG_CONFIG_PATH} "${_root}/lib/pkgconfig")
        endif()
      endif()
    endif()
  endforeach()
endif()

find_package(Qt6 COMPONENTS ${CATALOGER_QT_COMPONENTS} QUIET)
find_package(SQLite3 QUIET)
find_package(ICU COMPONENTS i18n uc QUIET)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(LCMS2 QUIET lcms2)
endif()
if(NOT LCMS2_FOUND)
  find_path(LCMS2_INCLUDE_DIR lcms2.h PATH_SUFFIXES lcms2)
  find_library(LCMS2_LIBRARY NAMES lcms2)
  if(LCMS2_INCLUDE_DIR AND LCMS2_LIBRARY)
    set(LCMS2_FOUND TRUE)
    set(LCMS2_INCLUDE_DIRS ${LCMS2_INCLUDE_DIR})
    set(LCMS2_LINK_LIBRARIES ${LCMS2_LIBRARY})
  endif()
endif()
if(LCMS2_FOUND AND NOT TARGET LCMS2::LCMS2)
  add_library(LCMS2::LCMS2 INTERFACE IMPORTED)
  target_include_directories(LCMS2::LCMS2 INTERFACE ${LCMS2_INCLUDE_DIRS})
  target_link_libraries(LCMS2::LCMS2 INTERFACE ${LCMS2_LINK_LIBRARIES})
endif()

if(NOT Qt6_FOUND)
  message(STATUS "Qt6 not found; UI targets will remain in stub mode until Qt6 is available.")
endif()

if(NOT SQLite3_FOUND)
  message(
    FATAL_ERROR
      "SQLite3 development files are required for the catalog service. Install the SQLite3 SDK and retry."
  )
endif()

if(NOT ICU_FOUND)
  message(STATUS "ICU not found; i18n features are disabled in the bootstrap build.")
endif()

# Ensure the color pipeline dependency is available.
if(NOT LCMS2_FOUND)
  message(
    FATAL_ERROR
      "LittleCMS (lcms2) is required for the preview color pipeline. Install the SDK and retry."
  )
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
