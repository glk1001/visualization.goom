#ifndef _GFONTRLE_H
#define _GFONTRLE_H

#include <cstdint>

struct TheFont
{
  uint16_t width;
  uint16_t height;
  uint16_t bytes_per_pixel;
  uint16_t rle_size;
  unsigned char rle_pixel[49725];
};

extern const TheFont the_font;

#endif
