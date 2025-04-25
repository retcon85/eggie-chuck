#include "model_game.h"
#include "prng.h"

extern uint8_t bird_counter;

void model_game_life_over(void);

inline bool check_player_collision(struct character_t *bird)
{
  if (!(
          (bird->x + 12 < pg->player.x) || // !TODO:
          (bird->x > pg->player.x + 12) || // !TODO:
          (bird->y < pg->player.y - 14) || // !TODO:
          (bird->y - 18 > pg->player.y)))  // !TODO:
  {
    return true;
  }
  return false;
}

inline void move_horizontally(struct character_t *bird)
{
  // choose a direction if we have none yet
  if (bird->vx == 0)
  {
    bird->vx = (prng_next() & 0x01) ? BIRD_SPEED : -BIRD_SPEED;
  }
  // move bird
  bird->x += bird->vx;
  // if hit edge of screen, turn around
  if (bird->x > (256 - 16)) // !TODO:
  {
    // if hit edge of screen, turn around
    bird->vx = -bird->vx;
    bird->x += bird->vx;
    bird->x += bird->vx;
  }
  // start with bottom (feet) of bird
  uint8_t *tile = &pg->screen_map[bird->y / 8 - 1][bird->x / 8];
  // if over ladder...
  if ((*tile & 0x18) == LADDER)
  {
    // decide whether or not to climb (~25% chance)
    if (!(prng_next() & 0x03))
    {
      bird->vx = 0;
      bird->is_climbing = true;
      bird->is_on_platform = false;
      bird->vy = (prng_next() & 0x01) ? BIRD_SPEED : -BIRD_SPEED;
      return;
    }
  }

  // if bird has walked over the edge of a platform, turn around
  tile += SCREEN_MAP_WIDTH;
  if (bird->vx > 0)
  {
    tile += 1;
  }
  if (((*tile & 0x07) != PLATFORM))
  {
    bird->vx = -bird->vx;
    bird->x += bird->vx;
    bird->x += bird->vx;
  }
}

inline void climb(struct character_t *bird)
{
  bird->y += bird->vy;
  uint8_t *tile = &pg->screen_map[bird->y / 8][bird->x / 8];
  // if reached platform, decide whether to get off ladder (~75% chance)
  if ((*tile & 0x07) == PLATFORM)
  {
    bird->vy = (prng_next() & 0x03) ? 0 : bird->vy;
    if (bird->vy == 0)
    {
      bird->is_climbing = false;
      bird->is_on_platform = true;
      bird->vx = (prng_next() & 0x01) ? BIRD_SPEED : -BIRD_SPEED;
      return;
    }
  }
  // if reached top or bottom of ladder and still climbing, turn around
  tile -= SCREEN_MAP_WIDTH;
  if (bird->vy > 0) // going down...
  {
    if ((*tile & 0x18) != LADDER)
    {
      bird->vy = -bird->vy;
      bird->y += bird->vy;
      bird->y += bird->vy;
    }
  }
  else if (bird->vy < 0) // going up...
  {
    // need to check top of bird instead of bottom
    if ((*tile & 0x18) != LADDER)
    {
      bird->vy = -bird->vy;
      bird->y += bird->vy;
      bird->y += bird->vy;
    }
  }
}

// checks collisions and periodically updates bird positions
void model_game_move_birds(void)
{
  for (uint8_t i = 0; i < pg->bird_count; i++)
  {
    struct character_t *bird = &pg->birds[i];
    if (check_player_collision(bird))
    {
      model_game_life_over();
      return;
    }

    if (bird_counter > 0)
      continue;

    if (!bird->is_climbing)
    {
      move_horizontally(bird);
      continue;
    }
    else
    {
      climb(bird);
    }
  }
}
