#include <stdint.h>
#include "SMSlib.h"
#include "PSGlib.h"
#include "assets.generated.h"
#include "view_game.h"
#include "model_game.h"

#define TILESET_START 41
#define SPRITES_START 256
#define TILE_BLANK (TILESET_START + 0)
#define TILE_PLATFORM (TILESET_START + 1)
#define TILE_LADDER (TILESET_START + 3)
#define TILE_EGG (TILESET_START + 5)
#define TILE_GRAIN (TILESET_START + 7)
#define TILE_LIFE (TILESET_START + 10)
#define TILE_DUCK (TILESET_START + 11)
#define TILE_TEXT_SCORE (TILESET_START + 41)
#define TILE_TEXT_STOP (TILE_TEXT_SCORE + 3)
#define TILE_TEXT_PLAYER (TILE_TEXT_SCORE + 4)
#define TILE_TEXT_LEVEL (TILE_TEXT_PLAYER + 3)
#define TILE_TEXT_BONUS (TILE_TEXT_LEVEL + 3)
#define TILE_TEXT_BLANK (TILE_TEXT_BONUS + 3)
#define TILE_TEXT_TIME (TILE_TEXT_BONUS + 4)
#define TILE_TEXT_NUMBERS (TILE_TEXT_TIME + 2)
#define TILE_TEXT_SINGLE_NUMBERS (TILE_TEXT_NUMBERS + 100)
#define TILE_GET_READY_TEXT (TILESET_START + 167)
#define TILE_GAME_OVER_TEXT (TILE_GET_READY_TEXT + 14)
#define TILE_PLAYER_TEXT (TILE_GAME_OVER_TEXT + 14)
#define TILE_PLAYER (SPRITES_START + 0)
#define TILE_ELEVATOR (SPRITES_START + 36)
#define TILE_BIRD (SPRITES_START + 42)

static void render_field(uint8_t field[], uint8_t size, bool zero_pad)
{
  for (uint8_t i = 0; i < size; i++)
  {
    if (!zero_pad)
    {
      if (field[size - i - 1] == 0 && i < size - 1)
      {
        SMS_setTile(TILE_TEXT_BLANK);
        continue;
      }
      else if (field[size - i - 1] < 10)
      {
        SMS_setTile(TILE_TEXT_SINGLE_NUMBERS + field[size - i - 1]);
        zero_pad = true;
        continue;
      }
    }
    SMS_setTile(TILE_TEXT_NUMBERS + field[size - i - 1]);
    zero_pad = true;
  }
  SMS_setTile(TILE_TEXT_STOP);
}

static void update_scorelines(void)
{
  SMS_setNextTileatXY(0, 0);

  // score
  unsigned int t = TILE_TEXT_SCORE;
  for (uint8_t i = 0; i < 4; i++, t++)
  {
    SMS_setTile(t);
  }
  render_field(pg->score, sizeof(pg->score), true);

  // current player
  t = TILE_TEXT_PLAYER;
  for (uint8_t i = 0; i < 3; i++, t++)
  {
    SMS_setTile(t);
  }
  SMS_setTile(TILE_TEXT_SINGLE_NUMBERS + m.current_player + 1);
  SMS_setTile(TILE_TEXT_STOP);

  // level
  t = TILE_TEXT_LEVEL;
  for (uint8_t i = 0; i < 3; i++, t++)
  {
    SMS_setTile(t);
  }
  SMS_setTile(TILE_TEXT_NUMBERS + pg->level + 1);
  SMS_setTile(TILE_TEXT_STOP);

  // bonus
  t = TILE_TEXT_BONUS;
  for (uint8_t i = 0; i < 3; i++, t++)
  {
    SMS_setTile(t);
  }
  render_field(pg->bonus, sizeof(pg->bonus), false);

  // time
  t = TILE_TEXT_TIME;
  for (uint8_t i = 0; i < 2; i++, t++)
  {
    SMS_setTile(t);
  }
  render_field(pg->time, sizeof(pg->time), false);

  // lives
  SMS_setNextTileatXY(0, 1);
  for (uint8_t i = 0; i < 32; i++)
  {
    SMS_setTile(pg->lives > i ? TILE_LIFE : TILE_BLANK);
  }
}

