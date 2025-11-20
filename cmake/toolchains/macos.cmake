set(CMAKE_SYSTEM_NAME Darwin)

if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
    set(_cataloger_default_arch "arm64")
  else()
    set(_cataloger_default_arch "x86_64")
  endif()
  set(CMAKE_OSX_ARCHITECTURES "${_cataloger_default_arch}" CACHE STRING "" FORCE)
endif()

set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "" FORCE)

if(NOT DEFINED CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER clang CACHE STRING "" FORCE)
endif()

if(NOT DEFINED CMAKE_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER clang++ CACHE STRING "" FORCE)
endif()
