#include "dimr_bmi.h"
#include "unity.h"
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

static void test_version_number(void) {
  char *version_string = NULL;
  get_version_string(&version_string);
  TEST_ASSERT(strlen(version_string) > 0);
  TEST_ASSERT_EQUAL_STRING_LEN(version_string, "v", 1);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_version_number);

  return UNITY_END();
}
