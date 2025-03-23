#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "SMSlib.h"
#include "testlib.h"

extern char *text_strncpy(char *dest, const char *src, size_t n);
extern const unsigned char test_font;

static int _fixture_index = 0;
static int _test_index = 0;
static int _assert_index = 0;
static unsigned char _test_name[32];
static unsigned char _fixture_name[32];

void clear_assertion_runs(void)
{
  SMS_setNextTileatXY(0, 2);
  for (int y = 2; y < 20; y++)
  {
    for (int x = 0; x < 32; x++)
    {
      SMS_print(" ");
    }
  }
}

void clear_test_runs(void)
{
  SMS_setNextTileatXY(0, 1);
  for (int x = 0; x < 32; x++)
  {
    SMS_print(" ");
  }
}

void clear_messages(void)
{
  SMS_setNextTileatXY(0, 2);
  for (int y = 20; y < 24; y++)
  {
    for (int x = 0; x < 32; x++)
    {
      SMS_print(" ");
    }
  }
}

void test_init(void)
{
  SMS_init();
  SMS_displayOff();
  SMS_setBackdropColor(0);
  SMS_setBGPaletteColor(0, 0xaa);
  SMS_setBGPaletteColor(1, 0x00);
  SMS_setSpritePaletteColor(0, 0x0a);
  SMS_load1bppTiles(&test_font, 32, 96 * 8, 0, 1);
  SMS_displayOn();
  SMS_setNextTileatXY(0, 0);
  for (int y = 0; y < 28; y++)
  {
    for (int x = 0; x < 32; x++)
    {
      SMS_print(" ");
    }
  }
  SMS_setNextTileatXY(0, 0);
  _fixture_index = 0;
  _test_index = 0;
  _assert_index = 0;
}

static unsigned char string_bank = TESTLIB_STRING_BANK;

void enter_string_bank(void)
{
}

void exit_string_bank(void)
{
}

inline void test_set_string_bank(unsigned char bank)
{
  string_bank = bank;
}

void test_push_fixture(unsigned char *fixture_name)
{
  clear_messages();
  clear_test_runs();
  clear_assertion_runs();
  enter_string_bank();
  text_strncpy(_fixture_name, fixture_name, 31);
  exit_string_bank();
  _fixture_name[31] = '/0';
  SMS_setNextTileatXY(0, 20);
  for (int i = 0; i < 32; i++)
    SMS_print(" ");
  SMS_setNextTileatXY(0, 20);
  SMS_print(_fixture_name);
  SMS_setNextTileatLoc(_fixture_index);
  SMS_print("O");
  _fixture_index++;
  _test_index = 0;
  _assert_index = 0;
}

void test_push_name(unsigned char *test_name)
{
  clear_assertion_runs();
  enter_string_bank();
  text_strncpy(_test_name, test_name, 31);
  exit_string_bank();
  _test_name[31] = '/0';
  SMS_setNextTileatXY(0, 21);
  for (int i = 0; i < 32; i++)
    SMS_print(" ");
  SMS_setNextTileatXY(0, 21);
  SMS_print(_test_name);
  SMS_setNextTileatLoc(32 + _test_index);
  SMS_print("o");
  _test_index++;
  _assert_index = 0;
}

void test_assert_msg(bool condition, unsigned char *msg)
{
  unsigned int loc = 64 + _assert_index;
  if (loc >= 640)
  {
    clear_assertion_runs();
    _assert_index = 0;
  }
  SMS_setNextTileatLoc(loc);
  if (!condition)
  {
    SMS_print("x");
    SMS_setNextTileatXY(0, 22);
    SMS_print("Test failed");
    SMS_setNextTileatXY(0, 23);
    enter_string_bank();
    if (msg != NULL)
    {
      SMS_print(msg);
    }
    else
    {
      SMS_print("(no message)");
    }
    exit_string_bank();
    SMS_setSpritePaletteColor(0, 0x02);
    while (true)
      ;
  }
  SMS_print(".");
  _assert_index++;
}

// avoid pulling in printf to save space!

/*-------------------------------------------------------------------------
 integer to string conversion

 Written by:   Bela Torok, 1999
               bela.torok@kssg.ch
 usage:

 uitoa(unsigned int value, char* string, int radix)
 itoa(int value, char* string, int radix)

 value  ->  Number to be converted
 string ->  Result
 radix  ->  Base of value (e.g.: 2 for binary, 10 for decimal, 16 for hex)
---------------------------------------------------------------------------*/

#define NUMBER_OF_DIGITS 16 /* space for NUMBER_OF_DIGITS + '\0' */

void uitoa(unsigned int value, char *string, int radix)
{
  unsigned char index, i;

  index = NUMBER_OF_DIGITS;
  i = 0;

  do
  {
    string[--index] = '0' + (value % radix);
    if (string[index] > '9')
      string[index] += 'A' - ':'; /* continue with A, B,.. */
    value /= radix;
  } while (value != 0);

  do
  {
    string[i++] = string[index++];
  } while (index < NUMBER_OF_DIGITS);

  string[i] = 0; /* string terminator */
}

void itoa(int value, char *string, int radix)
{
  if (value < 0 && radix == 10)
  {
    *string++ = '-';
    uitoa(-value, string, radix);
  }
  else
  {
    uitoa(value, string, radix);
  }
}

void test_assert_eq_int_msg(int actual, int expected, unsigned char *msg)
{
  unsigned char fmt[23], new_msg[33];

  if (actual != expected)
  {
    int fmt_len = 0;
    strcpy(fmt, "want:");
    fmt_len = strlen(fmt);
    itoa(expected, &fmt[fmt_len], 10);
    fmt_len = strlen(fmt);
    strcpy(&fmt[fmt_len], " got:");
    fmt_len = strlen(fmt);
    itoa(actual, &fmt[fmt_len], 10);
    fmt_len = strlen(fmt);

    enter_string_bank();
    if (msg == NULL)
    {
      test_assert_msg(false, fmt);
    }
    else
    {
      text_strncpy(new_msg, msg, 31 - fmt_len);
      new_msg[32] = '/0';
      int msg_len = strlen(new_msg);
      new_msg[msg_len] = ' ';
      strcpy(&new_msg[msg_len + 1], fmt);
      test_assert_msg(false, new_msg);
    }
    exit_string_bank();
  }
  else
  {
    test_assert(true);
  }
}

void test_assert_eq_byte_msg(unsigned char actual, unsigned char expected, unsigned char *msg)
{
  return test_assert_eq_int_msg(actual, expected, msg);
}

void test_complete(void)
{
  // clear_assertion_runs();
  SMS_setSpritePaletteColor(0, 0x04);
  SMS_setNextTileatXY(0, 23);
  SMS_print("All tests pass");
  while (true)
    ;
}
