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
  GoomDraw(uint32_t screenWidth, uint32_t screenHeight);

  [[nodiscard]] bool getAllowOverexposed() const;
  void setAllowOverexposed(bool val);

  [[nodiscard]] float getBuffIntensity() const;
  void setBuffIntensity(float val);

  [[nodiscard]] Pixel getPixelRGB(const PixelBuffer&, uint32_t x, uint32_t y) const;
  void setPixelRGB(PixelBuffer&, uint32_t x, uint32_t y, const Pixel& color) const;
  // Set the pixel but don't blend it with the existing pixel value.
  void setPixelRGBNoBlend(PixelBuffer&, uint32_t x, uint32_t y, const Pixel& color);

  [[nodiscard]] std::vector<Pixel> getPixelRGB(const std::vector<PixelBuffer*>&,
                                               uint32_t x,
                                               uint32_t y) const;
  void setPixelRGB(std::vector<PixelBuffer*>&,
                   uint32_t x,
                   uint32_t y,
                   const std::vector<Pixel>& colors) const;

  void circle(PixelBuffer&, int x0, int y0, int radius, const Pixel& color) const;

  void circle(std::vector<PixelBuffer*>&,
              int x0,
              int y0,
              int radius,
              const std::vector<Pixel>& colors) const;

  void filledCircle(
      PixelBuffer&, int x0, int y0, int radius, const std::vector<Pixel>& colors) const;

  void filledCircle(std::vector<PixelBuffer*>&,
                    int x0,
                    int y0,
                    int radius,
                    const std::vector<std::vector<Pixel>>& colorSets) const;

  void line(
      PixelBuffer&, int x1, int y1, int x2, int y2, const Pixel& color, uint8_t thickness) const;

  void line(std::vector<PixelBuffer*>&,
            int x1,
            int y1,
            int x2,
            int y2,
            const std::vector<Pixel>& colors,
            uint8_t thickness) const;

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