static void full_redraw(void)
{
  SMS_displayOff();
  SMS_setNextTileatLoc(0);
  for (uint8_t y = 0; y < SCREEN_MAP_HEIGHT; y++)
  {
    for (uint8_t x = 0; x < SCREEN_MAP_WIDTH; x++)
    {
      const uint8_t type = pg->screen_map[y][x] & 0x0f;
      const uint8_t var = pg->screen_map[y][x] >> 4;
      switch (type)
      {
      case LADDER:
      case PLATFORM | LADDER:
        SMS_setTile(TILE_LADDER + var);
        break;
      case PLATFORM:
        SMS_setTile(TILE_PLATFORM + var);
        break;
      case EGG:
        SMS_setTile(TILE_EGG + var);
        break;
      case GRAIN:
        SMS_setTile(TILE_GRAIN + var);
        break;
      default:
        SMS_setTile(TILE_BLANK);
        break;
      }
    }
  }

  // draw duck
  unsigned int t = TILE_DUCK;
  for (uint8_t y = 0; y < 6; y++)
  {
    SMS_setNextTileatXY(0, 2 + y);
    for (uint8_t x = 0; x < 5; x++, t++)
    {
      SMS_setTile(t);
    }
  }
  SMS_displayOn();
}

void clear_screen(void)
{
  SMS_setNextTileatLoc(0);
  for (uint8_t y = 0; y < SCREEN_MAP_HEIGHT; y++)
  {
    for (uint8_t x = 0; x < SCREEN_MAP_WIDTH; x++)
    {
      SMS_setTile(TILE_BLANK);
    }
  }
}

void view_game_init(void)
{
  SMS_saveROMBank();
  SMS_displayOff();
  clear_screen();
  SMS_setSpriteMode(SPRITEMODE_TALL);
  SMS_mapROMBank(game_tiles_bin_bank);
  SMS_loadTiles(game_tiles_bin, TILESET_START, game_tiles_bin_size);
  SMS_mapROMBank(game_palette_bin_bank);
  SMS_loadBGPalette(game_palette_bin);
  SMS_loadSpritePalette(&game_palette_bin[16]);
  full_redraw();
  update_scorelines();
  SMS_restoreROMBank();

  SMS_displayOn();
}

char sfx_tone[] = {
    0x80, // set tone 1
    0x40, // set tone 2
    0x90, // set volume
    0x38, // hold
    0x9f, // set volume
    0x39, // hold
    0x00, // eof
};

char sfx_noise[] = {
    0xe0, // set noise
    0xf0, // set volume
    0x38, // hold
    0xff, // set volume
    0x38, // hold
    0x00, // eof
};

