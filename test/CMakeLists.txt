include(FetchContent)

# Keeps Unity's own install rules out of cmake --install, which CMake can only
# do from 3.28
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
  set(PCP_UNITY_EXCLUDE EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(
  unity
  GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
  GIT_TAG v2.7.0
  ${PCP_UNITY_EXCLUDE}
)
FetchContent_MakeAvailable(unity)

# A test executable built from sources in the calling directory, plus the runner
# they all share
function(pcp_add_test_executable target)
  add_executable(${target} ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/runner.c ${ARGN})

  target_link_libraries(${target}
    PRIVATE
      pcp
      unity)

  target_include_directories(${target}
    PRIVATE
      ${CMAKE_CURRENT_FUNCTION_LIST_DIR})

  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${target}
      PRIVATE
        -fsanitize=address
        -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address)
  elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${target} PRIVATE /fsanitize=address PRIVATE /Zi)
  else()
    message(WARNING "ASan not configured for compiler: ${CMAKE_C_COMPILER_ID}")
  endif()

  # one ctest test per RUN_TEST(...) in the test sources, so tests only need to
  # be registered in their file's *_tests() function
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${ARGN})

  foreach(source IN LISTS ARGN)
    file(STRINGS ${source} lines REGEX "RUN_TEST\\([a-z0-9_]+\\);")
    foreach(line IN LISTS lines)
      string(REGEX REPLACE ".*RUN_TEST\\(([a-z0-9_]+)\\);.*" "\\1" test_case "${line}")
      add_test(NAME ${test_case} COMMAND ${target} ${test_case})
    endforeach()
  endforeach()
endfunction()

add_subdirectory(unit)
add_subdirectory(sim)

# Checks that every packet the generated code decodes encodes back to the same
# bytes, and that every message it builds decodes again. Run it with
# ./build/test/pcp_fuzz -max_total_time=60
if(PCP_BUILD_FUZZER)
  if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "PCP_BUILD_FUZZER needs clang for libFuzzer")
  endif()

  add_executable(pcp_fuzz pcp_fuzz.c ${PROJECT_SOURCE_DIR}/lib/pcp/pcp_gen.c)
  target_compile_features(pcp_fuzz PRIVATE c_std_23)
  target_include_directories(pcp_fuzz PRIVATE ${PROJECT_SOURCE_DIR}/lib/pcp)
  target_compile_options(pcp_fuzz
    PRIVATE
      -g
      -fsanitize=fuzzer,address,undefined)
  target_link_options(pcp_fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
  # pcp_fuzz.c is regenerated along with the library's sources
  add_dependencies(pcp_fuzz pcp)
endif()
