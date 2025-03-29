#include <stdbool.h>
#include "model_game.h"
#include "prng.h"

extern struct game_model_t m;
extern struct player_game_t *pg;

void reset_bonus(uint8_t msdigit);
void player_game_over(void);
bool update_field(uint8_t field[], uint8_t size, int8_t delta);
void load_level(void);
void next_player(void);
void reset_field(uint8_t field[], uint8_t size, uint8_t msd);
void restart_current_level(void);

static uint8_t time_counter = 0;
static uint8_t bonus_counter = 0;
static uint8_t bird_counter = 0;
extern uint8_t get_ready_counter;

static void life_over(void)
{
  m.audio_tone = 0;
  m.audio_music = true;
  if (pg->lives == 0)
  {
    player_game_over();
    return;
  }

  pg->lives--;
  next_player();
}

static inline bool update_time(uint8_t delta)
{
  return update_field(pg->time, sizeof(pg->time), -delta);
}

static inline bool update_bonus(uint8_t delta)
{
  return update_field(pg->bonus, sizeof(pg->bonus), -delta);
}

static void update_score(uint8_t delta)
{
  uint8_t ten_k = pg->score[2];
  update_field(pg->score, sizeof(pg->score), delta);
  if (pg->score[2] != ten_k)
  {
    pg->lives++;
  }
  m.render_mask |= VIEW_GAME_RENDER_SCORELINE;
}

static void collect_grain(void)
{
  update_score(50);
  time_counter -= GRAIN_BONUS_TIME;
}

static void next_level(void)
{
  if (pg->level == m.level_count - 1)
  {
    player_game_over();
    return;
  }
  pg->level++;
  load_level();
  restart_current_level();
}

static void collect_egg(void)
{
  if (pg->egg_count == 0)
    return;
  update_score(100);
  pg->egg_count--;
}

inline bool kill_player_offscreen(void)
{
  if (pg->player.y >= SCREEN_MAP_HEIGHT * 8 ||
      pg->player.is_on_elevator && pg->player.y < 24) // !TODO:
  {
    life_over();
    return true;
  }
  return false;
}

inline void update_counters(void)
{
  if (get_ready_counter > 0)
  {
    get_ready_counter--;
    if (get_ready_counter == 0 && !pg->game_over)
    {
      m.render_mask = VIEW_GAME_RENDER_FULL_REDRAW | VIEW_GAME_RENDER_SCORELINE;
    }
    return;
  }

  if (pg->egg_count == 0)
  {
    if (pg->bonus_exhausted)
    {
      next_level();
      return;
    }
    if (update_bonus(BONUS_TRANSFER_RATE))
    {
      reset_bonus(0);
      pg->bonus_exhausted = true;
      return;
    }
    m.audio_noise = true;
    m.audio_tone = AUDIO_NOISE_BONUS_TRANSFER;
    update_score(BONUS_TRANSFER_RATE);
    m.render_mask |= VIEW_GAME_RENDER_SCORELINE;
  }

  if (bird_counter++ == BIRD_COUNTER_FREQ)
  {
    bird_counter = 0;
  }
  if (time_counter++ == TIME_COUNTER_FREQ)
  {
    time_counter = 0;
    if (update_time(1))
    {
      life_over();
      return;
    }
    m.render_mask |= VIEW_GAME_RENDER_SCORELINE;
    if (++bonus_counter == BONUS_COUNTER_FREQ)
    {
      bonus_counter = 0;
      if (!pg->bonus_exhausted && update_bonus(10))
      {
        pg->bonus_exhausted = true;
        reset_bonus(0);
      }
    }
  }
}

inline void move_elevators(void)
{
  pg->player.is_on_elevator = false;
  if (!pg->elevator_enabled)
    return;

  uint8_t elevator_y = pg->elevator_pos_y;
  if (time_counter % 2 == 0)
  {
    pg->elevator_pos_y -= ELEVATOR_SPEED;
  }

  if (pg->player.x + 8 >= pg->elevator_pos_x + 5 &&    // !TODO:
      pg->player.x + 8 <= pg->elevator_pos_x + 24 - 4) // !TODO:
  {
    for (uint8_t i = 0; i < 2; i++, elevator_y -= 112) // !TODO:
    {
      if (pg->player.y >= elevator_y &&
          pg->player.y <= elevator_y + 4)
      {
        pg->player.y = pg->elevator_pos_y - (i ? 112 : 0); // !TODO:
        pg->player.is_on_elevator = true;
        pg->player.is_on_platform = true; // elevator is just like a platform
      };
    }
  }
}

