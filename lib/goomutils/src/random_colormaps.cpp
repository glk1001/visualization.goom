#include "random_colormaps.h"

#include "colordata/colormap_enums.h"
#include "colormaps.h"
#include "goom/goom_graphic.h"
#include "goomrand.h"

namespace GOOM::UTILS
{

using COLOR_DATA::ColorMapName;

RandomColorMaps::RandomColorMaps() noexcept = default;

RandomColorMaps::~RandomColorMaps() noexcept = default;

auto RandomColorMaps::GetRandomColorMapName() const -> COLOR_DATA::ColorMapName
{
  return static_cast<ColorMapName>(GetRandInRange(0U, GetNumColorMapNames()));
}

auto RandomColorMaps::GetRandomColorMapName(ColorMapGroup cmg) const -> COLOR_DATA::ColorMapName
{
  const ColorMapNames& colorMapNames = GetColorMapNames(cmg);
  return colorMapNames[GetRandInRange(0U, colorMapNames.size())];
}

auto RandomColorMaps::GetRandomColorMap() const -> const IColorMap&
{
  return GetColorMap(GetRandomColorMapName());
}

auto RandomColorMaps::GetRandomColorMap(ColorMapGroup cmg) const -> const IColorMap&
{
  return GetColorMap(GetRandomColorMapName(cmg));
}

auto RandomColorMaps::GetRandomColorMapPtr(const bool includeRotatePoints) const
    -> std::shared_ptr<const IColorMap>
{
  if (!includeRotatePoints)
  {
    return GetColorMapPtr(GetRandomColorMapName());
  }
  return GetColorMapPtr(GetRandomColorMapName(), GetRandInRange(0.1F, 0.9F));
}

auto RandomColorMaps::GetRandomColorMapPtr(ColorMapGroup cmg, const bool includeRotatePoints) const
    -> std::shared_ptr<const IColorMap>
{
  if (!includeRotatePoints)
  {
    return GetColorMapPtr(GetRandomColorMapName(cmg));
  }
  return GetColorMapPtr(GetRandomColorMapName(cmg), GetRandInRange(0.1F, 0.9F));
}

auto RandomColorMaps::GetRandomGroup() const -> ColorMapGroup
{
  return static_cast<ColorMapGroup>(GetRandInRange(0U, static_cast<size_t>(ColorMapGroup::_SIZE)));
}

auto RandomColorMaps::GetRandomColor(const IColorMap& colorMap, float t0, float t1) -> Pixel
{
  return colorMap.GetColor(GetRandInRange(t0, t1));
}

WeightedColorMaps::WeightedColorMaps() : RandomColorMaps{}, m_weights{}, m_weightsActive{true}
{
}

WeightedColorMaps::WeightedColorMaps(const Weights<ColorMapGroup>& w)
  : RandomColorMaps{}, m_weights{w}, m_weightsActive{true}
{
}

auto WeightedColorMaps::GetWeights() const -> const Weights<ColorMapGroup>&
{
  return m_weights;
}

void WeightedColorMaps::SetWeights(const Weights<ColorMapGroup>& w)
{
  m_weights = w;
}

auto WeightedColorMaps::AreWeightsActive() const -> bool
{
  return m_weightsActive;
}

void WeightedColorMaps::SetWeightsActive(const bool value)
{
  m_weightsActive = value;
}

auto WeightedColorMaps::GetRandomGroup() const -> ColorMapGroup
{
  if (!m_weightsActive)
  {
    return RandomColorMaps::GetRandomGroup();
  }

  return m_weights.GetRandomWeighted();
}

} // namespace GOOM::UTILS
