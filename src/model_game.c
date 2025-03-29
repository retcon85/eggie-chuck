#include <stdbool.h>
#include "model_game.h"

struct game_model_t m;
struct player_game_t *pg;

uint8_t get_ready_counter = 0;

void reset_field(uint8_t field[], uint8_t size, uint8_t msd)
{
  for (uint8_t i = 0; i < size - 1; i++, field++)
  {
    *field = 0;
  }
  *field = msd;
}

bool update_field(uint8_t field[], uint8_t size, int8_t delta)
{
  for (uint8_t i = 0; i < size; i++)
  {
    field[i] += delta;
    // no carry
    if (field[i] < 100)
    {
      return false;
    }
    // positive carry
    if (field[i] < 200)
    {
      delta = 1;
      field[i] -= 100;
      continue;
    }
    // negative carry
    delta = -1;
    field[i] += 100;
  }
  return true;
}

static inline void reset_score(void)
{
  reset_field(pg->score, sizeof(pg->score), 0);
}

void reset_bonus(uint8_t msdigit)
{
  pg->bonus_exhausted = false;
  reset_field(pg->bonus, sizeof(pg->bonus), msdigit);
}

void load_level(void)
{
  // clear screen
  for (uint8_t y = 0; y < SCREEN_MAP_HEIGHT; y++)
  {
    for (uint8_t x = 0; x < SCREEN_MAP_WIDTH; x++)
    {
      pg->screen_map[y][x] = NOTHING;
    }
  }

  pg->elevator_enabled = false;
  pg->bird_count = 0;
  pg->egg_count = 0;
  char cmd = ' ';
  uint8_t arg = 0;
  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t tmp = 0;
  const char *c = m.level_data[pg->level];
  while (cmd != '\0')
  {
    for (; *c >= '0' && *c <= '9'; c++)
    {
      arg = (arg * 10) + (*c - '0');
    }
    switch (cmd)
    {
    case 'x':
      x = arg;
      break;
    case 'y':
      y = arg;
      break;
    case 'e':
      pg->screen_map[y - 1][x] = EGG;
      pg->screen_map[y - 1][x + 1] = EGG | (1 << 4);
      pg->egg_count++;
      break;
    case 'g':
      pg->screen_map[y - 1][x] = GRAIN;
      pg->screen_map[y - 1][x + 1] = GRAIN | (1 << 4);
      break;
    case 'l':
      tmp = y;
      for (; arg > 0; arg--, tmp--)
      {
        pg->screen_map[tmp][x] = (pg->screen_map[tmp][x] & 0x07) | LADDER;
        pg->screen_map[tmp][x + 1] = (pg->screen_map[tmp][x + 1] & 0x07) | (LADDER | (1 << 4));
      }
      x += 2;
      break;
    case 'p':
      tmp = 0;
      for (arg = arg ? arg : 1; arg > 0; arg--, x++, tmp = (tmp + 1) % 2)
      {
        pg->screen_map[y][x] = PLATFORM | (tmp << 4);
      }
      break;
    case 'P':
      pg->player.initial_x = x * 8;
      pg->player.initial_y = y * 8;
      break;
    case 'B':
      pg->birds[pg->bird_count].initial_x = x * 8;
      pg->birds[pg->bird_count].initial_y = y * 8;
      pg->bird_count++;
      break;
    case 'v':
      pg->elevator_enabled = true;
      pg->elevator_pos_x = x * 8;
      pg->elevator_pos_y = y * 8;
      break;
    }
    cmd = *(c++);
    arg = 0;
  }
  m.render_mask |= VIEW_GAME_RENDER_FULL_REDRAW | VIEW_GAME_RENDER_SCORELINE;
  reset_bonus(10);
}

void model_game_init(char *level_data[], uint8_t level_count)
{
  m.reset = false;
  m.level_data = level_data;
  m.level_count = level_count;
  for (m.current_player = 0; m.current_player < m.player_count; m.current_player++)
  {
    pg = &m.player_games[m.current_player];
    pg->level = 0;
    pg->lives = INITIAL_LIVES - 1;
    pg->game_over = false;
    reset_score();
    for (uint8_t y = 0; y < SCREEN_MAP_HEIGHT; y++)
    {
      for (uint8_t x = 0; x < SCREEN_MAP_HEIGHT; x++)
      {
        pg->screen_map[y][x] = NOTHING;
      }
    }
    load_level();
  }
  model_game_select_player(0);
}

static inline void reset_time(void)
{
  reset_field(pg->time, sizeof(pg->time), 9);
}

void restart_current_level(void)
{
  m.audio_tone = 0;
  pg->player.is_jumping = false;
  pg->player.is_climbing = false;
  pg->player.x = pg->player.initial_x;
  pg->player.y = pg->player.initial_y;
  pg->player.vx = 0;
  pg->player.vy = 0;
  for (uint8_t i = 0; i < pg->bird_count; i++)
  {
    pg->birds[i].x = pg->birds[i].initial_x;
    pg->birds[i].y = pg->birds[i].initial_y;
  }
  reset_time();

  model_game_tick(); // initial tick to calculate surface!
}

void model_game_select_player(uint8_t player)
{
  pg = &m.player_games[player];
  get_ready_counter = GET_READY_SCREEN_TIMEOUT;
  m.render_mask = VIEW_GAME_SHOW_GET_READY_SCREEN | VIEW_GAME_WAIT;
  m.current_player = player;
  restart_current_level();
}

void player_game_over(void)
{
  m.render_mask = VIEW_GAME_SHOW_GET_GAME_OVER_OVERLAY | VIEW_GAME_WAIT;
  get_ready_counter = GET_READY_SCREEN_TIMEOUT;
  pg->game_over = true;
}

void next_player(void)
{
  uint8_t current_player = m.current_player;

  do
  {
    if (m.current_player == m.player_count - 1)
    {
      model_game_select_player(0);
    }
    else
    {
      model_game_select_player(m.current_player + 1);
    }
  } while (pg->game_over && m.current_player != current_player);

  if (pg->game_over)
  {
    m.reset = true;
  }
}

void model_game_player_left(void)
{
  if (pg->player.is_on_platform || pg->player.is_climbing)
  {
    pg->player.vx = -PLAYER_SPEED;
    if (pg->player.is_on_platform)
    {
      pg->player.is_climbing = false;
    }
  }
}

void model_game_player_right(void)
{
  if (pg->player.is_on_platform || pg->player.is_climbing)
  {
    pg->player.vx = PLAYER_SPEED;
    if (pg->player.is_on_platform)
    {
      pg->player.is_climbing = false;
    }
  }
}

void model_game_player_up(void)
{
  if (pg->player.is_over_ladder || pg->player.is_climbing)
  {
    pg->player.vy = -LADDER_SPEED;
    pg->player.is_climbing = true;
    pg->player.is_jumping = false;
  }
}

void model_game_player_down(void)
{
  if (pg->player.is_over_ladder || pg->player.is_climbing)
  {
    pg->player.vy = LADDER_SPEED;
    pg->player.is_climbing = true;
    pg->player.is_jumping = false;
  }
}

void model_game_player_jump(void)
{
  if (pg->game_over && get_ready_counter == 0)
  {
    next_player();
  }

  if (pg->player.is_on_platform || pg->player.is_climbing)
  {
    pg->player.vy = -JUMP_SPEED_INITIAL;
    pg->player.is_jumping = true;
    pg->player.is_climbing = false;
  }
}
