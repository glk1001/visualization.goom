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

  auto GetDisplacementVector(const V2dFlt& normalizedPoint) const -> V2dFlt;

private:
  std::unique_ptr<UTILS::ImageBitmap> m_imageBuffer{};
  const float m_ratioNormalizedXCoordToImageCoord;
  const float m_ratioNormalizedYCoordToImageCoord;
  auto NormalizedToImagePoint(const V2dFlt& normalizedPoint) const -> V2dInt;
};

} // namespace GOOM

#endif //VISUALIZATION_GOOM_IMAGE_DISPLACEMENT_H
