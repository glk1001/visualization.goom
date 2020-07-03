#include "drawmethods.h"

#include <array>
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <functional>
#include <algorithm>
#include <utility>
#include <vector>

static void WuDrawLine(float x0, float y0, float x1, float y1,
                       const std::function<void(int x, int y, float brightess)>& plot)
{
  auto ipart = [](float x) -> int {return int(std::floor(x));};
  auto round = [](float x) -> float {return std::round(x);};
  auto fpart = [](float x) -> float {return x - std::floor(x);};
  auto rfpart = [=](float x) -> float {return 1 - fpart(x);};

  const bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    std::swap(x0,y0);
    std::swap(x1,y1);
  }
  if (x0 > x1) {
    std::swap(x0,x1);
    std::swap(y0,y1);
  }

  const float dx = x1 - x0;
  const float dy = y1 - y0;
  const float gradient = (dx == 0) ? 1 : dy/dx;

  int xpx11;
  float intery;
  {
    const float xend = round(x0);
    const float yend = y0 + gradient * (xend - x0);
    const float xgap = rfpart(x0 + 0.5);
    xpx11 = int(xend);
    const int ypx11 = ipart(yend);
    if (steep) {
      plot(ypx11,     xpx11, rfpart(yend) * xgap);
      plot(ypx11 + 1, xpx11,  fpart(yend) * xgap);
    } else {
      plot(xpx11, ypx11,    rfpart(yend) * xgap);
      plot(xpx11, ypx11 + 1, fpart(yend) * xgap);
    }
    intery = yend + gradient;
  }

  int xpx12;
  {
    const float xend = round(x1);
    const float yend = y1 + gradient * (xend - x1);
    const float xgap = rfpart(x1 + 0.5);
    xpx12 = int(xend);
    const int ypx12 = ipart(yend);
    if (steep) {
        plot(ypx12,     xpx12, rfpart(yend) * xgap);
        plot(ypx12 + 1, xpx12,  fpart(yend) * xgap);
    } else {
        plot(xpx12, ypx12,    rfpart(yend) * xgap);
        plot(xpx12, ypx12 + 1, fpart(yend) * xgap);
    }
  }

  if (steep) {
    for (int x = xpx11 + 1; x < xpx12; x++) {
        plot(ipart(intery),     x, rfpart(intery));
        plot(ipart(intery) + 1, x,  fpart(intery));
        intery += gradient;
    }
  } else {
    for (int x = xpx11 + 1; x < xpx12; x++) {
        plot(x, ipart(intery),     rfpart(intery));
        plot(x, ipart(intery) + 1,  fpart(intery));
        intery += gradient;
    }
  }
}

static inline void draw_pixel(Pixel* frontBuff, Pixel* backBuff, uint32_t col)
{
  int tra = 0;
  unsigned char* bra = (unsigned char*)backBuff;
  unsigned char* dra = (unsigned char*)frontBuff;
  unsigned char* cra = (unsigned char*)&col;
  for (int i=0; i < 4; i++) {
    tra = *cra;
    tra += *bra;
    if (tra > 255) {
      tra = 255;
    }
    *dra = tra;
    ++dra;
    ++cra;
    ++bra;
  }                                                                                              \
}

inline uint32_t getColor(const float brightness, uint32_t color)
{
  assert(brightness >= 0.0 && brightness <= 1.0);
  if (brightness >= 0.999) {
    return color;
  }
  std::array<unsigned char, 4> arrayOfBytes;
  for (int i=0; i < 4; i++) {
//    arrayOfBytes[3-i] = color >> (i*8);
    arrayOfBytes[i] = color >> (i*8);
  }
  const uint32_t br = uint32_t(brightness*255);
  arrayOfBytes[ALPHA] = (unsigned char)((br * uint32_t(arrayOfBytes[ALPHA])) >> 8);
  arrayOfBytes[BLEU] = (unsigned char)((br * uint32_t(arrayOfBytes[BLEU])) >> 8);
  arrayOfBytes[VERT] = (unsigned char)((br * uint32_t(arrayOfBytes[VERT])) >> 8);
  arrayOfBytes[ROUGE] = (unsigned char)((br * uint32_t(arrayOfBytes[ROUGE])) >> 8);
  return uint32_t(
           (unsigned char)(arrayOfBytes[0]) << 24 |
           (unsigned char)(arrayOfBytes[1]) << 16 |
           (unsigned char)(arrayOfBytes[2]) <<  8 |
           (unsigned char)(arrayOfBytes[3])
         );
}

inline void modColors(const float brightness, const size_t n, std::vector<uint32_t>& colors)
{
  for (size_t i = 0; i < n; i++) {
    colors[i] = getColor(brightness, colors[i]);
  }
}

inline void draw_pixels(size_t n, Pixel* buffs[], const std::vector<uint32_t>& cols, const int pos)
{
  for (size_t i = 0; i < n ; i++) {
    Pixel* const p = &(buffs[i][pos]);
    draw_pixel(p, p, cols[i]);
  }
}

static void draw_wuline(size_t n, Pixel* buffs[], const std::vector<uint32_t>& cols,
                        int x1, int y1, int x2, int y2, int screenx, int screeny)
{
  if ((y1 < 0) || (y2 < 0) || (x1 < 0) || (x2 < 0) || (y1 >= screeny) || (y2 >= screeny) ||
      (x1 >= screenx) || (x2 >= screenx)) {
    return;
  }

  assert(n == cols.size());
  std::vector<uint32_t> colors = cols;
  auto plot = [&](int x, int y, float brightness) -> void {
    if (x >= screenx || y >= screeny) {
      return;
    }
    const int pos = y*screenx + x;
    if (brightness >= 0.999) {
      draw_pixels(n, buffs, cols, pos);
    } else {
      for (auto c : colors) { c = getColor(brightness, c); }
      draw_pixels(n, buffs, colors, pos);
    }
  };

  WuDrawLine(x1, y1, x2, y2, plot);
}

