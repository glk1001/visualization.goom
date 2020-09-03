#include "drawmethods.h"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

static void draw_wuline(const size_t n,
                        Pixel* buffs[],
                        const std::vector<Pixel>& cols,
                        const int x1,
                        const int y1,
                        const int x2,
                        const int y2,
                        const uint16_t screenx,
                        const uint16_t screeny);

void draw_line(Pixel* data,
               const int x1,
               const int y1,
               const int x2,
               const int y2,
               const uint32_t col,
               const uint16_t screenx,
               const uint16_t screeny)
{
  Pixel* buffs[] = {data};
  std::vector<Pixel> colors(1);
  colors[0].val = col;
  draw_line(1, buffs, colors, x1, y1, x2, y2, screenx, screeny);
}

void draw_line(const size_t n,
               Pixel* buffs[],
               const std::vector<Pixel>& cols,
               int x1,
               int y1,
               int x2,
               int y2,
               const uint16_t screenx,
               const uint16_t screeny)
{
  draw_wuline(n, buffs, cols, x1, y1, x2, y2, screenx, screeny);
}

static void WuDrawLine(float x0,
                       float y0,
                       float x1,
                       float y1,
                       const std::function<void(int x, int y, float brightess)>& plot)
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

inline uint8_t brighten(const uint32_t br, const unsigned char c)
{
  return static_cast<uint8_t>((br * uint32_t(c)) >> 8);
}

inline Pixel getColor(const float brightness, const Pixel& color)
{
  assert(brightness >= 0.0 && brightness <= 1.0);
  const uint32_t br = uint32_t(brightness * 255);
  Pixel c;
  c.channels.r = brighten(br, color.channels.r);
  c.channels.g = brighten(br, color.channels.g);
  c.channels.b = brighten(br, color.channels.b);
  c.channels.a = brighten(br, color.channels.a);
  return c;
}

inline uint8_t colorAdd(const unsigned char c1, const unsigned char c2)
{
  uint32_t cadd = uint32_t(c1) + (uint32_t(c2) >> 1);
  if (cadd > 255)
  {
    cadd = 255;
  }
  return static_cast<uint8_t>(cadd);
}

inline Pixel getColorAdd(const Pixel& color1, const Pixel& color2)
{
  Pixel c;
  c.channels.r = colorAdd(color1.channels.r, color2.channels.r);
  c.channels.g = colorAdd(color1.channels.g, color2.channels.g);
  c.channels.b = colorAdd(color1.channels.b, color2.channels.b);
  c.channels.a = colorAdd(color1.channels.a, color2.channels.a);
  return c;
}

inline void draw_pixels(const size_t n,
                        Pixel* buffs[],
                        const std::vector<Pixel>& cols,
                        const int pos)
{
  for (size_t i = 0; i < n; i++)
  {
    Pixel* const p = &(buffs[i][pos]);
    *p = getColorAdd(*p, cols[i]);
  }
}

static void draw_wuline(const size_t n,
                        Pixel* buffs[],
                        const std::vector<Pixel>& cols,
                        const int x1,
                        const int y1,
                        const int x2,
                        const int y2,
                        const uint16_t screenx,
                        const uint16_t screeny)
{
  if ((y1 < 0) || (y2 < 0) || (x1 < 0) || (x2 < 0) || (uint16_t(y1) >= screeny) ||
      (uint16_t(y2) >= screeny) || (uint16_t(x1) >= screenx) || (uint16_t(x2) >= screenx))
  {
    return;
  }

  assert(n == cols.size());
  const std::vector<Pixel> colors = cols;
  auto plot = [&](int x, int y, float brightness) -> void {
    if (uint16_t(x) >= screenx || uint16_t(y) >= screeny)
    {
      return;
    }
    if (brightness < 0.001)
    {
      return;
    }
    const int pos = y * int(screenx) + x;
    if (brightness >= 0.999)
    {
      draw_pixels(n, buffs, cols, pos);
    }
    else
    {
      for (auto c : colors)
      {
        c = getColor(brightness, c);
      }
      draw_pixels(n, buffs, colors, pos);
    }
  };

  WuDrawLine(x1, y1, x2, y2, plot);
}
