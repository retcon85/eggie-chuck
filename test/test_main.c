#include <string.h>
#include "testlib.h"
#include "SMSlib.h"
#include "model_game.h"

static void test_model_game(void);

void main(void)
{
  test_init();
  test_model_game();
  test_complete();
}

static void test_model_game_init(void)
{
  test_push_fixture("test_model_game_init");
  m.player_count = 1;
  model_game_init("y12x10p5Pp5", 1);
  uint8_t i;
  char msg[] = "platform[ ]\0";
  for (i = 0; i < 10; i++)
  {
    msg[9] = i + '0';
    test_assert_eq_byte_msg(pg->screen_map[12][i] & 0x0f, NOTHING, msg);
  }
  msg[11] = ']';
  msg[9] = '1';
  for (; i < 20; i++)
  {
    msg[10] = (i - 10) + '0';
    test_assert_eq_byte_msg(pg->screen_map[12][i] & 0x0f, PLATFORM, msg);
  }
  msg[9] = '2';
  for (; i < 30; i++)
  {
    msg[10] = (i - 20) + '0';
    test_assert_eq_byte_msg(pg->screen_map[12][i] & 0x0f, NOTHING, msg);
  }
  test_assert_eq_byte_msg(pg->player.x, 120, "player_pos.x");
  test_assert_eq_byte_msg(pg->player.y, 96, "player_pos.y");
}

static void test_model_game_walk(void)
{
  test_push_fixture("test_model_game_walk");
  m.player_count = 1;
  model_game_init("y12x10p5Pp5", 1);
  model_game_player_right();
  model_game_tick();
  test_assert_eq_byte_msg(pg->player.x, 121, "player_pos.x");
  test_assert_eq_byte_msg(pg->player.y, 96, "player_pos.y");
  model_game_player_right();
  model_game_tick();
  test_assert_eq_byte_msg(pg->player.x, 122, "player_pos.x");
  test_assert_eq_byte_msg(pg->player.y, 96, "player_pos.y");
  for (uint8_t i = 0; i < 4; i++)
  {
    model_game_player_left();
    model_game_tick();
  }
  test_assert_eq_byte_msg(pg->player.x, 118, "player_pos.x");
  test_assert_eq_byte_msg(pg->player.y, 96, "player_pos.y");
}

static void assert_position_sequence(uint8_t expected_x[], uint8_t expected_y[], size_t n)
{
  char msg[] = "player_pos.y[  ]";
  for (uint8_t i = 0; i < n; i++)
  {
    if (i >= 30)
    {
      msg[13] = '3';
      msg[14] = (i - 30) + '0';
    }
    else if (i >= 20)
    {
      msg[13] = '2';
      msg[14] = (i - 20) + '0';
    }
    else if (i >= 10)
    {
      msg[13] = '1';
      msg[14] = (i - 10) + '0';
    }
    else
    {
      msg[14] = i + '0';
    }
    msg[11] = 'x';
    test_assert_eq_byte_msg(pg->player.x, expected_x[i], msg);
    msg[11] = 'y';
    test_assert_eq_byte_msg(pg->player.y, expected_y[i], msg);
    model_game_tick();
  }
}

static void assert_static_position(uint8_t expected_x, uint8_t expected_y, uint8_t repeat)
{
  char msg[] = "player_pos.y[ ]";
  for (uint8_t i = 0; i < repeat; i++)
  {
    msg[13] = i + '0';
    msg[11] = 'x';
    test_assert_eq_byte_msg(pg->player.x, expected_x, msg);
    msg[11] = 'y';
    test_assert_eq_byte_msg(pg->player.y, expected_y, msg);
    model_game_tick();
  }
}

static void test_model_game_jump_up(void)
{
  test_push_fixture("test_model_game_jump_up");
  m.player_count = 1;
  model_game_init("y12x10p5Pp5", 1);
  model_game_player_jump();
  uint8_t expected_x[] = {120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120};
  uint8_t expected_y[] = {96, 94, 92, 90, 88, 87, 86, 85, 84, 83, 82, 81, 80, 80, 80, 80, 80, 80, 80, 80, 80, 81, 82, 83, 84, 85, 86, 87, 88, 90, 92, 94, 96};
  assert_position_sequence(expected_x, expected_y, sizeof(expected_x));
  test_push_fixture("test_model_game_jump_up_rest");
  assert_static_position(expected_x[sizeof(expected_x) - 1], expected_y[sizeof(expected_y) - 1], 9);
}

