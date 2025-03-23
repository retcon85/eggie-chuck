#include <stdbool.h>
#include "SMSlib.h"
#include "view_game.h"
#include "model_game.h"
#include "splash.h"
#include "title.h"

extern const char *levels[];

void main(void)
{
  // splash_show();
  title_show();
  model_game_init(levels, 8);

  view_game_init();

  while (true)
  {
    SMS_waitForVBlank();
    view_game_tick();
    unsigned int keys = SMS_getKeysHeld();
    if (keys & PORT_A_KEY_UP)
    {
      model_game_player_up();
    }
    else if (keys & PORT_A_KEY_DOWN)
    {
      model_game_player_down();
    }
    if (keys & PORT_A_KEY_LEFT)
    {
      model_game_player_left();
    }
    else if (keys & PORT_A_KEY_RIGHT)
    {
      model_game_player_right();
    }

    keys = SMS_getKeysPressed();
    if (keys & (PORT_A_KEY_1 | PORT_A_KEY_2))
    {
      model_game_player_jump();
    }
    model_game_tick();
  }
}
