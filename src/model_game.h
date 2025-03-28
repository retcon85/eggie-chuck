#ifndef MODEL_GAME_H_
#define MODEL_GAME_H_

#include <stdint.h>
#include <stdbool.h>

#define VIEW_GAME_RENDER_FULL_REDRAW 1
#define VIEW_GAME_RENDER_SCORELINE 2
#define VIEW_GAME_RENDER_PICKUP 4
#define VIEW_GAME_SHOW_GET_READY_SCREEN 8
#define VIEW_GAME_SHOW_GET_GAME_OVER_OVERLAY 9
#define VIEW_GAME_WAIT 128

#define SCREEN_MAP_WIDTH 32
#define SCREEN_MAP_HEIGHT 24

#define MAX_PLAYERS 4
#define MAX_BIRDS 4

#define NOTHING 0
#define PLATFORM 1
#define EGG 2
#define GRAIN 4
#define ELEVATOR 5
#define LADDER 8

#define INITIAL_LIVES 5

#define JUMP_SPEED_MAX 32
#define ELEVATOR_SPEED 2
#define BIRD_SPEED 8
#define PLAYER_SPEED 2
#define JUMP_SPEED_INITIAL 16
#define LADDER_SPEED 2
#define GRAVITY 2
#define BIRD_COUNTER_FREQ 12
#define TIME_COUNTER_FREQ 7
#define BONUS_COUNTER_FREQ 5
#define BONUS_TRANSFER_RATE 20
#define GRAIN_BONUS_TIME 64
#define GET_READY_SCREEN_TIMEOUT 240

#define AUDIO_TONE_WALKING 0xd8
#define AUDIO_TONE_CLIMBING 0x69
#define AUDIO_TONE_JUMPING 0x69
#define AUDIO_TONE_FALLING 0xc8
#define AUDIO_NOISE_BONUS_TRANSFER 0x04
#define AUDIO_NOISE_PICKUP 0x04
#define AUDIO_STEP 3

struct upos_t
{
  uint8_t x;
  uint8_t y;
};

struct spos_t
{
  int8_t x;
  int8_t y;
};

struct character_t
{
  uint8_t x;
  uint8_t y;
  int8_t vx;
  int8_t vy;
  bool is_jumping;
  bool is_climbing;
  bool is_on_platform;
  bool is_over_ladder;
  bool is_on_elevator;
  uint8_t animation_frame;
};

struct player_game_t
{
  uint8_t score[3];
  uint8_t level;
  uint8_t lives;
  struct upos_t player_initial_pos;
  struct character_t player;
  struct character_t birds[MAX_BIRDS];
  uint8_t bird_count;
  uint8_t screen_map[SCREEN_MAP_HEIGHT][SCREEN_MAP_WIDTH];
  uint8_t egg_count;
  bool elevator_enabled;
  struct upos_t elevator_pos;
  uint8_t time[2];
  uint8_t bonus[2];
  bool bonus_exhausted;
  bool game_over;
};

struct game_model_t
{
  uint8_t current_player;
  uint8_t player_count;
  char **level_data;
  uint8_t level_count;
  uint8_t render_mask;
  uint8_t audio_tone;
  bool audio_noise;
  bool audio_music;
  struct player_game_t player_games[MAX_PLAYERS];
  bool reset;
};

void model_game_init(char *level_data[], uint8_t level_count);
void model_game_select_player(uint8_t player);
void model_game_tick(void);
void model_game_player_left(void);
void model_game_player_right(void);
void model_game_player_up(void);
void model_game_player_down(void);
void model_game_player_jump(void);

extern struct game_model_t m;
extern struct player_game_t *pg;

#endif