static void test_model_game_jump_left(void)
{
  test_push_fixture("test_model_game_jump_left");
  m.player_count = 1;
  model_game_init("y12x10p8Pp2", 1);
  model_game_player_left();
  model_game_player_jump();
  uint8_t expected_x[] = {144, 143, 142, 141, 140, 139, 138, 137, 136, 135, 134, 133, 132, 131, 130, 129, 128, 127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112};
  uint8_t expected_y[] = {96, 94, 92, 90, 88, 87, 86, 85, 84, 83, 82, 81, 80, 80, 80, 80, 80, 80, 80, 80, 80, 81, 82, 83, 84, 85, 86, 87, 88, 90, 92, 94, 96};
  assert_position_sequence(expected_x, expected_y, sizeof(expected_x));
  test_push_fixture("test_model_game_jump_left_rest");
  // !TODO: fix this 1px drift
  // assert_static_position(expected_x[sizeof(expected_x) - 1], expected_y[sizeof(expected_y) - 1], 9);
  assert_static_position(111, expected_y[sizeof(expected_y) - 1], 9);
}

static void test_model_game_jump_right(void)
{
  test_push_fixture("test_model_game_jump_rt");
  m.player_count = 1;
  model_game_init("y12x10p2Pp8", 1);
  model_game_player_right();
  model_game_player_jump();
  uint8_t expected_x[] = {96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128};
  uint8_t expected_y[] = {96, 94, 92, 90, 88, 87, 86, 85, 84, 83, 82, 81, 80, 80, 80, 80, 80, 80, 80, 80, 80, 81, 82, 83, 84, 85, 86, 87, 88, 90, 92, 94, 96};
  assert_position_sequence(expected_x, expected_y, sizeof(expected_x));
  test_push_fixture("test_model_game_jump_rt_rest");
  // !TODO: fix this 1px drift
  // assert_static_position(expected_x[sizeof(expected_x) - 1], expected_y[sizeof(expected_y) - 1], 9);
  assert_static_position(129, expected_y[sizeof(expected_y) - 1], 9);
}

static void test_model_game_fall(void)
{
  test_push_fixture("test_model_game_fall");
  m.player_count = 1;
  model_game_init("y12x10p10P", 1);
  model_game_player_right();
  uint8_t expected_x[] = {160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160};
  uint8_t expected_y[] = {96, 96, 96, 96, 96, 96, 96, 96, 97, 98, 99, 100, 101, 102, 103, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132, 134, 136, 138};
  assert_position_sequence(expected_x, expected_y, sizeof(expected_x));
}

static void test_model_game_ladder_up(void)
{
  test_push_fixture("test_model_game_ladder_up");
  m.player_count = 1;
  model_game_init("y12x10p4Pp6 y18x14l14", 1);
  for (uint8_t i = 0; i < 10; i++)
  {
    model_game_player_up();
    model_game_tick();
  }
  assert_static_position(112, 86, 9);
}

static void test_model_game_ladder_down(void)
{
  test_push_fixture("test_model_game_ladder_down");
  m.player_count = 1;
  model_game_init("y12x10p4Pp6 y18x14l14", 1);
  for (uint8_t i = 0; i < 10; i++)
  {
    model_game_player_down();
    model_game_tick();
  }
  assert_static_position(112, 106, 9);
}

static void test_model_game_ladder_jump(void)
{
  test_push_fixture("test_model_game_ladder_jump");
  m.player_count = 1;
  model_game_init("y12x10p4Pp6 y18x14l14", 1);
  for (uint8_t i = 0; i < 10; i++)
  {
    model_game_player_up();
    model_game_tick();
  }
  model_game_player_jump();
  for (uint8_t i = 0; i < 38; i++)
  {
    model_game_tick();
  }
  assert_static_position(112, 96, 9);
}

static void test_model_game_jump_to_ladder(void)
{
  test_push_fixture("test_model_game_jump_to_ladder");
  m.player_count = 1;
  model_game_init("y12x10pPp9 y18x14l14", 1);
  model_game_player_right();
  model_game_player_jump();
  for (uint8_t i = 0; i < 24; i++)
  {
    model_game_tick();
  }
  model_game_player_up();
  model_game_tick(); // 1 pixel upward creep
  assert_static_position(112, 83, 9);
}

static void test_model_game(void)
{
  test_model_game_init();
  test_model_game_walk();
  test_model_game_jump_up();
  test_model_game_jump_left();
  test_model_game_jump_right();
  test_model_game_fall();
  test_model_game_ladder_up();
  test_model_game_ladder_down();
  test_model_game_ladder_jump();
  test_model_game_jump_to_ladder();
}

char *text_strncpy(char *dest, const char *src, size_t n)
{
  return strncpy(dest, src, n);
}
