add_library(pcp)
add_library(pcp::pcp ALIAS pcp)

target_compile_features(pcp PUBLIC c_std_23)

target_compile_definitions(pcp
  PRIVATE
    _POSIX_C_SOURCE=200112L)

target_sources(pcp
  PRIVATE
    ip.c
    mapping.c
    schedule.c
    session.c
    pcp/epoch.c
    pcp/wire.c
    pcp/pcp_gen.c)

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

target_sources(pcp
  PUBLIC
  FILE_SET HEADERS
  BASE_DIRS ${PROJECT_SOURCE_DIR}/include
    FILES
      ${PROJECT_SOURCE_DIR}/include/pcp/ip.h
      ${PROJECT_SOURCE_DIR}/include/pcp/pcp.h)

target_sources(pcp
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
target_link_libraries(pcp PRIVATE $<BUILD_INTERFACE:pcp_warnings>)

# A shared library's soname only changes with the major version
set_target_properties(pcp PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR})

# Installs the library and its headers, and a package so other projects can
# find_package(pcp) and link to pcp::pcp
if(PCP_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  set(PCP_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/pcp)

  install(TARGETS pcp
    EXPORT pcpTargets
    FILE_SET HEADERS)
  install(EXPORT pcpTargets
    NAMESPACE pcp::
    DESTINATION ${PCP_CMAKE_DIR})

  configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/pcpConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/pcpConfig.cmake
    INSTALL_DESTINATION ${PCP_CMAKE_DIR})
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/pcpConfigVersion.cmake
    COMPATIBILITY SameMajorVersion)
  install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/pcpConfig.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/pcpConfigVersion.cmake
    DESTINATION ${PCP_CMAKE_DIR})
endif()
