#ifndef _GOOMTOOLS_H
#define _GOOMTOOLS_H
#include "goomutils/logging.h"

#include <inttypes.h>

/**
 * Random number generator wrapper for faster random number.
 */

#ifdef _WIN32PC
#define inline __inline
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define random rand
#define bzero(x, y) memset(x, 0, y)
#endif

// Permuted congruential generator.
uint32_t pcg32_rand(void);
void pcg32_init(uint64_t seed);
uint64_t pcg32_get_seed();
uint64_t pcg32_get_last_state();
void pcg32_set_state(uint64_t s);

#define GOOM_NB_RAND 0x10000

struct GoomRandom
{
  uint32_t array[GOOM_NB_RAND];
  unsigned short pos;
};

GoomRandom* goom_random_init();
void goom_random_free(GoomRandom* grandom);

inline static uint32_t goom_random(GoomRandom* grandom)
{
  grandom->pos++; /* works because pos is an unsigned short */
  const uint32_t val = grandom->array[grandom->pos];
  logDebug("{}, {}", grandom->pos, val);
  return val;
}

inline static uint32_t goom_irand(GoomRandom* grandom, uint32_t i)
{
  grandom->pos++;
  logDebug("{}, {}", grandom->pos, grandom->array[grandom->pos]);
  return grandom->array[grandom->pos] % i;
}

/* called to change the specified number of value in the array, so that the array does not remain the same*/
void goom_random_update_array(GoomRandom* grandom, int numberOfValuesToChange);

#endif
