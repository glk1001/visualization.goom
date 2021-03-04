#ifndef VISUALIZATION_GOOM_IMAGE_DISPLACEMENT_H
#define VISUALIZATION_GOOM_IMAGE_DISPLACEMENT_H

#include "v2d.h"

#include <memory>
#include <string>

namespace GOOM
{

namespace UTILS
{
class ImageBitmap;
} // namespace UTILS

class ImageDisplacement
{
public:
  explicit ImageDisplacement(const std::string& imageFilename);
  virtual ~ImageDisplacement() noexcept;

  auto GetXColorCutoff() -> float;
  auto GetYColorCutoff() -> float;
  void SetXYColorCutoffs(float xColorCutoff, float yColorCutoff);

  auto GetDisplacementVector(const V2dFlt& normalizedPoint) const -> V2dFlt;

private:
  std::unique_ptr<UTILS::ImageBitmap> m_imageBuffer{};
  const int32_t m_xMax;
  const int32_t m_yMax;
  const float m_ratioNormalizedXCoordToImageCoord;
  const float m_ratioNormalizedYCoordToImageCoord;
  float m_xColorCutoff = 0.5F;
  float m_yColorCutoff = 0.5F;
  auto NormalizedToImagePoint(const V2dFlt& normalizedPoint) const -> V2dInt;
};

inline auto ImageDisplacement::GetXColorCutoff() -> float
{
  return m_xColorCutoff;
}

inline auto ImageDisplacement::GetYColorCutoff() -> float
{
  return m_yColorCutoff;
}

inline void ImageDisplacement::SetXYColorCutoffs(float xColorCutoff, float yColorCutoff)
{
  m_xColorCutoff = xColorCutoff;
  m_yColorCutoff = yColorCutoff;
}

} // namespace GOOM

#endif //VISUALIZATION_GOOM_IMAGE_DISPLACEMENT_H
