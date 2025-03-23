#include <stdbool.h>
#include <stddef.h>

#ifndef TESTLIB_H_
#define TESTLIB_H_

#ifndef TESTLIB_FONT_BANK
#define TESTLIB_FONT_BANK 2
#endif
#ifndef TESTLIB_STRING_BANK
#define TESTLIB_STRING_BANK TESTLIB_FONT_BANK
#endif

void test_init(void);
void test_push_name(unsigned char *test_name);
void test_push_fixture(unsigned char *fixture_name);
void test_assert_msg(bool condition, unsigned char *msg);
void test_assert_eq_int_msg(int actual, int expected, unsigned char *msg);
void test_assert_eq_byte_msg(unsigned char actual, unsigned char expected, unsigned char *msg);
void test_complete(void);
void test_set_string_bank(unsigned char bank);

inline void test_assert(bool condition)
{
  test_assert_msg(condition, NULL);
}

inline void test_assert_eq_int(int actual, int expected)
{
  test_assert_eq_int_msg(actual, expected, NULL);
}

inline void test_assert_eq_byte(unsigned char actual, unsigned char expected)
{
  test_assert_eq_byte_msg(actual, expected, NULL);
}

#endif
