
#include "ini/ini_read.h"
#include "unity.h"

#include <string.h>

void setUp(void) {}

void tearDown(void) {}

static int my_ini_handler(char *section, char *key, char *value, void *data_ptr) {
  if (!strcmp(section, "section1")) {
    if (!strcmp(key, "number")) {
      TEST_ASSERT_EQUAL_STRING("1", value);
    } else if (!strcmp(key, "another")) {
      TEST_ASSERT_EQUAL_STRING("3.14", value);
    } else if (!strcmp(key, "x_also")) {
      TEST_ASSERT_EQUAL_STRING("5", value);
    } else if (!strcmp(key, "y_and_then")) {
      TEST_ASSERT_EQUAL_STRING("72", value);
    } else if (!strcmp(key, "key")) {
      TEST_ASSERT_EQUAL_STRING("value", value);
    } else if (!strcmp(key, "keyname")) {
      TEST_ASSERT_EQUAL_STRING("value with spaces", value);
    } else if (!strcmp(key, "variable")) {
      TEST_ASSERT_EQUAL_STRING("a value", value);
    } else if (!strcmp(key, "var2")) {
      TEST_ASSERT_EQUAL_STRING("value with ; included", value);
    } else {
      // Section start, key and value should be empty.
      TEST_ASSERT_EQUAL_STRING("", key);
      TEST_ASSERT_EQUAL_STRING("", value);
    }
  } else if (!strcmp(section, "sections can have spaces")) {
    if (!strcmp(key, "novaluegiven")) {
      TEST_FAIL_MESSAGE("Handler called for key with empty value.");
    } else if (!strcmp(key, "no_value_only_comment")) {
      TEST_ASSERT_EQUAL_STRING_MESSAGE("", value, "Comment passed as value.");
      TEST_FAIL_MESSAGE("Handler called for key with empty value.");
    } else if (!strcmp(key, "lots_of_spaces")) {
      TEST_ASSERT_EQUAL_STRING("234.5", value);
    } else if (!strcmp(key, "attribute")) {
      TEST_ASSERT_EQUAL_STRING("42", value);
    } else if (!strcmp(key, "list_variable")) {
      TEST_ASSERT_EQUAL_STRING("1, 1, 2, 3, 5, 8", value);
    } else if (!strcmp(key, "#disabled")) {
      TEST_FAIL_MESSAGE("Trying to set disabled key.");
    } else if (!strcmp(key, ";disabled")) {
      TEST_FAIL_MESSAGE("Trying to set disabled key.");
    } else {
      // Section start, key and value should be empty.
      TEST_ASSERT_EQUAL_STRING("", key);
      TEST_ASSERT_EQUAL_STRING("", value);
    }
  } else {
    TEST_FAIL_MESSAGE("Unexpected call of handler.");
  }
  return INI_OK;
}

static void test_ini_read() {
  char *filepath = "test_ini_input.ini";
  int status = ini_read(filepath, my_ini_handler, NULL);
  TEST_ASSERT_MESSAGE(status == INI_OK, "Failed to load INI file.");
}

static void test_ini_parse_int() {
  int status = INI_OK;
  int ival = 0;

  ival = ini_parse_int("42", &status);
  TEST_ASSERT_MESSAGE(status == INI_OK, "Conversion to int failed.");
  TEST_ASSERT_INT_WITHIN(0, 42, ival);
}

static void test_ini_parse_double() {
  int status = INI_OK;
  double dval = 0.0;

  dval = ini_parse_double("123.4", &status);
  TEST_ASSERT_MESSAGE(status == INI_OK, "Conversion to double failed.");
  TEST_ASSERT_DOUBLE_WITHIN(0.05, 123.4, dval);

  dval = ini_parse_double("-567.8", &status);
  TEST_ASSERT_MESSAGE(status == INI_OK, "Conversion to double failed.");
  TEST_ASSERT_DOUBLE_WITHIN(0.05, -567.8, dval);
}

static void test_ini_parse_double_list() {
  int status = INI_OK;
  int length = 0;
  double *array = ini_parse_double_list("1,1,2,3.8, 5 ,8", &length, &status);
  TEST_ASSERT_MESSAGE(status == INI_OK, "Conversion to double list failed.");
  TEST_ASSERT(array != NULL);
  TEST_ASSERT(length == 6);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_ini_read);
  RUN_TEST(test_ini_parse_int);
  RUN_TEST(test_ini_parse_double);
  RUN_TEST(test_ini_parse_double_list);

  return UNITY_END();
}