#ifndef _DRAW_METHODS_H
#define _DRAW_METHODS_H

#include "goom_graphic.h"
#include "goomutils/colorutils.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace goom
{

void drawCircle(PixelBuffer&,
                const int x0,
                const int y0,
                const int radius,
                const Pixel& color,
                const uint32_t buffIntensity,
                const bool allowOverexposed,
                const uint32_t screenWidth,
                const uint32_t screenHeight);

void drawCircle(std::vector<PixelBuffer*>&,
                const int x0,
                const int y0,
                const int radius,
                const std::vector<Pixel>& colors,
                const uint32_t buffIntensity,
                const bool allowOverexposed,
                const uint32_t screenWidth,
                const uint32_t screenHeight);

// colors.size() == radius
void drawFilledCircle(PixelBuffer&,
                      const int x0,
                      const int y0,
                      const int radius,
                      const std::vector<Pixel>& colors,
                      const uint32_t buffIntensity,
                      const bool allowOverexposed,
                      const uint32_t screenWidth,
                      const uint32_t screenHeight);

void drawFilledCircle(std::vector<PixelBuffer*>&,
                      const int x0,
                      const int y0,
                      const int radius,
                      const std::vector<std::vector<Pixel>>& colorSets,
                      const uint32_t buffIntensity,
                      const bool allowOverexposed,
                      const uint32_t screenWidth,
                      const uint32_t screenHeight);

void drawLine(std::vector<PixelBuffer*>&,
              const int x1,
              const int y1,
              const int x2,
              const int y2,
              const std::vector<Pixel>& colors,
              const uint32_t buffIntensity,
              const bool allowOverexposed,
              const uint8_t thickness,
              const uint32_t screenx,
              const uint32_t screeny);

void drawLine(PixelBuffer&,
              int x1,
              int y1,
              int x2,
              int y2,
              const Pixel& color,
              const uint32_t buffIntensity,
              const bool allowOverexposed,
              const uint8_t thickness,
              const uint32_t screenx,
              const uint32_t screeny);

inline void drawPixels(std::vector<PixelBuffer*>& buffs,
                       const int pos,
                       const std::vector<Pixel>& newColors,
                       const uint32_t buffIntensity,
                       const bool allowOverexposed)
{
  for (size_t i = 0; i < buffs.size(); i++)
  {
    const Pixel brighterPixColor =
        getBrighterColorInt(buffIntensity, newColors[i], allowOverexposed);
    Pixel& p = (*buffs[i])(static_cast<size_t>(pos));
    p = getColorAdd(p, brighterPixColor, allowOverexposed);

    /***
	ATTEMPT AT BLENDING - WON'T WORK THOUGH - BECAUSE OF MULTIPLE BUFFERS??
    Pixel* const p = &(buffs[i][pos]);
    const Pixel existingColorBlended =
        getBrighterColorInt(buffIntensity, *p, allowOverexposed);
    const Pixel pixColorBlended =
        getBrighterColorInt(channel_limits<uint32_t>::max() - buffIntensity, newColors[i], allowOverexposed);
    *p = getColorAdd(existingColorBlended, pixColorBlended, allowOverexposed);
    ***/
  }
}

} // namespace goom
#endif
