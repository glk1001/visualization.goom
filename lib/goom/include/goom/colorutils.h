#ifndef _GOOM_COLORUTILS_H
#define _GOOM_COLORUTILS_H

#include "goom_graphic.h"

#include <cassert>
#include <cmath>
#include <cstdint>

namespace goom
{

uint32_t getIntColor(const uint8_t r, const uint8_t g, const uint8_t b);

inline uint8_t colorChannelAdd(const uint8_t c1, const uint8_t c2)
{
  uint32_t cadd = static_cast<uint32_t>(c1) + static_cast<uint32_t>(c2);
  if (cadd > 255)
  {
    cadd = 255;
  }
  return static_cast<uint8_t>(cadd);
}

inline Pixel getColorAdd(const Pixel& color1, const Pixel& color2)
{
  return Pixel{.channels{
      .r = colorChannelAdd(color1.channels.r, color2.channels.r),
      .g = colorChannelAdd(color1.channels.g, color2.channels.g),
      .b = colorChannelAdd(color1.channels.b, color2.channels.b),
      .a = colorChannelAdd(color1.channels.a, color2.channels.a),
  }};
}

inline uint8_t getBrighterChannelColor(const uint32_t brightness, const uint8_t c)
{
  const uint32_t newColor = (brightness * static_cast<uint32_t>(c)) >> 8;
  return (newColor & 0xffffff00) ? 0xff : static_cast<uint8_t>(newColor);
}

inline Pixel getBrighterColor(const uint32_t brightness, const Pixel& color)
{
  return Pixel{.channels{
      .r = getBrighterChannelColor(brightness, color.channels.r),
      .g = getBrighterChannelColor(brightness, color.channels.g),
      .b = getBrighterChannelColor(brightness, color.channels.b),
      .a = getBrighterChannelColor(brightness, color.channels.a),
  }};
}

inline Pixel getBrighterColor(const float brightness, const Pixel& color)
{
  assert(brightness >= 0.0 && brightness <= 1.0);
  const uint32_t br = static_cast<uint32_t>(std::round(brightness * 256 + 0.0001f));
  return getBrighterColor(br, color);
}

inline uint32_t getBrighterColor(const float brightness, const uint32_t color)
{
  return getBrighterColor(brightness, Pixel{.val = color}).val;
}

inline Pixel getRightShiftedChannels(const Pixel& color, const int value)
{
  Pixel p = color;

  p.channels.r >>= value;
  p.channels.g >>= value;
  p.channels.b >>= value;

  return p;
}

inline Pixel getHalfIntensityColor(const Pixel& color)
{
  return getRightShiftedChannels(color, 1);
}

uint32_t getLightenedColor(const uint32_t oldColor, const float power);
uint32_t getEvolvedColor(const uint32_t baseColor);

inline void setPixelRGB(
    Pixel* buff, const uint32_t x, const uint32_t y, const uint32_t screenWidth, const uint32_t col)
{
  buff[x + (y * screenWidth)].val = col;
}


// RGB -> Luma conversion formula.
//
// Photometric/digital ITU BT.709:
//
//     Y = 0.2126 R + 0.7152 G + 0.0722 B
//
// Digital ITU BT.601 (gives more weight to the R and B components):
//
//     Y = 0.299 R + 0.587 G + 0.114 B
//
// If you are willing to trade accuracy for perfomance, there are two approximation formulas for this one:
//
//     Y = 0.33 R + 0.5 G + 0.16 B
//
//     Y = 0.375 R + 0.5 G + 0.125 B
//
// These can be calculated quickly as
//
//     Y = (R+R+B+G+G+G)/6
//
//     Y = (R+R+R+B+G+G+G+G)>>3

inline uint32_t getLuma(const Pixel& color)
{
  const uint32_t r = color.channels.r;
  const uint32_t g = color.channels.g;
  const uint32_t b = color.channels.b;
  return (r + r + b + g + g + g) >> 3;
}

} // namespace goom

#endif