void draw_line(Pixel* data, int x1, int y1, int x2, int y2, uint32_t col, int screenx, int screeny)
{
  Pixel* buffs[] = { data };
  const std::vector<uint32_t> colors = { col };
  draw_line(1, buffs, colors, x1, y1, x2, y2, screenx, screeny);
}

void draw_line(size_t n, Pixel* buffs[], const std::vector<uint32_t>& cols,
               int x1, int y1, int x2, int y2, int screenx, int screeny)
{
  draw_wuline(n, buffs, cols, x1, y1, x2, y2, screenx, screeny);
  return;

  if ((y1 < 0) || (y2 < 0) || (x1 < 0) || (x2 < 0) || (y1 >= screeny) || (y2 >= screeny) ||
      (x1 >= screenx) || (x2 >= screenx)) {
    return;
  }

  /* clip to top edge
     if ((y1 < 0) && (y2 < 0))
     return;
	
     if (y1 < 0) {
     x1 += (y1 * (x1 - x2)) / (y2 - y1);
     y1 = 0;
     }
     if (y2 < 0) {
     x2 += (y2 * (x1 - x2)) / (y2 - y1);
     y2 = 0;
     }
         
     clip to bottom edge 
     if ((y1 >= screeny) && (y2 >= screeny))
     return;
     if (y1 >= screeny) {
     x1 -= ((screeny - y1) * (x1 - x2)) / (y2 - y1);
     y1 = screeny - 1;
     }
     if (y2 >= screeny) {
     x2 -= ((screeny - y2) * (x1 - x2)) / (y2 - y1);
     y2 = screeny - 1;
     }
     clip to left edge 
     if ((x1 < 0) && (x2 < 0))
     return;
     if (x1 < 0) {
     y1 += (x1 * (y1 - y2)) / (x2 - x1);
     x1 = 0;
     }
     if (x2 < 0) {
     y2 += (x2 * (y1 - y2)) / (x2 - x1);
     x2 = 0;
     }
     clip to right edge 
     if ((x1 >= screenx) && (x2 >= screenx))
     return;
     if (x1 >= screenx) {
     y1 -= ((screenx - x1) * (y1 - y2)) / (x2 - x1);
     x1 = screenx - 1;
     }
     if (x2 >= screenx) {
     y2 -= ((screenx - x2) * (y1 - y2)) / (x2 - x1);
     x2 = screenx - 1;
     }
  */

  int dx = x2 - x1;
  int dy = y2 - y1;
  if (x1 > x2) {
    int tmp = x1;
    x1 = x2;
    x2 = tmp;
    tmp = y1;
    y1 = y2;
    y2 = tmp;
    dx = x2 - x1;
    dy = y2 - y1;
  }

  /* vertical line */
  if (dx == 0) {
    if (y1 < y2) {
      int i = (screenx * y1) + x1;
      for (int y = y1; y <= y2; y++) {
        draw_pixels(n, buffs, cols, i);
        i += screenx;
      }
    } else {
      int i = (screenx * y2) + x1;
      for (int y = y2; y <= y1; y++) {
        draw_pixels(n, buffs, cols, i);
        i += screenx;
      }
    }
    return;
  }
  /* horizontal line */
  if (dy == 0) {
    if (x1 < x2) {
      int i = (screenx * y1) + x1;
      for (int x = x1; x <= x2; x++) {
        draw_pixels(n, buffs, cols, i);
        i++;
      }
      return;
    } else {
      int i = (screenx * y1) + x2;
      for (int x = x2; x <= x1; x++) {
        draw_pixels(n, buffs, cols, i);
        i++;
      }
      return;
    }
  }
  /* 1    */
  /* \   */
  /* \  */
  /* 2 */
  if (y2 > y1) {
    /* steep */
    if (dy > dx) {
      dx = ((dx << 16) / dy);
      int x = x1 << 16;
      for (int y = y1; y <= y2; y++) {
        const int xx = x >> 16;
        int i = (screenx * y) + xx;
        draw_pixels(n, buffs, cols, i);
        if (xx < (screenx - 1)) {
          i++;
          /* DRAWMETHOD; */
        }
        x += dx;
      }
      return;
    }
    /* shallow */
    else {
      dy = ((dy << 16) / dx);
      int y = y1 << 16;
      for (int x = x1; x <= x2; x++) {
        const int yy = y >> 16;
        int i = (screenx * yy) + x;
        draw_pixels(n, buffs, cols, i);
        if (yy < (screeny - 1)) {
          i += screeny;
          /* DRAWMETHOD; */
        }
        y += dy;
      }
    }
  }
  /* 2 */
  /* /  */
  /* /   */
  /* 1    */
  else {
    /* steep */
    if (-dy > dx) {
      dx = ((dx << 16) / -dy);
      int x = (x1 + 1) << 16;
      for (int y = y1; y >= y2; y--) {
        const int xx = x >> 16;
        int i = (screenx * y) + xx;
        draw_pixels(n, buffs, cols, i);
        if (xx < (screenx - 1)) {
          i--;
          /* DRAWMETHOD; */
        }
        x += dx;
      }
      return;
    }
    /* shallow */
    else {
      dy = ((dy << 16) / dx);
      int y = y1 << 16;
      for (int x = x1; x <= x2; x++) {
        const int yy = y >> 16;
        int i = (screenx * yy) + x;
        draw_pixels(n, buffs, cols, i);
        if (yy < (screeny - 1)) {
          i += screeny;
          /* DRAWMETHOD; */
        }
        y += dy;
      }
      return;
    }
  }
}
