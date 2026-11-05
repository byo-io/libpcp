#include "runner.h"
#include "sim.h"

void setUp() {}
void tearDown() { sim_teardown(); }

extern void scenario_tests();

int main(int argc, char *argv[]) {
  if (!test_runner_init(argc, argv)) {
    return 2;
  }

  UNITY_BEGIN();

  scenario_tests();

  int const result = UNITY_END();
  return test_runner_selected_test_found() ? result : 2;
}
