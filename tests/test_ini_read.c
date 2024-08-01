
#include "ini/ini_read.h"
#include "unity.h"

#include <string.h>

void setUp(void) {}

void tearDown(void) {}

static int my_ini_handler(char *section, char *key, char *value, void *data_ptr) {
  //TODO: Add some asserts here.
  return INI_OK;
}

static void test_ini_read() {
  char *filepath = "test_ini_input.ini";
  int status = ini_read(filepath, my_ini_handler, NULL);
  TEST_ASSERT_MESSAGE(status == INI_OK, "Failed to load INI file.");
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_ini_read);

  return UNITY_END();
}