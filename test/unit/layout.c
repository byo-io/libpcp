#include "layout.h"

#include <unity.h>

#include "runner.h"

static void test_layout_c11_matches_c23() {
  size_t index = 0;
#define LAYOUT_CHECK(value)                                                    \
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(value, layout_c11[index++], #value);
  PCP_LAYOUT(LAYOUT_CHECK)
#undef LAYOUT_CHECK
}

void layout_tests() { RUN_TEST(test_layout_c11_matches_c23); }
