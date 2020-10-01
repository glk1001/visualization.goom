#ifndef _GOOM_GRAPHIC_H
#define _GOOM_GRAPHIC_H

#include "goom_config.h"

#include <cstdint>

namespace goom
{

#ifdef COLOR_BGRA
#define A_CHANNEL 0x000000FFu
#define R_OFFSET 24u
#define G_OFFSET 16u
#define B_OFFSET 8u
#define A_OFFSET 0u

#define ALPHA 3u
#define BLEU 2u
#define VERT 1u
#define ROUGE 0u

union Pixel
{
  struct
  {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
  } channels;
  uint32_t val;
  uint8_t cop[4];
};

#else // not COLOR_BGRA
#define A_CHANNEL 0xFF000000u
#define A_OFFSET 24u
#define R_OFFSET 16u
#define G_OFFSET 8u
#define B_OFFSET 0u

#define BLEU 3u
#define VERT 2u
#define ROUGE 1u
#define ALPHA 0u

union Pixel
{
  struct
  {
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
  } channels;
  uint32_t val;
  uint8_t cop[4];
};

#endif /* COLOR_BGRA */

} // namespace goom
#endif
