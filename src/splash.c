#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "SMSlib.h"
#ifdef USEPSGLIB
#include "PSGlib.h"
#endif
#include "assets.generated.h"

const unsigned char color_reduction[] = {RGB(0, 0, 0), RGB(0, 0, 1), RGB(0, 1, 1), RGB(1, 1, 1),
                                         RGB(1, 1, 2), RGB(1, 2, 2), RGB(2, 2, 2),
                                         RGB(2, 2, 3), RGB(2, 3, 3), RGB(3, 3, 3)};

#define COLOR_SUBTRACT(c, r) (((((r) & 0x3) >= ((c) & 0x3)) ? 0 : (((c) & 0x3) - ((r) & 0x3))) | ((((r) & 0xC) >= ((c) & 0xC)) ? 0 : (((c) & 0xC) - ((r) & 0xC))) | ((((r) & 0x30) >= ((c) & 0x30)) ? 0 : (((c) & 0x30) - ((r) & 0x30))))

inline void clear_screen(const char *with)
{
  SMS_setNextTileatLoc(0);
  for (int i = 0; i < 896; i++)
    SMS_print(with);
}

void _SMS_loadBGPaletteafterColorSubtraction(const void *palette, const unsigned char subtraction_color)
{
  unsigned char i;
  unsigned char j = 0;
  SMS_setNextBGColoratIndex(0);
  // precalculate the first four colors so that we tie up the CPU long enough to miss the bottom border
  unsigned char new_colors[16];
  for (i = 0; i < 16; j = i++)
  {
    new_colors[i] = COLOR_SUBTRACT(((unsigned char *)(palette))[i], subtraction_color);
    if (SMS_getVCount() >= 216)
      break;
  }
  for (i = 0; i <= j; i++)
    SMS_setColor(new_colors[i]);

  for (; i < 16; i++)
    SMS_setColor(COLOR_SUBTRACT(((unsigned char *)(palette))[i], subtraction_color));
}

void _SMS_loadSpritePaletteafterColorSubtraction(const void *palette, const unsigned char subtraction_color)
{
  unsigned char i;
  unsigned char j = 0;
  SMS_setNextSpriteColoratIndex(0);
  // precalculate the first four colors so that we tie up the CPU long enough to miss the bottom border
  unsigned char new_colors[16];
  for (i = 0; i < 16; j = i++)
  {
    new_colors[i] = COLOR_SUBTRACT(((unsigned char *)(palette))[i], subtraction_color);
    if (SMS_getVCount() >= 216)
      break;
  }
  for (i = 0; i <= j; i++)
    SMS_setColor(new_colors[i]);

  for (; i < 16; i++)
    SMS_setColor(COLOR_SUBTRACT(((unsigned char *)(palette))[i], subtraction_color));
}

static void fade_in(const void *bg_palette, const void *sprite_palette, unsigned int sprite_palette_bank)
{
  SMS_displayOff();
  for (int i = sizeof(color_reduction) - 1; i >= 0; i--)
  {
    SMS_waitForVBlank();

    if (bg_palette != NULL)
      _SMS_loadBGPaletteafterColorSubtraction(bg_palette, color_reduction[i]);

    SMS_waitForVBlank();
    if (sprite_palette != NULL)
    {
      SMS_saveROMBank();
      SMS_mapROMBank(sprite_palette_bank);
      _SMS_loadSpritePaletteafterColorSubtraction(sprite_palette, color_reduction[i]);
      SMS_restoreROMBank();
    }
    if (i == sizeof(color_reduction) - 1)
      SMS_displayOn();
  }
}

static void fade_out(const void *bg_palette, const void *sprite_palette, unsigned int sprite_palette_bank)
{
  for (int i = 0; i < sizeof(color_reduction); i++)
  {
    SMS_waitForVBlank();

    if (bg_palette != NULL)
      _SMS_loadBGPaletteafterColorSubtraction(bg_palette, color_reduction[i]);

    SMS_waitForVBlank();
    if (sprite_palette != NULL)
    {
      SMS_saveROMBank();
      SMS_mapROMBank(sprite_palette_bank);
      _SMS_loadSpritePaletteafterColorSubtraction(sprite_palette, color_reduction[i]);
      SMS_restoreROMBank();
    }
  }
  SMS_displayOff();
}

inline void cancelable_wait(int seconds)
{
  for (int i = 0; i < 60 * seconds; i++)
  {
    SMS_waitForVBlank();
    if (SMS_getKeysHeld())
      return;
  }
}

