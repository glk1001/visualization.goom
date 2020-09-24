#ifndef _GOOM_COLORUTILS_H
#define _GOOM_COLORUTILS_H

#include "goom_graphic.h"

#include <cassert>
#include <cmath>
#include <cstdint>

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
  Pixel p;

  p.channels.r = colorChannelAdd(color1.channels.r, color2.channels.r);
  p.channels.g = colorChannelAdd(color1.channels.g, color2.channels.g);
  p.channels.b = colorChannelAdd(color1.channels.b, color2.channels.b);
  p.channels.a = colorChannelAdd(color1.channels.a, color2.channels.a);

  return p;
}

inline uint8_t getBrighterChannelColor(const uint32_t br, const uint8_t c)
{
  return static_cast<uint8_t>((br * static_cast<uint32_t>(c)) >> 8);
}

inline Pixel getBrighterColor(const float brightness, const Pixel& color)
{
  assert(brightness >= 0.0 && brightness <= 1.0);
  const uint32_t br = static_cast<uint32_t>(std::round(brightness * 256 + 0.0001f));
  Pixel p;
  p.channels.r = getBrighterChannelColor(br, color.channels.r);
  p.channels.g = getBrighterChannelColor(br, color.channels.g);
  p.channels.b = getBrighterChannelColor(br, color.channels.b);
  p.channels.a = color.channels.a;
  return p;
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
  const Pixel p{.val = col};

  *(buff + (x + (y * screenWidth))) = p;
}

#endif
