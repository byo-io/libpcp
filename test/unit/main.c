#include "runner.h"

void setUp() {}
void tearDown() {}

extern void epoch_tests();
extern void ip_tests();
extern void layout_tests();
extern void schedule_tests();
extern void session_tests();
extern void wire_tests();

int main(int argc, char *argv[]) {
  if (!test_runner_init(argc, argv)) {
    return 2;
  }

  UNITY_BEGIN();

  ip_tests();
  layout_tests();
  epoch_tests();
  schedule_tests();
  session_tests();
  wire_tests();

  int const result = UNITY_END();
  return test_runner_selected_test_found() ? result : 2;
}
