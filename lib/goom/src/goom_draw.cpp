#include "goom_draw.h"

#include "draw_methods.h"
#include "goom_config.h"
#include "goom_graphic.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace goom
{

GoomDraw::GoomDraw(const uint32_t screenW, const uint32_t screenH)
  : screenWidth{screenW}, screenHeight{screenH}
{
  setBuffIntensity(0.5);
  setAllowOverexposed(true);
}

void GoomDraw::circle(
    Pixel* buff, const int x0, const int y0, const int radius, const uint32_t color)
{
  drawCircle(buff, x0, y0, radius, color, intBuffIntensity, allowOverexposed, screenWidth,
             screenHeight);
}

void GoomDraw::filledCircle(
    Pixel* buff, const int x0, const int y0, const int radius, const std::vector<uint32_t>& colors)
{
  drawFilledCircle(buff, x0, y0, radius, colors, intBuffIntensity, allowOverexposed, screenWidth,
                   screenHeight);
}

void GoomDraw::line(Pixel* buff,
                    const int x1,
                    const int y1,
                    const int x2,
                    const int y2,
                    const uint32_t color,
                    const uint8_t thickness)
{
  drawLine(buff, x1, y1, x2, y2, color, intBuffIntensity, thickness, allowOverexposed, screenWidth,
           screenHeight);
}

void GoomDraw::line(const size_t n,
                    Pixel* buffs[],
                    int x1,
                    int y1,
                    int x2,
                    int y2,
                    const std::vector<Pixel>& colors,
                    const uint8_t thickness)
{
  drawLine(n, buffs, x1, y1, x2, y2, colors, intBuffIntensity, allowOverexposed, thickness,
           screenWidth, screenHeight);
}

} // namespace goom
