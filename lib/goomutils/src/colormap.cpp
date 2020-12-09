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

std::vector<ColorMap, ColorMap::ColorMapAllocator> ColorMaps::colorMaps{};
ColorMaps::GroupColorNames ColorMaps::groups{nullptr};

ColorMaps::ColorMaps() noexcept = default;

ColorMapName ColorMaps::getRandomColorMapName() const
{
  initColorMaps();

  return static_cast<ColorMapName>(getRandInRange(0u, colorMaps.size()));
}

ColorMapName ColorMaps::getRandomColorMapName(const ColorMapGroup groupName) const
{
  initGroups();
  initColorMaps();

  const std::vector<ColorMapName>* group = at(groups, groupName);
  return group->at(getRandInRange(0u, group->size()));
}

const ColorMap& ColorMaps::getRandomColorMap() const
{
  initColorMaps();

  return colorMaps[getRandInRange(0u, colorMaps.size())];
}

const ColorMap& ColorMaps::getRandomColorMap(const ColorMapGroup groupName) const
{
  initGroups();
  initColorMaps();

  const std::vector<ColorMapName>* group = at(groups, groupName);
  return getColorMap(group->at(getRandInRange(0u, group->size())));
}

const ColorMap& ColorMaps::getColorMap(const ColorMapName name) const
{
  initColorMaps();
  return colorMaps[static_cast<size_t>(name)];
}

const ColorMaps::ColorMapNames& ColorMaps::getColorMapNames(const ColorMapGroup groupName) const
{
  return *at(groups, groupName);
}

size_t ColorMaps::getNumColorMaps() const
{
  return colordata::allMaps.size();
}

void ColorMaps::initColorMaps()
{
  if (!colorMaps.empty())
  {
    return;
  }
  colorMaps.reserve(colordata::allMaps.size());
  for (const auto& [name, vividMap] : colordata::allMaps)
  {
    colorMaps.emplace_back(name, vividMap);
  }
}

size_t ColorMaps::getNumGroups() const
{
  initGroups();
  return groups.size();
}

ColorMapGroup ColorMaps::getRandomGroup() const
{
  initGroups();
  return static_cast<ColorMapGroup>(getRandInRange(0u, static_cast<size_t>(ColorMapGroup::_size)));
}

void ColorMaps::initGroups()
{
  if (groups[0] != nullptr)
  {
    return;
  }
  at(groups, ColorMapGroup::perceptuallyUniformSequential) = &colordata::perc_unif_sequentialMaps;
  at(groups, ColorMapGroup::sequential) = &colordata::sequentialMaps;
  at(groups, ColorMapGroup::sequential2) = &colordata::sequential2Maps;
  at(groups, ColorMapGroup::cyclic) = &colordata::cyclicMaps;
  at(groups, ColorMapGroup::diverging) = &colordata::divergingMaps;
  at(groups, ColorMapGroup::diverging_black) = &colordata::diverging_blackMaps;
  at(groups, ColorMapGroup::qualitative) = &colordata::qualitativeMaps;
  at(groups, ColorMapGroup::misc) = &colordata::miscMaps;
}

WeightedColorMaps::WeightedColorMaps() : ColorMaps{}, weights{}, weightsActive{true}
{
}

WeightedColorMaps::WeightedColorMaps(const Weights<ColorMapGroup>& w)
  : ColorMaps{}, weights{w}, weightsActive{true}
{
}

const Weights<ColorMapGroup>& WeightedColorMaps::getWeights() const
{
  return weights;
}

void WeightedColorMaps::setWeights(const Weights<ColorMapGroup>& w)
{
  weights = w;
}

bool WeightedColorMaps::areWeightsActive() const
{
  return weightsActive;
}

void WeightedColorMaps::setWeightsActive(const bool value)
{
  weightsActive = value;
}

ColorMapGroup WeightedColorMaps::getRandomGroup() const
{
  if (!weightsActive)
  {
    return ColorMaps::getRandomGroup();
  }

  return weights.getRandomWeighted();
}

ColorMap::ColorMap(const ColorMapName mapNm, const vivid::ColorMap& cm) : mapName{mapNm}, cmap{cm}
{
}

ColorMap::ColorMap(const ColorMap& other) = default;

Pixel ColorMap::getRandomColor(const ColorMap& cg, const float t0, const float t1)
{
  return cg.getColor(getRandInRange(t0, t1));
}

Pixel ColorMap::colorMix(const Pixel& col1, const Pixel& col2, const float t)
{
  const vivid::rgb_t c1 = vivid::rgb::fromRgb32(col1.rgba());
  const vivid::rgb_t c2 = vivid::rgb::fromRgb32(col2.rgba());
  return Pixel{vivid::lerpHsl(c1, c2, t).rgb32()};
}

Pixel ColorMap::getLighterColor(const Pixel& color, const int incPercent)
{
  vivid::hsl_t col = vivid::hsl::fromRgb(vivid::rgb::fromRgb32(color.rgba()));
  col.z *= 1.0 + static_cast<float>(incPercent / 100.0);
  if (col.z > 1.0)
  {
    col.z = 1.0;
  }

  return Pixel{vivid::Color(col).rgb32()};
}

} // namespace goom::utils
