#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_COLORUTILS_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_COLORUTILS_H_

#include "goom/goom_graphic.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>

namespace goom
{

Pixel getIntColor(uint8_t r, uint8_t g, uint8_t b);

Pixel getColorAverage(const std::vector<Pixel>& colors);
Pixel getColorAverage(const Pixel& color1, const Pixel& color2);
Pixel getColorBlend(const Pixel& srceColor);
Pixel getColorAdd(const Pixel& color1, const Pixel& color2, bool allowOverexposed);
Pixel getColorSubtract(const Pixel& color1, const Pixel& color2);
Pixel getBrighterColorInt(uint32_t brightness, const Pixel& color, bool allowOverexposed);
Pixel getBrighterColorInt(const float brightness, const Pixel&, const bool) = delete;

Pixel getBrighterColor(float brightness, const Pixel& color, bool allowOverexposed);
Pixel getBrighterColor(const uint32_t brightness, const Pixel&, const bool) = delete;

Pixel getRightShiftedChannels(const Pixel& color, int value);
Pixel getHalfIntensityColor(const Pixel& color);

Pixel getLightenedColor(const Pixel& oldColor, float power);
Pixel getEvolvedColor(const Pixel& baseColor);

uint32_t getLuma(const Pixel& color);

class GammaCorrection
{
public:
  GammaCorrection(float gamma, float threshold);
  [[nodiscard]] Pixel getCorrection(float brightness, const Pixel& color) const;

private:
  const float gammaReciprocal;
  const float threshold;
};


inline uint32_t colorChannelAdd(const uint8_t c1, const uint8_t c2)
{
  return static_cast<uint32_t>(c1) + static_cast<uint32_t>(c2);
}

inline uint32_t colorChannelSubtract(const uint8_t c1, const uint8_t c2)
{
  if (c1 < c2)
  {
    return 0;
  }
  return static_cast<uint32_t>(c1) - static_cast<uint32_t>(c2);
}

inline Pixel getColorAverage(const std::vector<Pixel>& colors)
{
  uint32_t newR = 0;
  uint32_t newG = 0;
  uint32_t newB = 0;
  uint32_t newA = 0;

  for (const auto& c : colors)
  {
    newR += static_cast<uint32_t>(c.R());
    newG += static_cast<uint32_t>(c.G());
    newB += static_cast<uint32_t>(c.B());
    newA += static_cast<uint32_t>(c.A());
  }

  return Pixel{{
      .r = static_cast<uint8_t>(newR / colors.size()),
      .g = static_cast<uint8_t>(newG / colors.size()),
      .b = static_cast<uint8_t>(newB / colors.size()),
      .a = static_cast<uint8_t>(newA / colors.size()),
  }};
}

inline Pixel getColorAverage(const Pixel& color1, const Pixel& color2)
{
  const uint32_t newR = colorChannelAdd(color1.R(), color2.R()) >> 1;
  const uint32_t newG = colorChannelAdd(color1.G(), color2.G()) >> 1;
  const uint32_t newB = colorChannelAdd(color1.B(), color2.B()) >> 1;
  const uint32_t newA = colorChannelAdd(color1.A(), color2.A()) >> 1;

  return Pixel{{
      .r = static_cast<uint8_t>(newR),
      .g = static_cast<uint8_t>(newG),
      .b = static_cast<uint8_t>(newB),
      .a = static_cast<uint8_t>(newA),
  }};
}

inline Pixel getColorBlend(const Pixel& srce, const Pixel& dest)
{
  const auto srceR = static_cast<int>(srce.R());
  const auto srceG = static_cast<int>(srce.G());
  const auto srceB = static_cast<int>(srce.B());
  const auto srceA = static_cast<int>(srce.A());
  const auto destR = static_cast<int>(dest.R());
  const auto destG = static_cast<int>(dest.G());
  const auto destB = static_cast<int>(dest.B());
  const auto destA = static_cast<int>(dest.A());

  const auto newR =
      static_cast<uint32_t>(destR + (srceA * (srceR - destR)) / channel_limits<int32_t>::max());
  const auto newG =
      static_cast<uint32_t>(destG + (srceA * (srceG - destG)) / channel_limits<int32_t>::max());
  const auto newB =
      static_cast<uint32_t>(destB + (srceA * (srceB - destB)) / channel_limits<int32_t>::max());
  const auto newA = std::min(channel_limits<int32_t>::max(), srceA + destA);

  return Pixel{{
      .r = static_cast<u_int8_t>(newR),
      .g = static_cast<u_int8_t>(newG),
      .b = static_cast<u_int8_t>(newB),
      .a = static_cast<u_int8_t>(newA),
  }};
}

inline Pixel getColorAdd(const Pixel& color1, const Pixel& color2, const bool allowOverexposed)
{
  uint32_t newR = colorChannelAdd(color1.R(), color2.R());
  uint32_t newG = colorChannelAdd(color1.G(), color2.G());
  uint32_t newB = colorChannelAdd(color1.B(), color2.B());
  const uint32_t newA = colorChannelAdd(color1.A(), color2.A());

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

  return Pixel{{
      .r = static_cast<uint8_t>((newR & 0xffffff00) ? 0xff : newR),
      .g = static_cast<uint8_t>((newG & 0xffffff00) ? 0xff : newG),
      .b = static_cast<uint8_t>((newB & 0xffffff00) ? 0xff : newB),
      .a = static_cast<uint8_t>((newA & 0xffffff00) ? 0xff : newA),
  }};
}

inline Pixel getColorSubtract(const Pixel& color1, const Pixel& color2)
{
  const uint32_t newR = colorChannelSubtract(color1.R(), color2.R());
  const uint32_t newG = colorChannelSubtract(color1.G(), color2.G());
  const uint32_t newB = colorChannelSubtract(color1.B(), color2.B());
  const uint32_t newA = colorChannelSubtract(color1.A(), color2.A());

  return Pixel{{
      .r = static_cast<uint8_t>(newR),
      .g = static_cast<uint8_t>(newG),
      .b = static_cast<uint8_t>(newB),
      .a = static_cast<uint8_t>(newA),
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
  uint32_t newR = getBrighterChannelColor(brightness, color.R());
  uint32_t newG = getBrighterChannelColor(brightness, color.G());
  uint32_t newB = getBrighterChannelColor(brightness, color.B());
  const uint32_t newA = getBrighterChannelColor(brightness, color.A());

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

  return Pixel{{
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
  assert(brightness >= 0.0 && brightness <= 2.0);
  const auto br = static_cast<uint32_t>(std::round(brightness * 256 + 0.0001f));
  return getBrighterColorInt(br, color, allowOverexposed);
}


inline Pixel getRightShiftedChannels(const Pixel& color, const int value)
{
  Pixel p = color;

  p.SetR(p.R() >> value);
  p.SetG(p.G() >> value);
  p.SetB(p.B() >> value);

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
  const uint32_t r = color.R();
  const uint32_t g = color.G();
  const uint32_t b = color.B();
  return (r + r + b + g + g + g) >> 3;
}

inline GammaCorrection::GammaCorrection(const float gamma, const float thresh)
  : gammaReciprocal(1.0F / gamma), threshold(thresh)
{
}

inline Pixel GammaCorrection::getCorrection(const float brightness, const Pixel& color) const
{
  if (brightness < threshold)
  {
    return getBrighterColor(brightness, color, true);
  }
  return getBrighterColor(std::pow(brightness, gammaReciprocal), color, true);
}

} // namespace goom

#endif
