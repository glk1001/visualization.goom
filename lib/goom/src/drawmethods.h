#ifndef _DRAWMETHODS_H
#define _DRAWMETHODS_H

#include "goom_graphic.h"

#include <cstdint>
#include <vector>

void draw_line(const size_t n,
               Pixel* buffs[],
               const std::vector<Pixel>& colors,
               const int x1,
               const int y1,
               const int x2,
               const int y2,
               const uint32_t screenx,
               const uint32_t screeny);

void draw_line(Pixel* data,
               int x1,
               int y1,
               int x2,
               int y2,
               const uint32_t color,
               const uint32_t screenx,
               const uint32_t screeny);

#endif