#pragma save
#pragma disable_warning 85
static void splash_bars(uint8_t freq, uint16_t repetitions)
{
  // freq will be in a
  // repetitions will be in de

  // init
  __asm__("ld l, a        ; save freq");
  __asm__("ld h, #2       ; first color (2)");
  __asm__("ex af, af';    ; use shadow a register");
  __asm__("ld a, #0x9f    ; ch1 volume (0)");
  __asm__("out(#0x7f), a  ; ch1 volume (0)");
  __asm__("ex af, af';    ; switch back to normal register");
  __asm__("ld a, #0x80    ; ch1 'AM' tone");
  __asm__("out(#0x7f), a  ; ch1 'AM' tone");
  __asm__("xor a          ; ch1 'AM' tone");
  __asm__("out(#0x7f), a  ; ch1 'AM' tone");

  // backdrop pattern
  __asm__("rpt:");
  __asm__("ld a, h        ; set backdrop color");
  __asm__("out(#0xbf), a  ; set backdrop color");
  __asm__("ld a, #0x87    ; set backdrop color");
  __asm__("out(#0xbf), a  ; set backdrop color");
  __asm__("ld b, l        ; restore freq");
  __asm__("wait:           ; wait");
  __asm__("djnz wait       ; wait");

  // sound
  __asm__("ex af, af'     ; use shadow register for volume");
  __asm__("cpl            ; complement volume value");
  __asm__("or #0x90       ; prepare for PSG volume register");
  __asm__("and #0x9f      ; prepare for PSG volume register");
  __asm__("out (#0x7f), a ; write volume to PSG");
  __asm__("ex af, af'     ; switch back to normal register");

  // switch backdrop color
  __asm__("ld a, h        ; load current color from h");
  __asm__("xor #0x01        ; flip bit 0");
  __asm__("ld h, a        ; store color to h");

  // repeat
  __asm__("dec e          ; decrement counter lsb");
  __asm__("jr nz, rpt     ; repeat if not zero");
  __asm__("xor a          ; test counter msb");
  __asm__("or d           ; test counter msb");
  __asm__("jr z, quit     ; if zero, quit");
  __asm__("dec d          ; decrement counter msb");
  __asm__("jr rpt         ; repeat");

  // quit
  __asm__("quit:");
  __asm__("ld a, #0x9f      ; silence audio channel");
  __asm__("out (#0x7f), a ; silence audio channel");
}
#pragma restore

void splash_show(void)
{
  // set up tile and palette for splash
  SMS_displayOff();
  SMS_initSprites();
  SMS_copySpritestoSAT();
  SMS_setBackdropColor(0);
  SMS_mapROMBank(retcon_tiles_bin_bank);
  SMS_loadTiles(retcon_tiles_bin, 0, retcon_tiles_bin_size);
  SMS_mapROMBank(splash_font_bin_bank);
  SMS_loadTiles(splash_font_bin, 32, splash_font_bin_size);
  SMS_loadBGPalette(retcon_palettes_bin);
  SMS_loadSpritePalette(&retcon_palettes_bin[16]);
  SMS_mapROMBank(retcon_tilemap_bin_bank);
  SMS_loadTileMap(0, 0, retcon_tilemap_bin, retcon_tilemap_bin_size);
  SMS_mapROMBank(retcon_palettes_bin_bank);

  // do speccy jazz
  const char jazz_str[] = "AllWorkAndNoPlayMakesJackADullBoyAllWorkAndNoPlayMakesJackADullBoy";
  SMS_setSpritePaletteColor(2, 0x3c); // red
  SMS_setSpritePaletteColor(3, 0x03); // cyan
  __asm__("di");
  splash_bars(146, 3233);
  SMS_setSpritePaletteColor(2, 0x30); // blue
  SMS_setSpritePaletteColor(3, 0x0f); // yellow
  __asm__("di");
  for (register uint8_t i = 0; i < sizeof(jazz_str) - 1; i++)
  {
    splash_bars((jazz_str[i] & 0x01) ? 19 : 52, 2);
    splash_bars((jazz_str[i] & 0x02) ? 19 : 52, 2);
    splash_bars((jazz_str[i] & 0x04) ? 19 : 52, 2);
    splash_bars((jazz_str[i] & 0x08) ? 19 : 52, 2);
    splash_bars((jazz_str[i] & 0x10) ? 19 : 52, 2);
    splash_bars((jazz_str[i] & 0x20) ? 19 : 52, 2);
    splash_bars((jazz_str[i] & 0x40) ? 19 : 52, 2);
    splash_bars((jazz_str[i] & 0x80) ? 19 : 52, 2);
  }
  __asm__("ei");

  // display retcon logo for 3 seconds
  SMS_setBackdropColor(0);
  SMS_setSpritePaletteColor(3, 0);
  SMS_setSpritePaletteColor(4, 0);
  SMS_displayOn();
  cancelable_wait(2);

  fade_out(retcon_palettes_bin, &retcon_palettes_bin[16], retcon_palettes_bin_bank);
  clear_screen(" ");
  cancelable_wait(2);

  // !WB: don't feel like this for this game right now

  // // SMS_mapROMBank(retcon_palettes_bin_bank);
  // SMS_printatXY(5, 11, "Undeveloper_ presents");
  // fade_in(retcon_palettes_bin, &retcon_palettes_bin[16], retcon_palettes_bin_bank);
  // cancelable_wait(3);
  // fade_out(retcon_palettes_bin, &retcon_palettes_bin[16], retcon_palettes_bin_bank);
  // clear_screen(" ");
  // cancelable_wait(2);
  // SMS_printatXY(6, 11, "A Retcon production");
  // fade_in(retcon_palettes_bin, &retcon_palettes_bin[16], retcon_palettes_bin_bank);
  // cancelable_wait(3);
  // fade_out(retcon_palettes_bin, &retcon_palettes_bin[16], retcon_palettes_bin_bank);
  // cancelable_wait(2);
}
