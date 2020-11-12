#include "draw_methods.h"

#include "goom_graphic.h"
#include "goomutils/colorutils.h"

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
void drawCircle(Pixel* buff,
                const int x0,
                const int y0,
                const int radius,
                const Pixel& color,
                const uint32_t buffIntensity,
                const bool allowOverexposed,
                const uint32_t screenWidth,
                const uint32_t screenHeight)
{
  std::vector<Pixel*> buffs{buff};
  const std::vector<Pixel> colors{color};
  drawCircle(buffs, x0, y0, radius, colors, buffIntensity, allowOverexposed, screenWidth,
             screenHeight);
}

void drawCircle(std::vector<Pixel*>& buffs,
                const int x0,
                const int y0,
                const int radius,
                const std::vector<Pixel>& colors,
                const uint32_t buffIntensity,
                const bool allowOverexposed,
                const uint32_t screenWidth,
                const uint32_t screenHeight)
{
  assert(buffs.size() == colors.size());

  auto plot = [&](const int x, const int y) -> void {
    if (uint32_t(x) >= screenWidth || uint32_t(y) >= screenHeight)
    {
      return;
    }
    const int pos = y * static_cast<int>(screenWidth) + x;
    drawPixels(buffs, pos, colors, buffIntensity, allowOverexposed);
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

void drawFilledCircle(Pixel* buff,
                      const int x0,
                      const int y0,
                      const int radius,
                      const std::vector<Pixel>& colors,
                      const uint32_t buffIntensity,
                      const bool allowOverexposed,
                      const uint32_t screenWidth,
                      const uint32_t screenHeight)
{
  for (int i = 1; i <= radius; i++)
  {
    drawCircle(buff, x0, y0, i, colors.at(static_cast<size_t>(i - 1)), buffIntensity,
               allowOverexposed, screenWidth, screenHeight);
  }
}

void drawFilledCircle(std::vector<Pixel*>& buffs,
                      const int x0,
                      const int y0,
                      const int radius,
                      const std::vector<std::vector<Pixel>>& colorSets,
                      const uint32_t buffIntensity,
                      const bool allowOverexposed,
                      const uint32_t screenWidth,
                      const uint32_t screenHeight)
{
  for (size_t i = 0; i < buffs.size(); i++)
  {
    drawFilledCircle(buffs[i], x0, y0, radius, colorSets.at(i), buffIntensity, allowOverexposed,
                     screenWidth, screenHeight);
  }
}

static void drawWuLine(std::vector<Pixel*>& buffs,
                       const int x1,
                       const int y1,
                       const int x2,
                       const int y2,
                       const std::vector<Pixel>& colors,
                       const uint32_t buffIntensity,
                       const bool allowOverexposed,
                       const uint32_t screenx,
                       const uint32_t screeny);

constexpr int LINE_THICKNESS_MIDDLE = 0;
constexpr int LINE_THICKNESS_DRAW_CLOCKWISE = 1;
constexpr int LINE_THICKNESS_DRAW_COUNTERCLOCKWISE = 2;

static void drawThickLine(std::vector<Pixel*>& buffs,
                          int x0,
                          int y0,
                          int x1,
                          int y1,
                          const std::vector<Pixel>& colors,
                          const uint32_t buffIntensity,
                          const bool allowOverexposed,
                          const uint8_t thickness,
                          const uint8_t thicknessMode,
                          const uint32_t screenWidth,
                          const uint32_t screenHeight);

void drawLine(Pixel* buff,
              const int x1,
              const int y1,
              const int x2,
              const int y2,
              const Pixel& color,
              const uint32_t buffIntensity,
              const bool allowOverexposed,
              const uint8_t thickness,
              const uint32_t screenx,
              const uint32_t screeny)
{
  std::vector<Pixel*> buffs{buff};
  std::vector<Pixel> colors{color};
  drawLine(buffs, x1, y1, x2, y2, colors, buffIntensity, allowOverexposed, thickness, screenx,
           screeny);
}

void drawLine(std::vector<Pixel*>& buffs,
              int x1,
              int y1,
              int x2,
              int y2,
              const std::vector<Pixel>& colors,
              const uint32_t buffIntensity,
              const bool allowOverexposed,
              const uint8_t thickness,
              const uint32_t screenx,
              const uint32_t screeny)
{
  if (thickness == 1)
  {
    drawWuLine(buffs, x1, y1, x2, y2, colors, buffIntensity, allowOverexposed, screenx, screeny);
  }
  else
  {
    drawThickLine(buffs, x1, y1, x2, y2, colors, buffIntensity, allowOverexposed, thickness,
                  LINE_THICKNESS_MIDDLE, screenx, screeny);
    // plotLineWidth(n, buffs, colors, x1, y1, x2, y2, 1.0, screenx, screeny);
  }
}

using PlotFunc = const std::function<void(int x, int y, float brightess)>;
static void wuLine(float x0, float y0, float x1, float y1, const PlotFunc&);

static void drawWuLine(std::vector<Pixel*>& buffs,
                       const int x1,
                       const int y1,
                       const int x2,
                       const int y2,
                       const std::vector<Pixel>& colors,
                       const uint32_t buffIntensity,
                       const bool allowOverexposed,
                       const uint32_t screenWidth,
                       const uint32_t screenHeight)
{
  if ((y1 < 0) || (y2 < 0) || (x1 < 0) || (x2 < 0) || (y1 >= static_cast<int>(screenHeight)) ||
      (y2 >= static_cast<int>(screenHeight)) || (x1 >= static_cast<int>(screenWidth)) ||
      (x2 >= static_cast<int>(screenWidth)))
  {
    return;
  }

  assert(buffs.size() == colors.size());
  std::vector<Pixel> tempColors = colors;
  auto plot = [&](const int x, const int y, const float brightness) -> void {
    if (uint16_t(x) >= screenWidth || uint16_t(y) >= screenHeight)
    {
      return;
    }
    if (brightness < 0.001)
    {
      return;
    }
    const int pos = y * static_cast<int>(screenWidth) + x;
    if (brightness >= 0.999)
    {
      drawPixels(buffs, pos, colors, buffIntensity, allowOverexposed);
    }
    else
    {
      for (size_t i = 0; i < colors.size(); i++)
      {
        tempColors[i] = getBrighterColor(brightness, colors[i], allowOverexposed);
      }
      drawPixels(buffs, pos, tempColors, buffIntensity, allowOverexposed);
    }
  };

  wuLine(x1, y1, x2, y2, plot);
}

// The Xiaolin Wu anti-aliased draw line.
// From https://rosettacode.org/wiki/Xiaolin_Wu%27s_line_algorithm#C.2B.2B
//
static void wuLine(float x0, float y0, float x1, float y1, const PlotFunc& plot)
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

/**
 * Modified Bresenham draw(line) with optional overlap. Required for drawThickLine().
 * Overlap draws additional pixel when changing minor direction. For standard bresenham overlap, choose LINE_OVERLAP_NONE (0).
 *
 *  Sample line:
 *
 *    00+
 *     -0000+
 *         -0000+
 *             -00
 *
 *  0 pixels are drawn for normal line without any overlap
 *  + pixels are drawn if LINE_OVERLAP_MAJOR
 *  - pixels are drawn if LINE_OVERLAP_MINOR
 */

constexpr int LINE_OVERLAP_NONE = 0;
constexpr int LINE_OVERLAP_MAJOR = 1;
constexpr int LINE_OVERLAP_MINOR = 2;

static void drawLineOverlap(std::vector<Pixel*>& buffs,
                            int x0,
                            int y0,
                            int x1,
                            int y1,
                            const std::vector<Pixel>& colors,
                            const uint32_t buffIntensity,
                            const bool allowOverexposed,
                            const float brightness,
                            const uint8_t overlap,
                            const uint32_t screenWidth,
                            const uint32_t screenHeight)
{
  if ((y0 < 0) || (y1 < 0) || (x0 < 0) || (x1 < 0) || (y0 >= static_cast<int>(screenHeight)) ||
      (y1 >= static_cast<int>(screenHeight)) || (x0 >= static_cast<int>(screenWidth)) ||
      (x1 >= static_cast<int>(screenWidth)))
  {
    return;
  }

  assert(buffs.size() == colors.size());
  std::vector<Pixel> tempColors = colors;
  auto plot = [&](const int x, const int y) -> void {
    if (uint16_t(x) >= screenWidth || uint16_t(y) >= screenHeight)
    {
      return;
    }
    const int pos = y * static_cast<int>(screenWidth) + x;
    if (brightness >= 0.999)
    {
      drawPixels(buffs, pos, colors, buffIntensity, allowOverexposed);
    }
    else
    {
      for (size_t i = 0; i < colors.size(); i++)
      {
        tempColors[i] = getBrighterColor(brightness, colors[i], allowOverexposed);
      }
      drawPixels(buffs, pos, tempColors, buffIntensity, allowOverexposed);
    }
  };

  if ((x0 == x1) || (y0 == y1))
  {
    //horizontal or vertical line -> fillRect() is faster than drawLine()
    //        LocalDisplay.fillRect(aXStart, aYStart, aXEnd, aYEnd, aColor);
    // ????????????????????????????????????????????????????????????????????????????????????????????
    drawWuLine(buffs, x0, y0, x1, y1, colors, buffIntensity, allowOverexposed, screenWidth,
               screenHeight);
  }
  else
  {
    int16_t error;
    int16_t stepX;
    int16_t stepY;

    // Calculate direction.
    int16_t deltaX = x1 - x0;
    int16_t deltaY = y1 - y0;
    if (deltaX < 0)
    {
      deltaX = -deltaX;
      stepX = -1;
    }
    else
    {
      stepX = +1;
    }
    if (deltaY < 0)
    {
      deltaY = -deltaY;
      stepY = -1;
    }
    else
    {
      stepY = +1;
    }

    const int16_t deltaXTimes2 = deltaX << 1;
    const int16_t deltaYTimes2 = deltaY << 1;

    // Draw start pixel.
    plot(x0, y0);
    if (deltaX > deltaY)
    {
      // Start value represents a half step in Y direction.
      error = deltaYTimes2 - deltaX;
      while (x0 != x1)
      {
        // Step in main direction.
        x0 += stepX;
        if (error >= 0)
        {
          if (overlap == LINE_OVERLAP_MAJOR)
          {
            // Draw pixel in main direction before changing.
            plot(x0, y0);
          }
          // change Y
          y0 += stepY;
          if (overlap == LINE_OVERLAP_MINOR)
          {
            // Draw pixel in minor direction before changing.
            plot(x0 - stepX, y0);
          }
          error -= deltaXTimes2;
        }
        error += deltaYTimes2;
        plot(x0, y0);
      }
    }
    else
    {
      error = deltaXTimes2 - deltaY;
      while (y0 != y1)
      {
        y0 += stepY;
        if (error >= 0)
        {
          if (overlap == LINE_OVERLAP_MAJOR)
          {
            // Draw pixel in main direction before changing.
            plot(x0, y0);
          }
          x0 += stepX;
          if (overlap == LINE_OVERLAP_MINOR)
          {
            // Draw pixel in minor direction before changing.
            plot(x0, y0 - stepY);
          }
          error -= deltaYTimes2;
        }
        error += deltaXTimes2;
        plot(x0, y0);
      }
    }
  }
}

/**
 * Bresenham with thickness.
 * No pixel missed and every pixel only drawn once!
 * thicknessMode can be one of LINE_THICKNESS_MIDDLE, LINE_THICKNESS_DRAW_CLOCKWISE,
 *   LINE_THICKNESS_DRAW_COUNTERCLOCKWISE
 */

static void drawThickLine(std::vector<Pixel*>& buffs,
                          int x0,
                          int y0,
                          int x1,
                          int y1,
                          const std::vector<Pixel>& colors,
                          const uint32_t buffIntensity,
                          const bool allowOverexposed,
                          const uint8_t thickness,
                          const uint8_t thicknessMode,
                          const uint32_t screenWidth,
                          const uint32_t screenHeight)
{
  if ((y0 < 0) || (y1 < 0) || (x0 < 0) || (x1 < 0) || (y0 >= static_cast<int>(screenHeight)) ||
      (y1 >= static_cast<int>(screenHeight)) || (x0 >= static_cast<int>(screenWidth)) ||
      (x1 >= static_cast<int>(screenWidth)))
  {
    return;
  }

  if (thickness <= 1)
  {
    drawLineOverlap(buffs, x0, y0, x1, y1, colors, buffIntensity, allowOverexposed, 1.0, 0,
                    screenWidth, screenHeight);
  }

  const float brightness = 0.8 * 2.0 / thickness;

  /**
    * For coordinate system with 0.0 top left
    * Swap X and Y delta and calculate clockwise (new delta X inverted)
    * or counterclockwise (new delta Y inverted) rectangular direction.
    * The right rectangular direction for LINE_OVERLAP_MAJOR toggles with each octant.
  */

  int16_t error;
  int16_t stepX;
  int16_t stepY;
  int16_t deltaY = x1 - x0;
  int16_t deltaX = y1 - y0;

  // Mirror 4 quadrants to one and adjust deltas and stepping direction.
  bool swap = true; // count effective mirroring
  if (deltaX < 0)
  {
    deltaX = -deltaX;
    stepX = -1;
    swap = !swap;
  }
  else
  {
    stepX = +1;
  }
  if (deltaY < 0)
  {
    deltaY = -deltaY;
    stepY = -1;
    swap = !swap;
  }
  else
  {
    stepY = +1;
  }

  const int deltaXTimes2 = deltaX << 1;
  const int deltaYTimes2 = deltaY << 1;

  bool overlap;
  // Adjust for right direction of thickness from line origin.
  int drawStartAdjustCount = thickness / 2;
  if (thicknessMode == LINE_THICKNESS_DRAW_COUNTERCLOCKWISE)
  {
    drawStartAdjustCount = thickness - 1;
  }
  else if (thicknessMode == LINE_THICKNESS_DRAW_CLOCKWISE)
  {
    drawStartAdjustCount = 0;
  }

  // Which octant are we now?
  if (deltaX >= deltaY)
  {
    if (swap)
    {
      drawStartAdjustCount = (thickness - 1) - drawStartAdjustCount;
      stepY = -stepY;
    }
    else
    {
      stepX = -stepX;
    }
    /*
     * Vector for draw direction of start of lines is rectangular and counterclockwise to
     * main line direction. Therefore no pixel will be missed if LINE_OVERLAP_MAJOR is used
     * on change in minor rectangular direction.
     */
    // adjust draw start point
    error = deltaYTimes2 - deltaX;
    for (int i = drawStartAdjustCount; i > 0; i--)
    {
      // change X (main direction here)
      x0 -= stepX;
      x1 -= stepX;
      if (error >= 0)
      {
        // change Y
        y0 -= stepY;
        y1 -= stepY;
        error -= deltaXTimes2;
      }
      error += deltaYTimes2;
    }
    //draw start line
    drawLineOverlap(buffs, x0, y0, x1, y1, colors, buffIntensity, allowOverexposed, brightness, 1,
                    screenWidth, screenHeight);
    // draw 'thickness' number of lines
    error = deltaYTimes2 - deltaX;
    for (int i = thickness; i > 1; i--)
    {
      // change X (main direction here)
      x0 += stepX;
      x1 += stepX;
      overlap = LINE_OVERLAP_NONE;
      if (error >= 0)
      {
        // change Y
        y0 += stepY;
        y1 += stepY;
        error -= deltaXTimes2;
        /*
         * Change minor direction reverse to line (main) direction because of choosing
         * the right (counter)clockwise draw vector. Use LINE_OVERLAP_MAJOR to fill all pixels.
         *
         * EXAMPLE:
         * 1,2 = Pixel of first 2 lines
         * 3 = Pixel of third line in normal line mode
         * - = Pixel which will additionally be drawn in LINE_OVERLAP_MAJOR mode
         *           33
         *       3333-22
         *   3333-222211
         *   33-22221111
         *  221111                     /\
         *  11                          Main direction of start of lines draw vector
         *  -> Line main direction
         *  <- Minor direction of counterclockwise of start of lines draw vector
         */
        overlap = LINE_OVERLAP_MAJOR;
      }
      error += deltaYTimes2;
      drawLineOverlap(buffs, x0, y0, x1, y1, colors, buffIntensity, allowOverexposed, brightness,
                      overlap, screenWidth, screenHeight);
    }
  }
  else
  {
    // the other octant
    if (swap)
    {
      stepX = -stepX;
    }
    else
    {
      drawStartAdjustCount = (thickness - 1) - drawStartAdjustCount;
      stepY = -stepY;
    }
    // adjust draw start point
    error = deltaXTimes2 - deltaY;
    for (int i = drawStartAdjustCount; i > 0; i--)
    {
      y0 -= stepY;
      y1 -= stepY;
      if (error >= 0)
      {
        x0 -= stepX;
        x1 -= stepX;
        error -= deltaYTimes2;
      }
      error += deltaXTimes2;
    }
    // draw start line
    drawLineOverlap(buffs, x0, y0, x1, y1, colors, buffIntensity, allowOverexposed, brightness, 0,
                    screenWidth, screenHeight);
    // draw 'thickness' number of lines
    error = deltaXTimes2 - deltaY;
    for (int i = thickness; i > 1; i--)
    {
      y0 += stepY;
      y1 += stepY;
      overlap = LINE_OVERLAP_NONE;
      if (error >= 0)
      {
        x0 += stepX;
        x1 += stepX;
        error -= deltaYTimes2;
        overlap = LINE_OVERLAP_MAJOR;
      }
      error += deltaXTimes2;
      drawLineOverlap(buffs, x0, y0, x1, y1, colors, buffIntensity, allowOverexposed, brightness,
                      overlap, screenWidth, screenHeight);
    }
  }
}

} // namespace goom
