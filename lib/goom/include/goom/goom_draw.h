#ifndef LIBS_GOOM_INCLUDE_GOOM_GOOM_DRAW_H_
#define LIBS_GOOM_INCLUDE_GOOM_GOOM_DRAW_H_

#include "goom_config.h"
#include "goom_graphic.h"

#include <cereal/archives/json.hpp>
#include <cstddef>
#include <cstdint>
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

  bool getAllowOverexposed() const;
  void setAllowOverexposed(const bool val);

  float getBuffIntensity() const;
  void setBuffIntensity(const float val);

  Pixel getPixelRGB(const PixelBuffer&, const uint32_t x, const uint32_t y) const;
  void setPixelRGB(PixelBuffer&, const uint32_t x, const uint32_t y, const Pixel& color);
  // Set the pixel but don't blend it with the existing pixel value.
  void setPixelRGBNoBlend(PixelBuffer&, const uint32_t x, const uint32_t y, const Pixel& color);

  std::vector<Pixel> getPixelRGB(const std::vector<PixelBuffer*>&,
                                 const uint32_t x,
                                 const uint32_t y) const;
  void setPixelRGB(std::vector<PixelBuffer*>&,
                   const uint32_t x,
                   const uint32_t y,
                   const std::vector<Pixel>& colors);

  void circle(PixelBuffer&, const int x0, const int y0, const int radius, const Pixel& color);

  void circle(std::vector<PixelBuffer*>&,
              const int x0,
              const int y0,
              const int radius,
              const std::vector<Pixel>& colors);

  void filledCircle(
      PixelBuffer&, const int x0, const int y0, const int radius, const std::vector<Pixel>& colors);

  void filledCircle(std::vector<PixelBuffer*>&,
                    const int x0,
                    const int y0,
                    const int radius,
                    const std::vector<std::vector<Pixel>>& colorSets);

  void line(PixelBuffer&,
            const int x1,
            const int y1,
            const int x2,
            const int y2,
            const Pixel& color,
            const uint8_t thickness);

  void line(std::vector<PixelBuffer*>&,
            int x1,
            int y1,
            int x2,
            int y2,
            const std::vector<Pixel>& colors,
            const uint8_t thickness);

  bool operator==(const GoomDraw&) const;

  template<class Archive>
  void serialize(Archive&);

private:
  uint32_t screenWidth;
  uint32_t screenHeight;
  bool allowOverexposed = false;
  float buffIntensity = 1;
  uint32_t intBuffIntensity = channel_limits<uint32_t>::max();
};

template<class Archive>
void GoomDraw::serialize(Archive& ar)
{
  ar(screenWidth, screenHeight, allowOverexposed, buffIntensity, intBuffIntensity);
}

inline void GoomDraw::setPixelRGBNoBlend(PixelBuffer& buff,
                                         const uint32_t x,
                                         const uint32_t y,
                                         const Pixel& color)
{
  buff(x, y) = color;
}

inline Pixel GoomDraw::getPixelRGB(const PixelBuffer& buff,
                                   const uint32_t x,
                                   const uint32_t y) const
{
  return buff(x, y);
}

inline std::vector<Pixel> GoomDraw::getPixelRGB(const std::vector<PixelBuffer*>& buffs,
                                                const uint32_t x,
                                                const uint32_t y) const
{
  const uint32_t pos = x + (y * screenWidth);
  std::vector<Pixel> colors(buffs.size());
  for (size_t i = 0; i < buffs.size(); i++)
  {
    colors[i] = (*buffs[i])(pos);
  }
  return colors;
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
