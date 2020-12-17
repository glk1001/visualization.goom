#include "goom_draw.h"

#include "draw_methods.h"
#include "gfontlib.h"
#include "goom_graphic.h"

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

void GoomDraw::circle(
    PixelBuffer& buff, const int x0, const int y0, const int radius, const Pixel& color) const
{
  drawCircle(buff, x0, y0, radius, color, intBuffIntensity, allowOverexposed, screenWidth,
             screenHeight);
}
void GoomDraw::circle(std::vector<PixelBuffer*>& buffs,
                      const int x0,
                      const int y0,
                      const int radius,
                      const std::vector<Pixel>& colors) const
{
  drawCircle(buffs, x0, y0, radius, colors, intBuffIntensity, allowOverexposed, screenWidth,
             screenHeight);
}

void GoomDraw::filledCircle(PixelBuffer& buff,
                            const int x0,
                            const int y0,
                            const int radius,
                            const std::vector<Pixel>& colors) const
{
  drawFilledCircle(buff, x0, y0, radius, colors, intBuffIntensity, allowOverexposed, screenWidth,
                   screenHeight);
}

void GoomDraw::filledCircle(std::vector<PixelBuffer*>& buffs,
                            const int x0,
                            const int y0,
                            const int radius,
                            const std::vector<std::vector<Pixel>>& colorSets) const
{
  drawFilledCircle(buffs, x0, y0, radius, colorSets, intBuffIntensity, allowOverexposed,
                   screenWidth, screenHeight);
}

void GoomDraw::line(PixelBuffer& buff,
                    const int x1,
                    const int y1,
                    const int x2,
                    const int y2,
                    const Pixel& color,
                    const uint8_t thickness) const
{
  drawLine(buff, x1, y1, x2, y2, color, intBuffIntensity, thickness, allowOverexposed, screenWidth,
           screenHeight);
}

void GoomDraw::line(std::vector<PixelBuffer*>& buffs,
                    int x1,
                    int y1,
                    int x2,
                    int y2,
                    const std::vector<Pixel>& colors,
                    const uint8_t thickness) const
{
  drawLine(buffs, x1, y1, x2, y2, colors, intBuffIntensity, allowOverexposed, thickness,
           screenWidth, screenHeight);
}

void GoomDraw::text(PixelBuffer& buff,
                    const int x,
                    const int y,
                    const std::string& text,
                    const float charSpace,
                    const bool center)
{
  if (!fontsLoaded)
  {
    gfont_load();
    fontsLoaded = true;
  }
  goom_draw_text(buff, screenWidth, screenHeight, x, y, text.c_str(), charSpace, center);
}

void GoomDraw::setPixelRGB(PixelBuffer& buff,
                           const uint32_t x,
                           const uint32_t y,
                           const Pixel& color) const
{
  std::vector<PixelBuffer*> buffs{&buff};
  std::vector<Pixel> colors{color};
  const auto pos = static_cast<int>(x + (y * screenWidth));
  drawPixels(buffs, pos, colors, intBuffIntensity, allowOverexposed);
}

void GoomDraw::setPixelRGB(std::vector<PixelBuffer*>& buffs,
                           const uint32_t x,
                           const uint32_t y,
                           const std::vector<Pixel>& colors) const
{
  const auto pos = static_cast<int>(x + (y * screenWidth));
  drawPixels(buffs, pos, colors, intBuffIntensity, allowOverexposed);
}

} // namespace goom
