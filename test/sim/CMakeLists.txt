# The client end to end in a simulated world (see sim.h). It gets no access to
# the library's private headers, so it can only do what an application can. The
# exception is the simulated server, which shares the generated wire format.
pcp_add_test_executable(sim_tests
  client.c
  main.c
  network.c
  observer.c
  scenarios.c
  server.c
  sim.c)

target_link_libraries(sim_tests PRIVATE pcp_warnings)

set_source_files_properties(server.c
  PROPERTIES INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/lib/pcp)
