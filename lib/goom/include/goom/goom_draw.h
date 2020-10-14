#ifndef LIBS_GOOM_INCLUDE_GOOM_GOOM_DRAW_H_
#define LIBS_GOOM_INCLUDE_GOOM_GOOM_DRAW_H_

#include "goom_config.h"
#include "goom_graphic.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace goom
{

class GoomDraw
{
public:
  GoomDraw(const uint32_t screenWidth, const uint32_t screenHeight);

  uint32_t getScreenWidth() const;
  uint32_t getScreenHeight() const;

  bool getAllowOverexposed() const;
  void setAllowOverexposed(const bool val);

  float getBuffIntensity() const;
  void setBuffIntensity(const float val);

  Pixel getPixelRGB(Pixel* buff, const uint32_t x, const uint32_t y);
  void setPixelRGB(Pixel* buff, const uint32_t x, const uint32_t y, const Pixel& color);

  void circle(Pixel* buff, const int x0, const int y0, const int radius, const uint32_t color);

  void filledCircle(Pixel* buff,
                    const int x0,
                    const int y0,
                    const int radius,
                    const std::vector<uint32_t>& colors);

  void line(Pixel* buff,
            const int x1,
            const int y1,
            const int x2,
            const int y2,
            const uint32_t color,
            const uint8_t thickness);

  void line(const size_t n,
            Pixel* buffs[],
            int x1,
            int y1,
            int x2,
            int y2,
            const std::vector<Pixel>& colors,
            const uint8_t thickness);

private:
  const uint32_t screenWidth;
  const uint32_t screenHeight;
  bool allowOverexposed = false;
  float buffIntensity = 1;
  uint32_t intBuffIntensity = channel_limits<uint32_t>::max();
};

inline Pixel GoomDraw::getPixelRGB(Pixel* buff, const uint32_t x, const uint32_t y)
{
  return buff[x + (y * screenWidth)];
}

inline void GoomDraw::setPixelRGB(Pixel* buff,
                                  const uint32_t x,
                                  const uint32_t y,
                                  const Pixel& color)
{
  buff[x + (y * screenWidth)] = color;
}

inline uint32_t GoomDraw::getScreenWidth() const
{
  return screenWidth;
}

inline uint32_t GoomDraw::getScreenHeight() const
{
  return screenHeight;
}

inline bool GoomDraw::getAllowOverexposed() const
{
  return allowOverexposed;
}

inline void GoomDraw::setAllowOverexposed(const bool val)
{
  allowOverexposed = val;
}

inline float GoomDraw::getBuffIntensity() const
{
  return buffIntensity;
}

inline void GoomDraw::setBuffIntensity(const float val)
{
  buffIntensity = val;
  intBuffIntensity = static_cast<uint32_t>(channel_limits<float>::max() * buffIntensity);
}

} // namespace goom

#endif /* LIBS_GOOM_INCLUDE_GOOM_GOOM_DRAW_H_ */