/* inline */ void check_object_collection(uint8_t *tile, uint8_t tile_x, uint8_t tile_y)
{
  if ((*tile & 0x0f) == EGG)
  {
    collect_egg();
  }
  else if ((*tile & 0x0f) == GRAIN)
  {
    collect_grain();
  }
  else
  {
    return;
  }

  // update the map
  if ((*tile & 0xf0) > 0)
  {
    // we caught the right-hand edge, so step back one
    tile--;
    tile_x--;
  }
  *(tile++) = NOTHING;
  *tile = NOTHING;

  // tell the view to blank off the tile where the object was collected
  m.render_mask |= VIEW_GAME_RENDER_COLLECT;
  m.collect_x = tile_x;
  m.collect_y = tile_y;

  // play sfx
  m.audio_noise = true;
  m.audio_tone = AUDIO_NOISE_COLLECT;
}

inline void collision_calculations(void)
{
  pg->player.is_over_ladder = false;
  if (pg->player.is_on_elevator)
    return;

  pg->player.is_on_platform = false;

  // check all four background tiles behind player, starting with...
  // ...top left
  uint8_t tile_x = (pg->player.x + 7) / 8;
  uint8_t tile_y = pg->player.y / 8 - 1;
  uint8_t *tile = &pg->screen_map[tile_y][tile_x];
  check_object_collection(tile, tile_x, tile_y);
  if (((*tile & 0x18) == LADDER) && ((pg->player.x & 0x07) == 0))
  {
    pg->player.is_over_ladder = true;
  }
  // ...top right
  tile_x++;
  tile++;
  check_object_collection(tile, tile_x, tile_y);
  // ...bottom right
  tile_y++;
  tile += SCREEN_MAP_WIDTH;
  check_object_collection(tile, tile_x, tile_y);
  // ...bottom left
  tile_x--;
  tile--;
  check_object_collection(tile, tile_x, tile_y);
  if ((*tile & 0x07) == PLATFORM)
  {
    pg->player.is_on_platform = true;
  }
}

// checks collisions and periodically updates bird positions
inline void move_birds(void)
{
  for (uint8_t i = 0; i < pg->bird_count; i++)
  {
    struct character_t *bird = &pg->birds[i];
    // check collisions
    if (!(
            (bird->x + 12 < pg->player.x) || // !TODO:
            (bird->x > pg->player.x + 12) || // !TODO:
            (bird->y < pg->player.y - 14) || // !TODO:
            (bird->y - 18 > pg->player.y)))  // !TODO:
    {
      life_over();
      return;
    }

    if (bird_counter > 0)
      continue;

    if (!bird->is_climbing)
    {
      if (bird->vx == 0)
      {
        bird->vx = (prng_next() & 0x01) ? BIRD_SPEED : -BIRD_SPEED;
      }
      bird->x += bird->vx;
      uint8_t *tile = &pg->screen_map[bird->y / 8][bird->x / 8];
      // if exactly over ladder, decide whether to climb it or not
      if ((*tile & 0x18) == LADDER && (bird->x & 0x07) == 0)
      {
        bird->vx = (prng_next() & 0x01) ? bird->vx : 0;
        if (bird->vx == 0)
        {
          bird->is_climbing = true;
          bird->is_on_platform = false;
          bird->vy = (prng_next() & 0x01) ? BIRD_SPEED : -BIRD_SPEED;
          bird->y += bird->vy;
        }
      }
      else
      {
        if (bird->vx > 0)
        {
          tile += 2;
        }
        if ((*tile & 0x07) != PLATFORM)
        {
          // if walked over edge, turn around
          bird->vx = -bird->vx;
        }
      }
    }
    else
    {
      bird->y += bird->vy;
      uint8_t *tile = &pg->screen_map[bird->y / 8][bird->x / 8];
      if ((*tile & 0x07) == PLATFORM && (bird->y & 0x07) == 0)
      {
        // if reached platform, decide whether to get off ladder
        bird->vy = (prng_next() & 0x01) ? bird->vy : 0;
        if (bird->vy == 0)
        {
          bird->is_climbing = false;
          bird->is_on_platform = true;
          bird->vx = (prng_next() & 0x01) ? BIRD_SPEED : -BIRD_SPEED;
        }
      }
      // if reached top or bottom of ladder, turn around
      else if (bird->vy > 0)
      {
        if ((*tile & 0x18) != LADDER)
        {
          bird->vy = -bird->vy;
        }
      }
      else
      {
        // need to check top of bird instead of bottom
        tile -= SCREEN_MAP_WIDTH * 2;
        if ((*tile & 0x18) != LADDER)
        {
          bird->vy = -bird->vy;
        }
      }
    }
  }
}

