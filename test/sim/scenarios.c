#include <unity.h>

#include "runner.h"
#include "sim.h"

// jitter and nonces come from the seed, so go through a few of them
static constexpr uint64_t SEEDS = 16;

static void full_lifecycle(uint64_t seed) {
  static constexpr pcp_time_s_t lifetime = 3600;

  sim_start((sim_config_t){.seed = seed});
  auto const handle = sim_map(lifetime, UDP, 1234);

  sim_run_for(SIM_SECOND);
  sim_assert_mapped(handle);
  auto const external = sim_external(handle);

  // the same mapping all day, without the application hearing about it
  sim_run_for(24 * SIM_HOUR);
  sim_assert_mapped(handle);
  auto const still_external = sim_external(handle);
  TEST_ASSERT_EQUAL_INT(0, socket_address_cmp(&external, &still_external));
  SIM_ASSERT_EVENTS(handle, PCP_EVENT_MAPPED);

  // renewing 1/2 to 5/8 into every lease (RFC 6887 section 11.2.1), and nothing
  // is lost so every request is a first attempt
  auto stats = sim_stats();
  TEST_ASSERT_EQUAL_size_t(0, stats.expired);
  TEST_ASSERT_EQUAL_size_t(1, stats.created);
  TEST_ASSERT_GREATER_OR_EQUAL_size_t(24 * 3600 / (lifetime * 5 / 8),
                                      stats.renewed);
  TEST_ASSERT_LESS_OR_EQUAL_size_t(24 * 3600 / (lifetime / 2), stats.renewed);
  TEST_ASSERT_EQUAL_size_t(1 + stats.renewed, stats.requests);

  sim_release(handle);
  sim_run_for(SIM_SECOND);
  sim_assert_no_lease(handle);
  sim_assert_idle();
  SIM_ASSERT_EVENTS(handle, PCP_EVENT_MAPPED);

  stats = sim_stats();
  TEST_ASSERT_EQUAL_size_t(1, stats.deleted);
  TEST_ASSERT_EQUAL_size_t(0, stats.expired);
  TEST_ASSERT_EQUAL_size_t(2 + stats.renewed, stats.requests);

  sim_end();
}

static void test_sim_full_lifecycle() {
  for (uint64_t seed = 1; seed <= SEEDS; ++seed) {
    full_lifecycle(seed);
  }
}

void scenario_tests() { RUN_TEST(test_sim_full_lifecycle); }
