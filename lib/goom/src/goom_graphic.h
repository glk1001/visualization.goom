#ifndef GRAPHIC_H
#define GRAPHIC_H

#include "goom_config.h"

#include <cstdint>

struct Color
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

extern const Color BLACK;
extern const Color WHITE;
extern const Color RED;
extern const Color BLUE;
extern const Color GREEN;
extern const Color YELLOW;
extern const Color ORANGE;
extern const Color VIOLET;

#ifdef COLOR_BGRA
#define A_CHANNEL 0x000000FF
#define R_OFFSET 24
#define G_OFFSET 16
#define B_OFFSET 8
#define A_OFFSET 0

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
#define A_CHANNEL 0xFF000000
#define A_OFFSET 24
#define R_OFFSET 16
#define G_OFFSET 8
#define B_OFFSET 0

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

#endif
