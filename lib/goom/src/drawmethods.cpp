#include "drawmethods.h"

#include "colorutils.h"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace goom
{

// Bresenhams midpoint circle algorithm from
//   "https://rosettacode.org/wiki/Bitmap/Midpoint_circle_algorithm".
//
void circle(Pixel* buff,
            const int x0,
            const int y0,
            const int radius,
            const uint32_t color,
            const uint32_t screenWidth,
            const uint32_t screenHeight)
{
  const Pixel pixColor{.val = color};
  const auto plot = [&](const int x, const int y) -> void {
    if (uint32_t(x) >= screenWidth || uint32_t(y) >= screenHeight)
    {
      return;
    }
    const int pos = y * static_cast<int>(screenWidth) + x;
    Pixel* const p = &(buff[pos]);
    *p = getColorAdd(*p, getHalfIntensityColor(pixColor));
  };

  int f = 1 - radius;
  int ddF_x = 0;
  int ddF_y = -2 * radius;
  int x = 0;
  int y = radius;

  plot(x0, y0 + radius);
  plot(x0, y0 - radius);
  plot(x0 + radius, y0);
  plot(x0 - radius, y0);

  while (x < y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x + 1;
    plot(x0 + x, y0 + y);
    plot(x0 - x, y0 + y);
    plot(x0 + x, y0 - y);
    plot(x0 - x, y0 - y);
    plot(x0 + y, y0 + x);
    plot(x0 - y, y0 + x);
    plot(x0 + y, y0 - x);
    plot(x0 - y, y0 - x);
  }
}

void filledCircle(Pixel* buff,
                  const int x0,
                  const int y0,
                  const int radius,
                  const std::vector<uint32_t> colors,
                  const uint32_t screenWidth,
                  const uint32_t screenHeight)
{
  for (int i = 1; i <= radius; i++)
  {
    circle(buff, x0, y0, i, colors.at(static_cast<size_t>(i - 1)), screenWidth, screenHeight);
  }
}

static void draw_wuline(const size_t n,
                        Pixel* buffs[],
                        const std::vector<Pixel>& colors,
                        const int x1,
                        const int y1,
                        const int x2,
                        const int y2,
                        const uint32_t screenx,
                        const uint32_t screeny);

void draw_line(Pixel* buff,
               const int x1,
               const int y1,
               const int x2,
               const int y2,
               const uint32_t color,
               const uint32_t screenx,
               const uint32_t screeny)
{
  Pixel* buffs[] = {buff};
  std::vector<Pixel> colors(1);
  colors[0].val = color;
  draw_line(1, buffs, colors, x1, y1, x2, y2, screenx, screeny);
}

void draw_line(const size_t n,
               Pixel* buffs[],
               const std::vector<Pixel>& colors,
               int x1,
               int y1,
               int x2,
               int y2,
               const uint32_t screenx,
               const uint32_t screeny)
{
  draw_wuline(n, buffs, colors, x1, y1, x2, y2, screenx, screeny);
}

using PlotFunc = const std::function<void(int x, int y, float brightess)>;
static void WuDrawLine(float x0, float y0, float x1, float y1, const PlotFunc&);

inline void draw_pixels(const size_t n,
                        Pixel* buffs[],
                        const std::vector<Pixel>& colors,
                        const int pos)
{
  for (size_t i = 0; i < n; i++)
  {
    Pixel* const p = &(buffs[i][pos]);
    *p = getColorAdd(*p, getHalfIntensityColor(colors[i]));
  }
}

static void draw_wuline(const size_t n,
                        Pixel* buffs[],
                        const std::vector<Pixel>& colors,
                        const int x1,
                        const int y1,
                        const int x2,
                        const int y2,
                        const uint32_t screenx,
                        const uint32_t screeny)
{
  if ((y1 < 0) || (y2 < 0) || (x1 < 0) || (x2 < 0) || (y1 >= static_cast<int>(screeny)) ||
      (y2 >= static_cast<int>(screeny)) || (x1 >= static_cast<int>(screenx)) ||
      (x2 >= static_cast<int>(screenx)))
  {
    return;
  }

  assert(n == colors.size());
  std::vector<Pixel> tempColors = colors;
  auto plot = [&](const int x, const int y, const float brightness) -> void {
    if (uint16_t(x) >= screenx || uint16_t(y) >= screeny)
    {
      return;
    }
    if (brightness < 0.001)
    {
      return;
    }
    const int pos = y * static_cast<int>(screenx) + x;
    if (brightness >= 0.999)
    {
      draw_pixels(n, buffs, colors, pos);
    }
    else
    {
      for (size_t i = 0; i < colors.size(); i++)
      {
        tempColors[i] = getBrighterColor(brightness, colors[i]);
      }
      draw_pixels(n, buffs, tempColors, pos);
    }
  };

  WuDrawLine(x1, y1, x2, y2, plot);
}

// The Xiaolin Wu anti-aliased draw line.
// From https://rosettacode.org/wiki/Xiaolin_Wu%27s_line_algorithm#C.2B.2B
//
static void WuDrawLine(float x0, float y0, float x1, float y1, const PlotFunc& plot)
{
  const auto ipart = [](const float x) -> int { return static_cast<int>(std::floor(x)); };
  const auto round = [](const float x) -> float { return std::round(x); };
  const auto fpart = [](const float x) -> float { return x - std::floor(x); };
  const auto rfpart = [=](const float x) -> float { return 1 - fpart(x); };

  const bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep)
  {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }
  if (x0 > x1)
  {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  const float dx = x1 - x0; // because of above swap, must be >= 0
  const float dy = y1 - y0;
  const float gradient = (dx < 0.001) ? 1 : dy / dx;

  int xpx11;
  float intery;
  {
    const float xend = round(x0);
    const float yend = y0 + gradient * (xend - x0);
    const float xgap = rfpart(x0 + 0.5);
    xpx11 = static_cast<int>(xend);
    const int ypx11 = ipart(yend);
    if (steep)
    {
      plot(ypx11, xpx11, rfpart(yend) * xgap);
      plot(ypx11 + 1, xpx11, fpart(yend) * xgap);
    }
    else
    {
      plot(xpx11, ypx11, rfpart(yend) * xgap);
      plot(xpx11, ypx11 + 1, fpart(yend) * xgap);
    }
    intery = yend + gradient;
  }

  int xpx12;
  {
    const float xend = round(x1);
    const float yend = y1 + gradient * (xend - x1);
    const float xgap = rfpart(x1 + 0.5);
    xpx12 = static_cast<int>(xend);
    const int ypx12 = ipart(yend);
    if (steep)
    {
      plot(ypx12, xpx12, rfpart(yend) * xgap);
      plot(ypx12 + 1, xpx12, fpart(yend) * xgap);
    }
    else
    {
      plot(xpx12, ypx12, rfpart(yend) * xgap);
      plot(xpx12, ypx12 + 1, fpart(yend) * xgap);
    }
  }

  if (steep)
  {
    for (int x = xpx11 + 1; x < xpx12; x++)
    {
      plot(ipart(intery), x, rfpart(intery));
      plot(ipart(intery) + 1, x, fpart(intery));
      intery += gradient;
    }
  }
  else
  {
    for (int x = xpx11 + 1; x < xpx12; x++)
    {
      plot(x, ipart(intery), rfpart(intery));
      plot(x, ipart(intery) + 1, fpart(intery));
      intery += gradient;
    }
  }
}

} // namespace goom
