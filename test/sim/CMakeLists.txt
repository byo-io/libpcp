# The application side of the simulated world, nothing uses it yet
add_library(sim_client OBJECT client.c)

target_link_libraries(sim_client PRIVATE pcp pcp_warnings)

# What carries datagrams in the simulated world, nothing uses it yet either
add_library(sim_network OBJECT network.c)

target_link_libraries(sim_network PRIVATE pcp unity pcp_warnings)

# A PCP server for the simulated world, nothing uses it yet either. Unlike the
# rest it shares the library's generated wire format.
add_library(sim_server OBJECT server.c)

target_link_libraries(sim_server PRIVATE pcp unity pcp_warnings)

set_source_files_properties(server.c
  PROPERTIES INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/lib/pcp)

# What checks the simulated application's view, nothing uses it yet either
add_library(sim_observer OBJECT observer.c)

target_link_libraries(sim_observer PRIVATE pcp unity pcp_warnings)
