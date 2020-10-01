#ifndef _DRAWMETHODS_H
#define _DRAWMETHODS_H

#include "goom_graphic.h"

#include <cstdint>
#include <vector>

namespace goom
{

void draw_line(const size_t n,
               Pixel* buffs[],
               const std::vector<Pixel>& colors,
               const int x1,
               const int y1,
               const int x2,
               const int y2,
               const uint32_t screenx,
               const uint32_t screeny);

void draw_line(Pixel* buff,
               int x1,
               int y1,
               int x2,
               int y2,
               const uint32_t color,
               const uint32_t screenx,
               const uint32_t screeny);

void circle(Pixel* buff,
            const int x0,
            const int y0,
            const int radius,
            const uint32_t color,
            const uint32_t screenWidth,
            const uint32_t screenHeight);

// colors.size() == radius
void filledCircle(Pixel* buff,
                  const int x0,
                  const int y0,
                  const int radius,
                  const std::vector<uint32_t> colors,
                  const uint32_t screenWidth,
                  const uint32_t screenHeight);

} // namespace goom
#endif
