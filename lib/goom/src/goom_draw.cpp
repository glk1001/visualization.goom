#include "goom_draw.h"

#include "draw_methods.h"
#include "goom_config.h"
#include "goom_graphic.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace goom
{

GoomDraw::GoomDraw() : screenWidth{0}, screenHeight{0}
{
}

GoomDraw::GoomDraw(const uint32_t screenW, const uint32_t screenH)
  : screenWidth{screenW}, screenHeight{screenH}
{
  setBuffIntensity(0.5);
  setAllowOverexposed(true);
}

bool GoomDraw::operator==(const GoomDraw& d) const
{
  return screenWidth == d.screenWidth && screenHeight == d.screenHeight &&
         allowOverexposed == d.allowOverexposed && buffIntensity == d.buffIntensity &&
         intBuffIntensity == d.intBuffIntensity;
}

void GoomDraw::circle(Pixel* buff, const int x0, const int y0, const int radius, const Pixel& color)
{
  drawCircle(buff, x0, y0, radius, color, intBuffIntensity, allowOverexposed, screenWidth,
             screenHeight);
}
void GoomDraw::circle(std::vector<Pixel*> buffs,
                      const int x0,
                      const int y0,
                      const int radius,
                      const std::vector<Pixel>& colors)
{
  drawCircle(buffs, x0, y0, radius, colors, intBuffIntensity, allowOverexposed, screenWidth,
             screenHeight);
}

void GoomDraw::filledCircle(
    Pixel* buff, const int x0, const int y0, const int radius, const std::vector<Pixel>& colors)
{
  drawFilledCircle(buff, x0, y0, radius, colors, intBuffIntensity, allowOverexposed, screenWidth,
                   screenHeight);
}

void GoomDraw::filledCircle(std::vector<Pixel*> buffs,
                            const int x0,
                            const int y0,
                            const int radius,
                            const std::vector<std::vector<Pixel>>& colorSets)
{
  drawFilledCircle(buffs, x0, y0, radius, colorSets, intBuffIntensity, allowOverexposed,
                   screenWidth, screenHeight);
}

void GoomDraw::line(Pixel* buff,
                    const int x1,
                    const int y1,
                    const int x2,
                    const int y2,
                    const Pixel& color,
                    const uint8_t thickness)
{
  drawLine(buff, x1, y1, x2, y2, color, intBuffIntensity, thickness, allowOverexposed, screenWidth,
           screenHeight);
}

void GoomDraw::line(std::vector<Pixel*> buffs,
                    int x1,
                    int y1,
                    int x2,
                    int y2,
                    const std::vector<Pixel>& colors,
                    const uint8_t thickness)
{
  drawLine(buffs, x1, y1, x2, y2, colors, intBuffIntensity, allowOverexposed, thickness,
           screenWidth, screenHeight);
}

void GoomDraw::setPixelRGB(Pixel* buff, const uint32_t x, const uint32_t y, const Pixel& color)
{
  std::vector<Pixel*> buffs{buff};
  std::vector<Pixel> colors{color};
  const int pos = static_cast<int>(x + (y * screenWidth));
  drawPixels(buffs, pos, colors, intBuffIntensity, allowOverexposed);
}

void GoomDraw::setPixelRGB(std::vector<Pixel*>& buffs,
                           const uint32_t x,
                           const uint32_t y,
                           const std::vector<Pixel>& colors)
{
  const int pos = static_cast<int>(x + (y * screenWidth));
  drawPixels(buffs, pos, colors, intBuffIntensity, allowOverexposed);
}

} // namespace goom
