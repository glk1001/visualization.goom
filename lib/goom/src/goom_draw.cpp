#include "goom_draw.h"

#include "draw_methods.h"
#include "goom_graphic.h"
#include "goomutils/colormaps.h"
#include "goomutils/colorutils.h"
#include "goomutils/parallel_utils.h"

#include <cassert>
#include <cstdint>
#include <vector>

namespace GOOM
{

using namespace UTILS;

GoomDraw::GoomDraw() : m_screenWidth{0}, m_screenHeight{0}
{
}

GoomDraw::GoomDraw(const uint32_t screenW, const uint32_t screenH)
  : m_screenWidth{screenW}, m_screenHeight{screenH}, m_parallel{-1} // max cores - 1
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

void GoomDraw::Bitmap(PixelBuffer& buff,
                      int xCentre,
                      int yCentre,
                      const PixelBuffer& bitmap,
                      const GetBitmapColorFunc& getColor)
{
  const auto bitmapWidth = static_cast<int>(bitmap.GetWidth());
  const auto bitmapHeight = static_cast<int>(bitmap.GetHeight());

  const int x0 = std::clamp(xCentre - bitmapWidth / 2, 0, static_cast<int>(m_screenWidth) - 1);
  const int y0 = std::clamp(yCentre - bitmapHeight / 2, 0, static_cast<int>(m_screenHeight) - 1);
  const int x1 = std::min(x0 + bitmapWidth - 1, static_cast<int>(m_screenWidth) - 1);
  const int y1 = std::min(y0 + bitmapHeight - 1, static_cast<int>(m_screenHeight) - 1);

  const auto setDestPixelRow = [&](const uint32_t y) {
    const auto yBitmap = static_cast<size_t>(y);
    size_t xBitmap = 0;
    for (int x = x0; x <= x1; ++x)
    {
      const Pixel finalColor = getColor(xBitmap, yBitmap, bitmap(xBitmap, yBitmap));
      DrawPixel(&buff, x, static_cast<int>(y) + y0, finalColor, m_intBuffIntensity,
                m_allowOverexposed);
      xBitmap++;
    }
  };

  if (bitmapWidth > 199)
  {
    m_parallel.ForLoop(static_cast<uint32_t>(y1 - y0 + 1), setDestPixelRow);
  }
  else
  {
    for (uint32_t y = 0; y <= static_cast<uint32_t>(y1 - y0); ++y)
    {
      setDestPixelRow(y);
    }
  }
}

void GoomDraw::Bitmap(std::vector<PixelBuffer*>& buffs,
                      int xCentre,
                      int yCentre,
                      const std::vector<const PixelBuffer*>& bitmaps,
                      const std::vector<GetBitmapColorFunc>& getColors)
{
  for (size_t i = 0; i < buffs.size(); i++)
  {
    Bitmap(*(buffs[i]), xCentre, yCentre, (*bitmaps[i]), getColors[i]);
  }
}

void GoomDraw::Line(PixelBuffer& buff,
                    const int x1,
                    const int y1,
                    const int x2,
                    const int y2,
                    const Pixel& color,
                    const uint8_t thickness) const
{
  DrawLine(buff, x1, y1, x2, y2, color, m_intBuffIntensity, m_allowOverexposed, thickness,
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
  DrawPixels(buffs, static_cast<int>(x), static_cast<int>(y), colors, m_intBuffIntensity,
             m_allowOverexposed);
}

void GoomDraw::SetPixelRgb(std::vector<PixelBuffer*>& buffs,
                           const uint32_t x,
                           const uint32_t y,
                           const std::vector<Pixel>& colors) const
{
  DrawPixels(buffs, static_cast<int>(x), static_cast<int>(y), colors, m_intBuffIntensity,
             m_allowOverexposed);
}

} // namespace GOOM
