#include <unity.h>

#include "runner.h"
#include "sim.h"

// Scenarios for the client in a simulated world (see sim.h). Each one is a
// short script: set up the world, act like an application, let time pass, and
// check what the application and the server ended up with.
//
//   SCENARIO(test_sim_releases_its_mapping) {
//     auto const handle = start_mapped(sim);
//     sim_run_for(sim, SIM_HOUR);
//
//     sim_client_release(&sim->client, handle);
//     sim_run_for(sim, SIM_SECOND);
//     sim_assert_no_lease(sim, handle);
//   }
//
// and a RUN_TEST for it at the bottom of this file. It runs once for each of
// SEEDS seeds, and prints the timeline of the first seed and of any that fail.
// To watch one, run e.g. ./build/test/sim/sim_tests test_sim_full_lifecycle
//
// What a scenario has to work with:
//
// - The world: sim_start, or start_mapped to begin with a granted mapping.
//   sim_run_for lets time pass.
// - The application: map, or sim_client_map for the library's whole result,
//   and sim_client_release.
// - Checks: sim_assert_mapped, sim_assert_no_lease, sim_assert_idle,
//   SIM_ASSERT_EVENTS, and the counts in sim_stats.
//
// Use sim->seed for anything random, so that every seed is different.

// jitter, nonces and network faults come from the seed, so go through a few
static constexpr uint64_t SEEDS = 16;

static constexpr pcp_time_s_t LIFETIME = 3600;
static constexpr port_t PORT = 1234;

// a scenario that runs once per seed, in a new world each time
#define SCENARIO(name)                                                         \
  static void name##_scenario(sim_t *sim);                                     \
  static void name() { sim_run_scenario(name##_scenario, SEEDS); }             \
  static void name##_scenario(sim_t *sim)

static void assert_same_address(socket_address_t expected,
                                socket_address_t actual) {
  TEST_ASSERT_EQUAL_INT_MESSAGE(0, socket_address_cmp(&expected, &actual),
                                "the external address changed");
}

// asks for a mapping like the application would, a request that fails has
// already failed the scenario
static pcp_handle_t
map(sim_t *sim, pcp_time_s_t lifetime, pcp_protocol_t protocol, port_t port) {
  return sim_client_map(&sim->client, lifetime, protocol, port).value;
}

// a mapping the server has granted, with some time left until its renewal
static pcp_handle_t map_and_settle(sim_t *sim) {
  auto const handle = map(sim, LIFETIME, UDP, PORT);
  sim_run_for(sim, SIM_SECOND);
  sim_assert_mapped(sim, handle);
  return handle;
}

static pcp_handle_t start_mapped(sim_t *sim) {
  sim_start(sim);
  return map_and_settle(sim);
}

// Network

SCENARIO(test_sim_full_lifecycle) {
  auto const handle = start_mapped(sim);
  auto const external = sim_external(sim, handle);

  // the same mapping all day, without the application hearing about it
  sim_run_for(sim, 24 * SIM_HOUR);
  sim_assert_mapped(sim, handle);
  assert_same_address(external, sim_external(sim, handle));
  SIM_ASSERT_EVENTS(sim, handle, PCP_EVENT_MAPPED);

  // renewing 1/2 to 5/8 into every lease (section 11.2.1), and nothing is lost
  // so every request is a first attempt
  auto stats = sim_stats(sim);
  TEST_ASSERT_EQUAL_size_t(0, stats.expired);
  TEST_ASSERT_EQUAL_size_t(1, stats.created);
  TEST_ASSERT_GREATER_OR_EQUAL_size_t(24 * 3600 / (LIFETIME * 5 / 8),
                                      stats.renewed);
  TEST_ASSERT_LESS_OR_EQUAL_size_t(24 * 3600 / (LIFETIME / 2), stats.renewed);
  TEST_ASSERT_EQUAL_size_t(1 + stats.renewed, stats.requests);

  sim_client_release(&sim->client, handle);
  sim_run_for(sim, SIM_SECOND);
  sim_assert_no_lease(sim, handle);
  sim_assert_idle(sim);
  SIM_ASSERT_EVENTS(sim, handle, PCP_EVENT_MAPPED);

  stats = sim_stats(sim);
  TEST_ASSERT_EQUAL_size_t(1, stats.deleted);
  TEST_ASSERT_EQUAL_size_t(0, stats.expired);
  TEST_ASSERT_EQUAL_size_t(2 + stats.renewed, stats.requests);
}

void scenario_tests() { RUN_TEST(test_sim_full_lifecycle); }
