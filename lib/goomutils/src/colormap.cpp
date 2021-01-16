#include "colormap.h"

#include "colordata/all_maps.h"
#include "colordata/colormap_enums.h"
#include "goom/goom_graphic.h"
#include "goomrand.h"

#include <vector>
#include <vivid/vivid.h>

namespace goom::utils
{

using colordata::ColorMapName;

std::vector<ColorMap, ColorMap::ColorMapAllocator> ColorMaps::s_colorMaps{};
ColorMaps::GroupColorNames ColorMaps::s_groups{nullptr};

ColorMaps::ColorMaps() noexcept = default;

auto ColorMaps::GetRandomColorMapName() const -> ColorMapName
{
  InitColorMaps();

  return static_cast<ColorMapName>(getRandInRange(0u, s_colorMaps.size()));
}

auto ColorMaps::GetRandomColorMapName(const ColorMapGroup groupName) const -> ColorMapName
{
  InitGroups();
  InitColorMaps();

  const std::vector<ColorMapName>* group = at(s_groups, groupName);
  return group->at(getRandInRange(0U, group->size()));
}

auto ColorMaps::GetRandomColorMap() const -> const ColorMap&
{
  InitColorMaps();

  return s_colorMaps[getRandInRange(0U, s_colorMaps.size())];
}

auto ColorMaps::GetRandomColorMap(const ColorMapGroup groupName) const -> const ColorMap&
{
  InitGroups();
  InitColorMaps();

  const std::vector<ColorMapName>* group = at(s_groups, groupName);
  return GetColorMap(group->at(getRandInRange(0U, group->size())));
}

auto ColorMaps::GetColorMap(const ColorMapName name) const -> const ColorMap&
{
  InitColorMaps();
  return s_colorMaps[static_cast<size_t>(name)];
}

auto ColorMaps::GetColorMapNames(const ColorMapGroup groupName) const -> const ColorMapNames&
{
  InitGroups();
  return *at(s_groups, groupName);
}

auto ColorMaps::GetNumColorMaps() const -> size_t
{
  return colordata::allMaps.size();
}

void ColorMaps::InitColorMaps()
{
  if (!s_colorMaps.empty())
  {
    return;
  }
  s_colorMaps.reserve(colordata::allMaps.size());
  for (const auto& [name, vividMap] : colordata::allMaps)
  {
    (void)s_colorMaps.emplace_back(name, vividMap);
  }
}

auto ColorMaps::GetNumGroups() const -> size_t
{
  InitGroups();
  return s_groups.size();
}

auto ColorMaps::GetRandomGroup() const -> ColorMapGroup
{
  InitGroups();
  return static_cast<ColorMapGroup>(getRandInRange(0U, static_cast<size_t>(ColorMapGroup::_size)));
}

void ColorMaps::InitGroups()
{
  if (s_groups[0] != nullptr)
  {
    return;
  }
  at(s_groups, ColorMapGroup::perceptuallyUniformSequential) = &colordata::perc_unif_sequentialMaps;
  at(s_groups, ColorMapGroup::sequential) = &colordata::sequentialMaps;
  at(s_groups, ColorMapGroup::sequential2) = &colordata::sequential2Maps;
  at(s_groups, ColorMapGroup::cyclic) = &colordata::cyclicMaps;
  at(s_groups, ColorMapGroup::diverging) = &colordata::divergingMaps;
  at(s_groups, ColorMapGroup::diverging_black) = &colordata::diverging_blackMaps;
  at(s_groups, ColorMapGroup::qualitative) = &colordata::qualitativeMaps;
  at(s_groups, ColorMapGroup::misc) = &colordata::miscMaps;
}

WeightedColorMaps::WeightedColorMaps() : ColorMaps{}, m_weights{}, m_weightsActive{true}
{
}

WeightedColorMaps::WeightedColorMaps(const Weights<ColorMapGroup>& w)
  : ColorMaps{}, m_weights{w}, m_weightsActive{true}
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
    return ColorMaps::GetRandomGroup();
  }

  return m_weights.getRandomWeighted();
}

ColorMap::ColorMap(const ColorMapName mapNm, const vivid::ColorMap& cm)
  : m_mapName{mapNm}, m_cmap{cm}
{
}

ColorMap::ColorMap(const ColorMap& other) = default;

auto ColorMap::GetRandomColor(const ColorMap& cg, const float t0, const float t1) -> Pixel
{
  return cg.GetColor(getRandInRange(t0, t1));
}

auto ColorMap::ColorMix(const Pixel& col1, const Pixel& col2, const float t) -> Pixel
{
  const vivid::rgb_t c1 = vivid::rgb::fromRgb32(col1.Rgba());
  const vivid::rgb_t c2 = vivid::rgb::fromRgb32(col2.Rgba());
  return Pixel{vivid::lerpHsl(c1, c2, t).rgb32()};
}

auto ColorMap::GetLighterColor(const Pixel& color, const int incPercent) -> Pixel
{
  vivid::hsl_t col = vivid::hsl::fromRgb(vivid::rgb::fromRgb32(color.Rgba()));
  col.z *= 1.0 + static_cast<float>(incPercent) / 100.0F;
  if (col.z > 1.0)
  {
    col.z = 1.0;
  }

  return Pixel{vivid::Color(col).rgb32()};
}

} // namespace goom::utils
