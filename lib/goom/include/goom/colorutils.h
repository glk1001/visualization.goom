#ifndef _GOOM_COLORUTILS_H
#define _GOOM_COLORUTILS_H

#include "goom_graphic.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>

namespace goom
{

uint32_t getIntColor(const uint8_t r, const uint8_t g, const uint8_t b);

Pixel getColorAdd(const Pixel& color1, const Pixel& color2, const bool allowOverexposed);
Pixel getBrighterColorInt(const uint32_t brightness,
                          const Pixel& color,
                          const bool allowOverexposed);

Pixel getBrighterColor(const float brightness, const Pixel& color, const bool allowOverexposed);
uint32_t getBrighterColor(const float brightness,
                          const uint32_t color,
                          const bool allowOverexposed);

Pixel getRightShiftedChannels(const Pixel& color, const int value);
Pixel getHalfIntensityColor(const Pixel& color);

uint32_t getLightenedColor(const uint32_t oldColor, const float power);
uint32_t getEvolvedColor(const uint32_t baseColor);

uint32_t getLuma(const Pixel& color);


inline uint32_t colorChannelAdd(const uint8_t c1, const uint8_t c2)
{
  return static_cast<uint32_t>(c1) + static_cast<uint32_t>(c2);
}

inline Pixel getColorAdd(const Pixel& color1, const Pixel& color2, const bool allowOverexposed)
{
  uint32_t newR = colorChannelAdd(color1.channels.r, color2.channels.r);
  uint32_t newG = colorChannelAdd(color1.channels.g, color2.channels.g);
  uint32_t newB = colorChannelAdd(color1.channels.b, color2.channels.b);
  const uint32_t newA = colorChannelAdd(color1.channels.a, color2.channels.a);

  if (!allowOverexposed)
  {
    const uint32_t maxVal = std::max({newR, newG, newB});
    if (maxVal > channel_limits<uint32_t>::max())
    {
      // scale all channels back
      newR = (newR << 8) / maxVal;
      newG = (newG << 8) / maxVal;
      newB = (newB << 8) / maxVal;
    }
  }

  return Pixel{.channels{
      .r = static_cast<uint8_t>((newR & 0xffffff00) ? 0xff : newR),
      .g = static_cast<uint8_t>((newG & 0xffffff00) ? 0xff : newG),
      .b = static_cast<uint8_t>((newB & 0xffffff00) ? 0xff : newB),
      .a = static_cast<uint8_t>((newA & 0xffffff00) ? 0xff : newA),
  }};
}


inline uint32_t getBrighterChannelColor(const uint32_t brightness, const uint8_t channelVal)
{
  return (brightness * static_cast<uint32_t>(channelVal)) >> 8;
}

inline Pixel getBrighterColorInt(const uint32_t brightness,
                                 const Pixel& color,
                                 const bool allowOverexposed)
{
  uint32_t newR = getBrighterChannelColor(brightness, color.channels.r);
  uint32_t newG = getBrighterChannelColor(brightness, color.channels.g);
  uint32_t newB = getBrighterChannelColor(brightness, color.channels.b);
  const uint32_t newA = getBrighterChannelColor(brightness, color.channels.a);

  if (!allowOverexposed)
  {
    const uint32_t maxVal = std::max({newR, newG, newB});
    if (maxVal > channel_limits<uint32_t>::max())
    {
      // scale all channels back
      newR = (newR << 8) / maxVal;
      newG = (newG << 8) / maxVal;
      newB = (newB << 8) / maxVal;
    }
  }

  return Pixel{.channels{
      .r = static_cast<uint8_t>((newR & 0xffffff00) ? 0xff : newR),
      .g = static_cast<uint8_t>((newG & 0xffffff00) ? 0xff : newG),
      .b = static_cast<uint8_t>((newB & 0xffffff00) ? 0xff : newB),
      .a = static_cast<uint8_t>((newA & 0xffffff00) ? 0xff : newA),
  }};
}


inline Pixel getBrighterColor(const float brightness,
                              const Pixel& color,
                              const bool allowOverexposed)
{
  assert(brightness >= 0.0 && brightness <= 1.0);
  const uint32_t br = static_cast<uint32_t>(std::round(brightness * 256 + 0.0001f));
  return getBrighterColorInt(br, color, allowOverexposed);
}

inline uint32_t getBrighterColor(const float brightness,
                                 const uint32_t color,
                                 const bool allowOverexposed)
{
  return getBrighterColorInt(brightness, Pixel{.val = color}, allowOverexposed).val;
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
