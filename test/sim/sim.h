#pragma once

#include <pcp/pcp.h>

#include "network.h"
#include "server.h"

// A simulated world for testing the client end to end. This is what scenarios
// use, and it owns virtual time and connects the pieces:
//
// - client.h, a generic application loop around the library's public API
// - network.h, what carries datagrams between the client and the server
// - server.h, a PCP server with real leases
// - observer.h, which checks the application only hears what pcp.h promises
//
// There's one world at a time, so it can be cleaned up and reported on after a
// failed test unwinds (see sim_teardown).

static constexpr sim_duration_ms_t SIM_SECOND = 1000;
static constexpr sim_duration_ms_t SIM_MINUTE = 60 * SIM_SECOND;
static constexpr sim_duration_ms_t SIM_HOUR = 60 * SIM_MINUTE;

typedef struct sim_config {
  /// Where all of the world's randomness comes from
  uint64_t seed;
} sim_config_t;

/**
 * @brief Start a world
 */
void sim_begin(sim_config_t config);

/**
 * @brief Tear the world down. Safe to call without one.
 */
void sim_end();

/**
 * @brief Report the world after a failed test and end it, for tearDown
 */
void sim_teardown();

/**
 * @brief Let `duration` of virtual time pass
 */
void sim_run_for(sim_duration_ms_t duration);

/**
 * @brief Request a mapping like the application would
 */
pcp_handle_t
sim_map(pcp_time_s_t lifetime, pcp_protocol_t protocol, port_t port);

/**
 * @brief Release a mapping like the application would
 */
void sim_release(pcp_handle_t handle);

/**
 * @returns The address the application was told about for `handle`
 */
socket_address_t sim_external(pcp_handle_t handle);

/**
 * @returns What the server has done so far
 */
sim_server_stats_t sim_stats();

/**
 * @brief Check the application thinks `handle` is mapped, and that the server
 *        agrees on the address
 */
void sim_assert_mapped(pcp_handle_t handle);

/**
 * @brief Check the server has no lease left for what `handle` asked for
 */
void sim_assert_no_lease(pcp_handle_t handle);

/**
 * @brief Check nothing is in flight and the client has nothing scheduled
 */
void sim_assert_idle();

/**
 * @brief Check these are all the events `handle` got, not counting sends
 */
void sim_assert_events(pcp_handle_t handle,
                       size_t count,
                       int const events[count]);

#define SIM_ASSERT_EVENTS(handle, ...)                                         \
  sim_assert_events((handle),                                                  \
                    sizeof((int const[]){__VA_ARGS__}) / sizeof(int),          \
                    (int const[]){__VA_ARGS__})
