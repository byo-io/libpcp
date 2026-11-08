cmake_minimum_required(VERSION 3.23)

project(pcp VERSION 1.0.0 LANGUAGES C)

# The example is only built by default when libpcp isn't pulled in by another
# project (e.g. through FetchContent)
option(PCP_BUILD_EXAMPLE "Build the example client" ${PROJECT_IS_TOP_LEVEL})
option(PCP_BUILD_FUZZER "Build the libFuzzer harness for the wire format (clang)"
       OFF)
# Off by default, so a newer compiler's warnings don't break the build. CI turns
# it on.
option(PCP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(PCP_INSTALL "Generate the install rules" ${PROJECT_IS_TOP_LEVEL})

include(cmake/warnings.cmake)

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ${PCP_WARNINGS_AS_ERRORS})

# BUILD_TESTING comes from CTest. A project pulling in libpcp never builds its
# tests, even if it builds its own. lib/ needs to know, since it builds a copy
# of the library for the tests.
if(PROJECT_IS_TOP_LEVEL)
  include(CTest)
endif()
if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
  set(PCP_BUILD_TESTS ON)
else()
  set(PCP_BUILD_TESTS OFF)
endif()

add_subdirectory(lib)

if(PCP_BUILD_EXAMPLE)
  add_subdirectory(example)
endif()

if(PCP_BUILD_TESTS)
  add_subdirectory(test)
endif()
