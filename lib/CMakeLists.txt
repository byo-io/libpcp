# The library's sources and settings, shared by the library itself and the
# static copy the tests use
function(pcp_add_library target)
  add_library(${target} ${ARGN})

  # The library is C23, its headers only need C11
  target_compile_features(${target}
    PUBLIC c_std_11
    PRIVATE c_std_23)

  target_compile_definitions(${target}
    PRIVATE
      _POSIX_C_SOURCE=200112L)

  target_sources(${target}
    PRIVATE
      ip.c
      mapping.c
      schedule.c
      session.c
      pcp/epoch.c
      pcp/wire.c
      pcp/pcp_gen.c)

  target_sources(${target}
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}/include
      FILES
        ${PROJECT_SOURCE_DIR}/include/byo/pcp/compat.h
        ${PROJECT_SOURCE_DIR}/include/byo/pcp/ip.h
        ${PROJECT_SOURCE_DIR}/include/byo/pcp/pcp.h)

  target_sources(${target}
    PRIVATE
      FILE_SET private_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
          ${CMAKE_CURRENT_SOURCE_DIR}/duration.h
          ${CMAKE_CURRENT_SOURCE_DIR}/mapping.h
          ${CMAKE_CURRENT_SOURCE_DIR}/schedule.h
          ${CMAKE_CURRENT_SOURCE_DIR}/session.h
          ${CMAKE_CURRENT_SOURCE_DIR}/view.h
          ${CMAKE_CURRENT_SOURCE_DIR}/pcp/epoch.h
          ${CMAKE_CURRENT_SOURCE_DIR}/pcp/pcp_gen.h
          ${CMAKE_CURRENT_SOURCE_DIR}/pcp/wire.h)

  # Only while building, so the installed library doesn't need the warnings
  # target
  target_link_libraries(${target} PRIVATE $<BUILD_INTERFACE:pcp_warnings>)
endfunction()

pcp_add_library(pcp)
add_library(byo::pcp ALIAS pcp)

# The tests also call the library's internal functions, which the shared
# library doesn't export
if(PCP_BUILD_TESTS)
  pcp_add_library(pcp_testing STATIC)
endif()

# The wire format is generated from pcp/pcp.pkt by pktc
# (https://github.com/davidlwesten/pktc). The output is checked in, so pktc is
# only needed to change the schema. When it's installed, editing pcp/pcp.pkt
# regenerates the code as part of the build, along with pktc's libFuzzer
# harness for it (test/pcp_fuzz.c).
find_program(PKTC_EXECUTABLE pktc)
if(PKTC_EXECUTABLE)
  # pktc writes everything into one directory, so generate into the build tree
  # and copy the harness to the tests
  set(PKTC_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pktc)
  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_SOURCE_DIR}/pcp/pcp_gen.c
      ${CMAKE_CURRENT_SOURCE_DIR}/pcp/pcp_gen.h
      ${PROJECT_SOURCE_DIR}/test/pcp_fuzz.c
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PKTC_OUTPUT}
    COMMAND ${PKTC_EXECUTABLE} --fuzz -o ${PKTC_OUTPUT}
            ${CMAKE_CURRENT_SOURCE_DIR}/pcp/pcp.pkt
    COMMAND ${CMAKE_COMMAND} -E copy ${PKTC_OUTPUT}/pcp_gen.c
            ${PKTC_OUTPUT}/pcp_gen.h ${CMAKE_CURRENT_SOURCE_DIR}/pcp
    COMMAND ${CMAKE_COMMAND} -E copy ${PKTC_OUTPUT}/pcp_fuzz.c
            ${PROJECT_SOURCE_DIR}/test
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pcp/pcp.pkt
    COMMENT "Generating the PCP wire format from pcp/pcp.pkt"
    VERBATIM)
endif()

# Installed as libbyo-pcp, since other projects already use libpcp. A shared
# library's soname only changes with the major version.
# Only functions marked with PCP_EXPORT are exported.
set_target_properties(pcp PROPERTIES
  OUTPUT_NAME byo-pcp
  C_VISIBILITY_PRESET hidden
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR})

# Installs the library and its headers, a package so other projects can
# find_package(byo-pcp) and link to byo::pcp, and a pkg-config file
if(PCP_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  set(PCP_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/byo-pcp)

  install(TARGETS pcp
    EXPORT byo-pcpTargets
    FILE_SET HEADERS)
  install(EXPORT byo-pcpTargets
    NAMESPACE byo::
    DESTINATION ${PCP_CMAKE_DIR})

  configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/byo-pcpConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/byo-pcpConfig.cmake
    INSTALL_DESTINATION ${PCP_CMAKE_DIR})
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/byo-pcpConfigVersion.cmake
    COMPATIBILITY SameMajorVersion)
  install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/byo-pcpConfig.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/byo-pcpConfigVersion.cmake
    DESTINATION ${PCP_CMAKE_DIR})

  # The paths are absolute, from CMAKE_INSTALL_PREFIX, so pkg-config can tell
  # when they're system directories and leave them out
  configure_file(${PROJECT_SOURCE_DIR}/cmake/byo-pcp.pc.in
                 ${CMAKE_CURRENT_BINARY_DIR}/byo-pcp.pc @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/byo-pcp.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()
