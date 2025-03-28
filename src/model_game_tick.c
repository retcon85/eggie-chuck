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
    m.render_mask |= VIEW_GAME_RENDER_SCORELINE;
  }
}

static void collect_grain(void)
{
  update_score(50);
  m.render_mask |= VIEW_GAME_RENDER_PICKUP;
  m.audio_noise = true;
  m.audio_tone = AUDIO_NOISE_PICKUP;
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
  model_game_select_player(m.current_player);
}

static void collect_egg(void)
{
  if (pg->egg_count == 0)
    return;
  update_score(100);
  pg->egg_count--;
  m.render_mask |= VIEW_GAME_RENDER_PICKUP;
  m.audio_noise = true;
  m.audio_tone = AUDIO_NOISE_PICKUP;
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
  bool on_elevator = false;
  if (pg->elevator_enabled)
  {
    uint8_t elevator_y = pg->elevator_pos.y;
    if (time_counter % 2 == 0)
    {
      pg->elevator_pos.y -= ELEVATOR_SPEED;
    }

    if (pg->player.x >= pg->elevator_pos.x - 6 &&     // !TODO:
        pg->player.x <= pg->elevator_pos.x + 24 - 12) // !TODO:
    {
      for (uint8_t i = 0; i < 2; i++, elevator_y -= 112) // !TODO:
      {
        if (pg->player.y >= elevator_y &&
            pg->player.y <= elevator_y + 4)
        {
          pg->player.y = pg->elevator_pos.y - (i ? 112 : 0); // !TODO:
          pg->player.is_on_elevator = true;
        };
      }
    }
  }
}

inline void check_object_collection(uint8_t *tile)
{
  if ((*tile & 0x0f) == EGG)
  {
    collect_egg();
    *tile = NOTHING;
  }
  else if ((*tile & 0x0f) == GRAIN)
  {
    collect_grain();
    *tile = NOTHING;
  }
}

inline void collision_calculations(void)
{
  if (pg->player.is_on_elevator)
  {
    pg->player.is_on_platform = false;
    pg->player.is_over_ladder = false;
    return;
  }

  pg->player.is_on_platform = false;
  pg->player.is_over_ladder = false;
  // check all four background tiles behind player, starting with top left
  uint8_t *tile = &pg->screen_map[pg->player.y / 8 - 1][pg->player.x / 8];
  check_object_collection(tile);
  if ((*tile & 0x18) == LADDER)
  {
    pg->player.is_over_ladder = true;
  }
  // top right
  tile++;
  check_object_collection(tile);
  // bottom left
  tile += SCREEN_MAP_WIDTH - 1;
  // need to be fully on a ladder tile to use it
  if (!pg->player.is_over_ladder || (*tile & 0x18) != LADDER || (pg->player.x & 0x07))
  {
    pg->player.is_over_ladder = false;
    check_object_collection(tile);
    if ((*tile & 0x07) == PLATFORM)
    {
      pg->player.is_on_platform = true;
    }
  }
  // bottom right
  tile++;
  check_object_collection(tile);
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
            (bird->x + 16 < pg->player.x) || // !TODO:
            (bird->x > pg->player.x + 16) || // !TODO:
            (bird->y < pg->player.y - 16) || // !TODO:
            (bird->y - 24 > pg->player.y)))  // !TODO:
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

  // if we are climbing but tried to move off the ladder, don't allow it
  if (pg->player.is_climbing && !pg->player.is_over_ladder && !pg->player.is_on_platform)
  {
    pg->player.vx = 0;
    pg->player.vy = 0;
    pg->player.x = (pg->player.x + 4) & 0xf8;
    pg->player.y = (pg->player.y + 4) & 0xf8 - 1;
    pg->player.is_over_ladder = true;
    // return;
  }

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
    pg->player.x = (256 - 16);
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
    pg->player.y += pg->player.vy;
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
