#include "colorutils.h"

#include "goom_graphic.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace goom
{

uint32_t getIntColor(const uint8_t r, const uint8_t g, const uint8_t b)
{
  return Pixel{.channels = {.r = r, .g = g, .b = b, .a = 0xff}}.val;
}

inline uint8_t lighten(const uint8_t value, const float power)
{
  const float t = static_cast<float>(value * std::log10(power) / 2.0);
  if (t <= 0.0)
  {
    return 0;
  }

  // (32.0f * log (t));
  return std::clamp(static_cast<int>(t), channel_limits<int>::min(), channel_limits<int>::max());
}

uint32_t getLightenedColor(const uint32_t oldColor, const float power)
{
  Pixel pixel{.val = oldColor};

  pixel.channels.r = lighten(pixel.channels.r, power);
  pixel.channels.g = lighten(pixel.channels.g, power);
  pixel.channels.b = lighten(pixel.channels.b, power);

  return pixel.val;
}

inline uint32_t evolvedColor(uint32_t src, uint32_t dest, const uint32_t mask, const uint32_t incr)
{
  const int32_t color = static_cast<int32_t>(src & (~mask));
  src &= mask;
  dest &= mask;

  if ((src != mask) && (src < dest))
  {
    src += incr;
  }
  if (src > dest)
  {
    src -= incr;
  }

  return static_cast<uint32_t>((src & mask) | static_cast<uint32_t>(color));
}

uint32_t getEvolvedColor(const uint32_t baseColor)
{
  uint32_t newColor = baseColor;

  newColor = evolvedColor(newColor, baseColor, 0xff, 0x01);
  newColor = evolvedColor(newColor, baseColor, 0xff00, 0x0100);
  newColor = evolvedColor(newColor, baseColor, 0xff0000, 0x010000);
  newColor = evolvedColor(newColor, baseColor, 0xff000000, 0x01000000);

  newColor = getLightenedColor(newColor, 10.0 * 2.0f + 2.0f);

  return newColor;
}

} // namespace goom
