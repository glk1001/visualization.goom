#ifndef LIBS_GOOM_INCLUDE_GOOM_GOOM_DRAW_H_
#define LIBS_GOOM_INCLUDE_GOOM_GOOM_DRAW_H_

#include "goom_config.h"
#include "goom_graphic.h"

#include <cstddef>
#include <cstdint>
// TODO Try to avoid including this
#include <nlohmann/json.hpp>
#include <vector>

namespace goom
{

class GoomDraw
{
public:
  GoomDraw();
  GoomDraw(const uint32_t screenWidth, const uint32_t screenHeight);

  uint32_t getScreenWidth() const;
  uint32_t getScreenHeight() const;

  void setScreenWidth(const uint32_t val);
  void setScreenHeight(const uint32_t val);

  bool getAllowOverexposed() const;
  void setAllowOverexposed(const bool val);

  float getBuffIntensity() const;
  void setBuffIntensity(const float val);

  Pixel getPixelRGB(Pixel* buff, const uint32_t x, const uint32_t y) const;
  void setPixelRGB(Pixel* buff, const uint32_t x, const uint32_t y, const Pixel& color);
  // Set the pixel but don't blend it with the existing pixel value.
  void setPixelRGBNoBlend(Pixel* buff, const uint32_t x, const uint32_t y, const Pixel& color);

  std::vector<Pixel> getPixelRGB(const std::vector<Pixel*>& buffs,
                                 const uint32_t x,
                                 const uint32_t y) const;
  void setPixelRGB(std::vector<Pixel*>& buffs,
                   const uint32_t x,
                   const uint32_t y,
                   const std::vector<Pixel>& colors);

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

  void line(std::vector<Pixel*> buffs,
            int x1,
            int y1,
            int x2,
            int y2,
            const std::vector<Pixel>& colors,
            const uint8_t thickness);

private:
  uint32_t screenWidth;
  uint32_t screenHeight;
  bool allowOverexposed = false;
  float buffIntensity = 1;
  uint32_t intBuffIntensity = channel_limits<uint32_t>::max();
};

void from_json(const nlohmann::json&, GoomDraw&);

inline void GoomDraw::setPixelRGBNoBlend(Pixel* buff,
                                         const uint32_t x,
                                         const uint32_t y,
                                         const Pixel& color)
{
  buff[x + (y * screenWidth)] = color;
}

inline Pixel GoomDraw::getPixelRGB(Pixel* buff, const uint32_t x, const uint32_t y) const
{
  return buff[x + (y * screenWidth)];
}

inline std::vector<Pixel> GoomDraw::getPixelRGB(const std::vector<Pixel*>& buffs,
                                                const uint32_t x,
                                                const uint32_t y) const
{
  const uint32_t pos = x + (y * screenWidth);
  std::vector<Pixel> colors(buffs.size());
  for (size_t i = 0; i < buffs.size(); i++)
  {
    colors[i] = (buffs[i])[pos];
  }
  return colors;
}

inline uint32_t GoomDraw::getScreenWidth() const
{
  return screenWidth;
}

inline void GoomDraw::setScreenWidth(const uint32_t val)
{
  screenWidth = val;
}

inline uint32_t GoomDraw::getScreenHeight() const
{
  return screenHeight;
}

inline void GoomDraw::setScreenHeight(const uint32_t val)
{
  screenHeight = val;
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
