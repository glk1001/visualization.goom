#include "image_displacement.h"

#include "filter_buffers.h"
#include "goomutils/goomrand.h"
#include "goomutils/image_bitmaps.h"
#include "goomutils/mathutils.h"

#include <cmath>

namespace GOOM
{

using UTILS::GetRandInRange;
using UTILS::ImageBitmap;
using UTILS::ProbabilityOfMInN;

ImageDisplacement::ImageDisplacement(const std::string& imageFilename)
  : m_imageBuffer(std::make_unique<ImageBitmap>(imageFilename)),
    m_ratioNormalizedXCoordToImageCoord{
        static_cast<float>(m_imageBuffer->GetWidth() - 1) /
        (ZoomFilterBuffers::MAX_NORMALIZED_COORD - ZoomFilterBuffers::MIN_NORMALIZED_COORD)},
    m_ratioNormalizedYCoordToImageCoord{
        static_cast<float>(m_imageBuffer->GetHeight() - 1) /
        (ZoomFilterBuffers::MAX_NORMALIZED_COORD - ZoomFilterBuffers::MIN_NORMALIZED_COORD)}
{
}

ImageDisplacement::~ImageDisplacement() noexcept = default;

auto ImageDisplacement::GetDisplacementVector(const V2dFlt& normalizedPoint) const -> V2dFlt
{
  const V2dInt imagePoint = NormalizedToImagePoint(normalizedPoint);
  const Pixel color =
      (*m_imageBuffer)(static_cast<size_t>(imagePoint.x), static_cast<size_t>(imagePoint.y));

  const float x = color.RFlt() - 0.5F;
  const float y = color.GFlt() - 0.5F;
  //const float y = (ProbabilityOfMInN(1, 2) ? color.GFlt() : color.BFlt()) - 0.5F;

  return {x, y};
}

inline auto ImageDisplacement::NormalizedToImagePoint(const V2dFlt& normalizedPoint) const -> V2dInt
{
  const int32_t x = static_cast<int32_t>(
      std::lround(m_ratioNormalizedXCoordToImageCoord *
                  (normalizedPoint.x - ZoomFilterBuffers::MIN_NORMALIZED_COORD)));
  const int32_t y = static_cast<int32_t>(
      std::lround(m_ratioNormalizedYCoordToImageCoord *
                  (normalizedPoint.y - ZoomFilterBuffers::MIN_NORMALIZED_COORD)));

  constexpr int32_t FUZZ = 3;
  return {stdnew::clamp(GetRandInRange(x - FUZZ, x + FUZZ), 0,
                        static_cast<int32_t>(m_imageBuffer->GetWidth() - 1)),
          stdnew::clamp(GetRandInRange(y - FUZZ, y + FUZZ), 0,
                        static_cast<int32_t>(m_imageBuffer->GetHeight() - 1))};
}

} // namespace GOOM
