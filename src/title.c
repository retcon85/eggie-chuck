#include <stdint.h>
#include "SMSlib.h"
#include "assets.generated.h"
#include "model_game.h"

#define TITLE_WIDTH 20
#define TITLE_HEIGHT 10
#define TITLE_X 6
#define TITLE_Y 3
#define TITLE_TILES 128

void title_show(void)
{
  SMS_saveROMBank();
  SMS_displayOff();
  SMS_mapROMBank(game_palette_bin_bank);
  SMS_loadBGPalette(game_palette_bin);
  SMS_loadSpritePalette(&game_palette_bin[16]);
  SMS_mapROMBank(splash_font_bin_bank);
  SMS_loadTiles(splash_font_bin, ' ', splash_font_bin_size);
  SMS_mapROMBank(title_tiles_bin_bank);
  SMS_loadTiles(title_tiles_bin, TITLE_TILES, title_tiles_bin_size);
  SMS_restoreROMBank();

  SMS_setNextTileatLoc(0);
  for (int i = 0; i < 896; i++)
    SMS_setTile(' ');

  unsigned int t = TITLE_TILES;
  for (uint8_t j = 0, y = TITLE_Y; j < TITLE_HEIGHT; j++, y++)
  {
    SMS_setNextTileatXY(TITLE_X, y);
    for (uint8_t i = 0; i < TITLE_WIDTH; i++)
    {
      SMS_setTile(t++);
    }
  }
  SMS_displayOn();
  SMS_setNextTileatXY(1, 14);
  SMS_print("An eggstremely UNoriginal game");
  SMS_setNextTileatXY(1, 17);
  SMS_print("Number of players? 1");

  m.player_count = 1;

  unsigned int keys = 0;
  while (1)
  {
    SMS_waitForVBlank();
    keys = SMS_getKeysPressed();
    if (keys & PORT_A_KEY_1)
    {
      break;
    }
    else if (keys & PORT_A_KEY_UP)
    {
      m.player_count++;
      if (m.player_count > MAX_PLAYERS)
      {
        m.player_count = 1;
      }
    }
    else if (keys & PORT_A_KEY_DOWN)
    {
      m.player_count--;
      if (m.player_count == 0)
      {
        m.player_count = MAX_PLAYERS;
      }
    }
    else
    {
      continue;
    }
    SMS_setNextTileatXY(20, 17);
    SMS_setTile(m.player_count + '0');
  }
}
