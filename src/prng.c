#include <stdint.h>
#include "prng.h"

static uint8_t random = 0x49;

void prng_set_seed(uint8_t seed)
{
  if (seed == 0)
  {
    random = 0x49;
    return;
  }
  random = seed;
}

uint8_t prng_next(void)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    uint8_t lsb = random & 1;
    random >>= 1;
    if (lsb == 1)
      random ^= 0xb8;
  }
  return random;
}
