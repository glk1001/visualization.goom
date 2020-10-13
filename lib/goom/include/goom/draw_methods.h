#ifndef _DRAW_METHODS_H
#define _DRAW_METHODS_H

#include "goom_graphic.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace goom
{

void drawCircle(Pixel* buff,
                const int x0,
                const int y0,
                const int radius,
                const uint32_t color,
                const uint32_t buffIntensity,
                const bool allowOverexposure,
                const uint32_t screenWidth,
                const uint32_t screenHeight);

// colors.size() == radius
void drawFilledCircle(Pixel* buff,
                      const int x0,
                      const int y0,
                      const int radius,
                      const std::vector<uint32_t> colors,
                      const uint32_t buffIntensity,
                      const bool allowOverexposure,
                      const uint32_t screenWidth,
                      const uint32_t screenHeight);

void drawLine(const size_t n,
              Pixel* buffs[],
              const int x1,
              const int y1,
              const int x2,
              const int y2,
              const std::vector<Pixel>& colors,
              const uint32_t buffIntensity,
              const bool allowOverexposure,
              const uint8_t thickness,
              const uint32_t screenx,
              const uint32_t screeny);

void drawLine(Pixel* buff,
              int x1,
              int y1,
              int x2,
              int y2,
              const uint32_t color,
              const uint32_t buffIntensity,
              const bool allowOverexposure,
              const uint8_t thickness,
              const uint32_t screenx,
              const uint32_t screeny);

} // namespace goom
#endif
