#include "goom_draw.h"

#include "draw_methods.h"
#include "goom_graphic.h"

#include <cstdint>
#include <vector>

namespace goom
{

GoomDraw::GoomDraw() : m_screenWidth{0}, m_screenHeight{0}
{
}

GoomDraw::GoomDraw(const uint32_t screenW, const uint32_t screenH)
  : m_screenWidth{screenW}, m_screenHeight{screenH}
{
  SetBuffIntensity(0.5);
  SetAllowOverexposed(true);
}

auto GoomDraw::operator==(const GoomDraw& d) const -> bool
{
  return m_screenWidth == d.m_screenWidth && m_screenHeight == d.m_screenHeight &&
         m_allowOverexposed == d.m_allowOverexposed && m_buffIntensity == d.m_buffIntensity &&
         m_intBuffIntensity == d.m_intBuffIntensity;
}

void GoomDraw::Circle(
    PixelBuffer& buff, const int x0, const int y0, const int radius, const Pixel& color) const
{
  DrawCircle(buff, x0, y0, radius, color, m_intBuffIntensity, m_allowOverexposed, m_screenWidth,
             m_screenHeight);
}
void GoomDraw::Circle(std::vector<PixelBuffer*>& buffs,
                      const int x0,
                      const int y0,
                      const int radius,
                      const std::vector<Pixel>& colors) const
{
  DrawCircle(buffs, x0, y0, radius, colors, m_intBuffIntensity, m_allowOverexposed, m_screenWidth,
             m_screenHeight);
}

void GoomDraw::FilledCircle(
    PixelBuffer& buff, const int x0, const int y0, const int radius, const Pixel& color) const
{
  DrawFilledCircle(buff, x0, y0, radius, color, m_intBuffIntensity, m_allowOverexposed,
                   m_screenWidth, m_screenHeight);
}

void GoomDraw::FilledCircle(std::vector<PixelBuffer*>& buffs,
                            const int x0,
                            const int y0,
                            const int radius,
                            const std::vector<Pixel>& colors) const
{
  DrawFilledCircle(buffs, x0, y0, radius, colors, m_intBuffIntensity, m_allowOverexposed,
                   m_screenWidth, m_screenHeight);
}

void GoomDraw::Line(PixelBuffer& buff,
                    const int x1,
                    const int y1,
                    const int x2,
                    const int y2,
                    const Pixel& color,
                    const uint8_t thickness) const
{
  DrawLine(buff, x1, y1, x2, y2, color, m_intBuffIntensity, thickness, m_allowOverexposed,
           m_screenWidth, m_screenHeight);
}

void GoomDraw::Line(std::vector<PixelBuffer*>& buffs,
                    int x1,
                    int y1,
                    int x2,
                    int y2,
                    const std::vector<Pixel>& colors,
                    const uint8_t thickness) const
{
  DrawLine(buffs, x1, y1, x2, y2, colors, m_intBuffIntensity, m_allowOverexposed, thickness,
           m_screenWidth, m_screenHeight);
}

void GoomDraw::SetPixelRgb(PixelBuffer& buff,
                           const uint32_t x,
                           const uint32_t y,
                           const Pixel& color) const
{
  std::vector<PixelBuffer*> buffs{&buff};
  std::vector<Pixel> colors{color};
  const auto pos = static_cast<int>(x + (y * m_screenWidth));
  DrawPixels(buffs, pos, colors, m_intBuffIntensity, m_allowOverexposed);
}

void GoomDraw::SetPixelRgb(std::vector<PixelBuffer*>& buffs,
                           const uint32_t x,
                           const uint32_t y,
                           const std::vector<Pixel>& colors) const
{
  const auto pos = static_cast<int>(x + (y * m_screenWidth));
  DrawPixels(buffs, pos, colors, m_intBuffIntensity, m_allowOverexposed);
}

} // namespace goom
