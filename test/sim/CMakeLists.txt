# The application side of the simulated world, nothing uses it yet
add_library(sim_client OBJECT client.c)

target_link_libraries(sim_client PRIVATE pcp pcp_warnings)

# What carries datagrams in the simulated world, nothing uses it yet either
add_library(sim_network OBJECT network.c)

target_link_libraries(sim_network PRIVATE pcp unity pcp_warnings)