void view_game_tick(void)
{
  PSGSFXFrame();
  // if (pg->birds[0].is_climbing)
  // {
  //   SMS_setBackdropColor(2); // pink
  // }
  // else if (pg->birds[0].is_over_ladder)
  // {
  //   SMS_setBackdropColor(1); // green
  // }
  // else if (pg->birds[0].is_on_platform)
  // {
  //   SMS_setBackdropColor(3); // yellow
  // }
  // if (pg->player.is_jumping)
  // {
  //   SMS_setBackdropColor(3); // yellow
  // }
  // else
  // {
  //   SMS_setBackdropColor(0);
  // }
  if ((m.render_mask & 0x01f) == VIEW_GAME_SHOW_GET_READY_SCREEN)
  {
    SMS_displayOff();
    SMS_initSprites();
    SMS_copySpritestoSAT();
    clear_screen();
    SMS_setNextTileatXY(8, 10);
    unsigned int t = TILE_GET_READY_TEXT;
    // G e t
    for (uint8_t i = 0; i < 6; i++)
    {
      SMS_setTile(t++);
    }
    // (space)
    SMS_setTile(TILE_BLANK);
    SMS_setTile(TILE_BLANK);
    // R
    SMS_setTile(t++);
    SMS_setTile(t++);
    // e
    SMS_setTile(TILE_GET_READY_TEXT + 2);
    SMS_setTile(TILE_GET_READY_TEXT + 3);
    // a d y
    for (uint8_t i = 0; i < 6; i++)
    {
      SMS_setTile(t++);
    }
    t = TILE_PLAYER_TEXT;
    SMS_setNextTileatXY(9, 13);
    // P l a y e r
    for (uint8_t i = 0; i < 12; i++)
    {
      SMS_setTile(t++);
    }
    // (space)
    SMS_setTile(TILE_BLANK);
    SMS_setTile(TILE_BLANK);
    // (player number)
    t += 2 * m.current_player;
    SMS_setTile(t++);
    SMS_setTile(t);
    SMS_displayOn();
    m.render_mask &= ~VIEW_GAME_SHOW_GET_READY_SCREEN;
    return;
  }

  if ((m.render_mask & 0x01f) == VIEW_GAME_SHOW_GET_GAME_OVER_OVERLAY)
  {
    SMS_initSprites();
    SMS_copySpritestoSAT();
    for (uint8_t j = 0; j < 5; j++)
    {
      SMS_setNextTileatXY(7, j + 9);
      for (uint8_t i = 0; i < 19; i++)
      {
        SMS_setTile(TILE_BLANK);
      }
    }
    SMS_setNextTileatXY(8, 10);
    unsigned int t = TILE_GAME_OVER_TEXT;
    // G a m e
    for (uint8_t i = 0; i < 8; i++)
    {
      SMS_setTile(t++);
    }
    // (space)
    SMS_setTile(TILE_BLANK);
    SMS_setTile(TILE_BLANK);
    // O v
    SMS_setTile(t++);
    SMS_setTile(t++);
    SMS_setTile(t++);
    SMS_setTile(t++);
    // e
    SMS_setTile(TILE_GAME_OVER_TEXT + 6);
    SMS_setTile(TILE_GAME_OVER_TEXT + 7);
    // r
    SMS_setTile(t++);
    SMS_setTile(t++);
    // (space)
    SMS_setTile(TILE_BLANK);
    SMS_setTile(TILE_BLANK);
    t = TILE_PLAYER_TEXT;
    SMS_setNextTileatXY(9, 13);
    // P l a y e r
    for (uint8_t i = 0; i < 12; i++)
    {
      SMS_setTile(t++);
    }
    // (space)
    SMS_setTile(TILE_BLANK);
    SMS_setTile(TILE_BLANK);
    // (player number)
    t += 2 * m.current_player;
    SMS_setTile(t++);
    SMS_setTile(t);
    m.render_mask &= ~VIEW_GAME_SHOW_GET_GAME_OVER_OVERLAY;
    return;
  }

  if (m.render_mask & VIEW_GAME_WAIT)
  {
    return;
  }

  SMS_copySpritestoSAT();
  SMS_initSprites();
  SMS_addSprite(pg->player.x, pg->player.y - 16, TILE_PLAYER + pg->player.animation_frame * 4);
  SMS_addSprite(pg->player.x + 8, pg->player.y - 16, TILE_PLAYER + pg->player.animation_frame * 4 + 2);
  for (uint8_t i = 0; i < pg->bird_count; i++)
  {
    SMS_addSprite(pg->birds[i].x, pg->birds[i].y - 24, TILE_BIRD);
    SMS_addSprite(pg->birds[i].x + 8, pg->birds[i].y - 24, TILE_BIRD + 2);
    SMS_addSprite(pg->birds[i].x, pg->birds[i].y - 8, TILE_BIRD + 4);
    SMS_addSprite(pg->birds[i].x + 8, pg->birds[i].y - 8, TILE_BIRD + 6);
  }
  if (pg->elevator_enabled)
  {
    uint8_t y = pg->elevator_pos.y;
    if (y > 16 && y < 192)
    {
      SMS_addSprite(pg->elevator_pos.x, y, TILE_ELEVATOR);
      SMS_addSprite(pg->elevator_pos.x + 8, y, TILE_ELEVATOR + 2);
      SMS_addSprite(pg->elevator_pos.x + 16, y, TILE_ELEVATOR + 4);
    }
    y -= 112;
    if (y > 16 && y < 192)
    {
      SMS_addSprite(pg->elevator_pos.x, y, TILE_ELEVATOR);
      SMS_addSprite(pg->elevator_pos.x + 8, y, TILE_ELEVATOR + 2);
      SMS_addSprite(pg->elevator_pos.x + 16, y, TILE_ELEVATOR + 4);
    }
  }

  if (m.render_mask & VIEW_GAME_RENDER_FULL_REDRAW)
  {
    m.render_mask &= ~VIEW_GAME_RENDER_FULL_REDRAW;
    full_redraw();
    return;
  }
  if (m.render_mask & VIEW_GAME_RENDER_SCORELINE)
  {
    update_scorelines();
    m.render_mask &= ~VIEW_GAME_RENDER_SCORELINE;
  }
  if (m.render_mask & VIEW_GAME_RENDER_PICKUP)
  {
    SMS_setNextTileatXY(pg->player.x / 8, pg->player.y / 8 - 1);
    SMS_setTile(TILE_BLANK);
    SMS_setTile(TILE_BLANK);

    m.render_mask &= ~VIEW_GAME_RENDER_PICKUP;
  }
  // cue sound effects
  if (m.audio_tone > 0)
  {
    if (PSGSFXGetStatus() == PSG_STOPPED)
    {
      if (m.audio_noise)
      {
        sfx_noise[0] = 0xe0 | (m.audio_tone & 0x07);
        PSGSFXPlay(sfx_noise, 0x00);
      }
      else
      {
        sfx_tone[0] = 0x80 | (m.audio_tone & 0x0f);
        sfx_tone[1] = 0x40 | (m.audio_tone >> 4);
        PSGSFXPlay(sfx_tone, 0x00);
      }
      m.audio_tone = 0;
    }
  }
}
