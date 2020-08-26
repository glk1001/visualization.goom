#ifndef _DRAWMETHODS_H
#define _DRAWMETHODS_H

#include "goom_graphic.h"

#include <cstdint>
#include <vector>

extern void draw_line(const size_t n,
                      Pixel* buffs[],
                      const std::vector<Pixel>& cols,
                      const int x1,
                      const int y1,
                      const int x2,
                      const int y2,
                      const uint16_t screenx,
                      const uint16_t screeny);

extern void draw_line(Pixel* data,
                      int x1,
                      int y1,
                      int x2,
                      int y2,
                      const uint32_t col,
                      const uint16_t screenx,
                      const uint16_t screeny);

#endif
