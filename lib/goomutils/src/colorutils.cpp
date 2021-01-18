#include "colorutils.h"

#include "goom/goom_graphic.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace GOOM
{

static_assert(sizeof(Pixel) == sizeof(uint32_t));

Pixel GetIntColor(uint8_t r, uint8_t g, uint8_t b)
{
  return Pixel{{.r = r, .g = g, .b = b, .a = 0xff}};
}

inline uint8_t lighten(const uint8_t value, const float power)
{
  const auto t = static_cast<float>(value * std::log10(power) / 2.0);
  if (t <= 0.0)
  {
    return 0;
  }

  // (32.0f * log (t));
  return std::clamp(static_cast<int>(t), channel_limits<int>::min(), channel_limits<int>::max());
}

Pixel getLightenedColor(const Pixel& oldColor, const float power)
{
  Pixel pixel = oldColor;

  pixel.SetR(lighten(pixel.R(), power));
  pixel.SetG(lighten(pixel.G(), power));
  pixel.SetB(lighten(pixel.B(), power));

  return pixel;
}

inline Pixel evolvedColor(const Pixel& src,
                          const Pixel& dest,
                          const uint32_t mask,
                          const uint32_t incr)
{
  const auto color = static_cast<int32_t>(src.Rgba() & (~mask));
  uint32_t isrc = src.Rgba() & mask;
  const uint32_t idest = dest.Rgba() & mask;

  if ((isrc != mask) && (isrc < idest))
  {
    isrc += incr;
  }
  if (isrc > idest)
  {
    isrc -= incr;
  }

  return Pixel{static_cast<uint32_t>((isrc & mask) | static_cast<uint32_t>(color))};
}

Pixel getEvolvedColor(const Pixel& baseColor)
{
  Pixel newColor = baseColor;

  newColor = evolvedColor(newColor, baseColor, 0xff, 0x01);
  newColor = evolvedColor(newColor, baseColor, 0xff00, 0x0100);
  newColor = evolvedColor(newColor, baseColor, 0xff0000, 0x010000);
  newColor = evolvedColor(newColor, baseColor, 0xff000000, 0x01000000);

  newColor = getLightenedColor(newColor, 10.0 * 2.0f + 2.0f);

  return newColor;
}

} // namespace GOOM