inline void move_player(void)
{
  // for dynamic sounds, i.e. jumping, falling
  static uint8_t audio_tone = 0;

  // check left bounds + update x position
  if (!pg->player.is_climbing && pg->player.vx != 0 && -pg->player.vx < pg->player.x)
  {
    pg->player.x += pg->player.vx;
    if (!m.audio_tone)
    {
      m.audio_noise = false;
      m.audio_tone = AUDIO_TONE_WALKING;
    }
  }
  // check right bounds
  if (pg->player.x > (256 - 16)) // !TODO: extract define
  {
    pg->player.x = 240; // (256 - 16);
  }

  if (pg->player.is_jumping)
  {
    pg->player.y += pg->player.vy >> 3;
    if (audio_tone == 0)
    {
      audio_tone = AUDIO_TONE_JUMPING;
    }
    if (pg->player.vy == 0)
    {
      m.audio_tone = 0;
    }
    else if (pg->player.vy < 0)
    {
      if (!m.audio_tone)
      {
        m.audio_noise = false;
        m.audio_tone = audio_tone;
        audio_tone -= AUDIO_STEP;
      }
    }
    else
    {
      if (!m.audio_tone)
      {
        m.audio_noise = false;
        m.audio_tone = audio_tone;
        audio_tone += AUDIO_STEP;
      }
    }
  }
  if (pg->player.is_climbing)
  {
    pg->player.y += pg->player.vy;
    // not the most efficient, but do another collision check to see if the player is still on a ladder after climbing up/down
    collision_calculations();
    if (!pg->player.is_over_ladder)
    {
      pg->player.y -= pg->player.vy;
      pg->player.vy = 0;
      // sometimes the player can get off alignment horizontally, so fix that up
      pg->player.x = (pg->player.x + 4) & 0xf8;
    }

    // animation & sfx for climbing
    if (pg->player.vy == 0)
    {
      pg->player.animation_frame = 6;
    }
    else
    {
      pg->player.animation_frame++;
      pg->player.animation_frame %= 3;
      pg->player.animation_frame += 6;
      if (!m.audio_tone)
      {
        m.audio_noise = false;
        m.audio_tone = AUDIO_TONE_CLIMBING;
      }
    }
    pg->player.vy = 0;
    pg->player.vx = 0;
    return;
  }
  else
  {
    // !TODO: can probably optimize this, feels a bit sloppy!
    if (pg->player.vx == 0)
    {
      if (pg->player.animation_frame > 2)
      {
        pg->player.animation_frame = 3;
      }
      else
      {
        pg->player.animation_frame = 0;
      }
    }
    else
    {
      pg->player.animation_frame++;
      pg->player.animation_frame %= 3;
      if (pg->player.vx < 0)
      {
        pg->player.animation_frame += 3;
      }
    }

    if (pg->player.is_on_platform && pg->player.vy >= 0)
    {
      pg->player.vy = 0;
      pg->player.vx = 0;
      pg->player.is_jumping = false;
      if (!pg->player.is_on_elevator)
      {
        pg->player.y &= 0xf8;
      }
      audio_tone = 0;
      return;
    }
    else if (!pg->player.is_jumping) // falling
    {
      pg->player.y += pg->player.vy >> 3;
      pg->player.vx = 0;
      if (audio_tone == 0)
      {
        audio_tone = AUDIO_TONE_FALLING;
      }
      if (!m.audio_tone)
      {
        m.audio_noise = false;
        m.audio_tone = audio_tone;
        audio_tone += AUDIO_STEP;
      }
    }
  }
  if (pg->player.vy < JUMP_SPEED_MAX)
  {
    pg->player.vy += GRAVITY;
  }
}

void model_game_tick(void)
{
  if (m.audio_music)
  {
    return; // wait until music finished
  }
  if (get_ready_counter > 0)
  {
    update_counters();
    return;
  }
  if (pg->game_over)
  {
    return; // wait until button pressed
  }

  static bool tick_mod = false;
  tick_mod = !tick_mod;
  if (!tick_mod)
    return;
  if (kill_player_offscreen())
    return;

  update_counters();

  if (pg->egg_count == 0)
    return;

  move_elevators();
  move_birds();
  collision_calculations();
  move_player();
}
