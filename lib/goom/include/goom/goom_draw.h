#ifndef VISUALIZATION_GOOM_GOOM_DRAW_H
#define VISUALIZATION_GOOM_GOOM_DRAW_H

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

  [[nodiscard]] auto GetAllowOverexposed() const -> bool;
  void SetAllowOverexposed(bool val);

  [[nodiscard]] auto GetBuffIntensity() const -> float;
  void SetBuffIntensity(float val);

  [[nodiscard]] auto GetPixelRgb(const PixelBuffer& buff, uint32_t x, uint32_t y) const -> Pixel;
  void SetPixelRgb(PixelBuffer& buff, uint32_t x, uint32_t y, const Pixel& color) const;
  // Set the pixel but don't blend it with the existing pixel value.
  void SetPixelRgbNoBlend(PixelBuffer& buff, uint32_t x, uint32_t y, const Pixel& color);

  [[nodiscard]] auto GetPixelRgb(const std::vector<PixelBuffer*>& buffs,
                                 uint32_t x,
                                 uint32_t y) const -> std::vector<Pixel>;
  void SetPixelRgb(std::vector<PixelBuffer*>& buff,
                   uint32_t x,
                   uint32_t y,
                   const std::vector<Pixel>& colors) const;

  void Circle(PixelBuffer&, int x0, int y0, int radius, const Pixel& color) const;

  void Circle(std::vector<PixelBuffer*>& buffs,
              int x0,
              int y0,
              int radius,
              const std::vector<Pixel>& colors) const;

  void FilledCircle(PixelBuffer& buff, int x0, int y0, int radius, const Pixel& color) const;

  void FilledCircle(std::vector<PixelBuffer*>& buffs,
                    int x0,
                    int y0,
                    int radius,
                    const std::vector<Pixel>& colors) const;

  void Line(
      PixelBuffer&, int x1, int y1, int x2, int y2, const Pixel& color, uint8_t thickness) const;

  void Line(std::vector<PixelBuffer*>& buffs,
            int x1,
            int y1,
            int x2,
            int y2,
            const std::vector<Pixel>& colors,
            uint8_t thickness) const;

  auto operator==(const GoomDraw&) const -> bool;

  template<class Archive>
  void serialize(Archive&);

private:
  uint32_t m_screenWidth;
  uint32_t m_screenHeight;
  bool m_allowOverexposed = false;
  float m_buffIntensity = 1.0;
  uint32_t m_intBuffIntensity = channel_limits<uint32_t>::max();
  bool m_fontsLoaded = false;
};

template<class Archive>
void GoomDraw::serialize(Archive& ar)
{
  ar(m_screenWidth, m_screenHeight, m_allowOverexposed, m_buffIntensity, m_intBuffIntensity);
}

inline void GoomDraw::SetPixelRgbNoBlend(PixelBuffer& buff,
                                         const uint32_t x,
                                         const uint32_t y,
                                         const Pixel& color)
{
  buff(x, y) = color;
}

inline auto GoomDraw::GetPixelRgb(const PixelBuffer& buff, const uint32_t x, const uint32_t y) const
    -> Pixel
{
  return buff(x, y);
}

inline auto GoomDraw::GetPixelRgb(const std::vector<PixelBuffer*>& buffs,
                                  const uint32_t x,
                                  const uint32_t y) const -> std::vector<Pixel>
{
  const uint32_t pos = x + (y * m_screenWidth);
  std::vector<Pixel> colors(buffs.size());
  for (size_t i = 0; i < buffs.size(); i++)
  {
    colors[i] = (*buffs[i])(pos);
  }
  return colors;
}

[[maybe_unused]] inline auto GoomDraw::GetAllowOverexposed() const -> bool
{
  return m_allowOverexposed;
}

inline void GoomDraw::SetAllowOverexposed(const bool val)
{
  m_allowOverexposed = val;
}

inline auto GoomDraw::GetBuffIntensity() const -> float
{
  return m_buffIntensity;
}

inline void GoomDraw::SetBuffIntensity(const float val)
{
  m_buffIntensity = val;
  m_intBuffIntensity = static_cast<uint32_t>(channel_limits<float>::max() * m_buffIntensity);
}

} // namespace goom

#endif /* VISUALIZATION_GOOM_GOOM_DRAW_H */
