#include "goomutils/colormap.h"

#include "goomutils/colormap_enums.h"
#include "goomutils/math_utils.h"
#include "goomutils/colordata/all_maps.h"

#include <vivid/vivid.h>

#include <algorithm>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>

std::vector<ColorMap, ColorMap::ColorMapAllocator> ColorMaps::colorMaps{};
ColorMaps::GroupColorNames ColorMaps::groups{ nullptr };

ColorMaps::ColorMaps()
  : overrideColorMap{ nullptr }
  , overrideColorGroup{ ColorMapGroup::_null }
{
}

void ColorMaps::setOverrideColorMap(const ColorMap& cm)
{
  overrideColorMap = &cm;
}

void ColorMaps::setOverrideColorGroup(const ColorMapGroup grp)
{
  overrideColorGroup = grp;
}

const ColorMap& ColorMaps::getRandomColorMap() const
{
  if (overrideColorMap != nullptr) {
    return *overrideColorMap;
  }

  initColorMaps();

  return colorMaps[getRandInRange(0, colorMaps.size())];
}

const ColorMap& ColorMaps::getRandomColorMap(const ColorMapGroup groupName) const
{
  if (overrideColorMap != nullptr) {
    return *overrideColorMap;
  }

  initGroups();
  initColorMaps();

  const std::vector<ColorMapName>* group = at(groups, groupName);
  return getColorMap(group->at(getRandInRange(0, group->size())));
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
  if (colorMaps.size() != 0) {
    return;
  }
  colorMaps.reserve(colordata::allMaps.size());
  for(const auto& [name, vividMap] : colordata::allMaps) {
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
  if (overrideColorGroup != ColorMapGroup::_null) {
    return overrideColorGroup;
  }

  initGroups();
  return static_cast<ColorMapGroup>(getRandInRange(0u, static_cast<size_t>(ColorMapGroup::_size)));
}

void ColorMaps::initGroups()
{
  if (groups[0] != nullptr) {
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

WeightedColorMaps::WeightedColorMaps()
  : ColorMaps{}
  , weights{}
  , weightsActive{ true }
{
}

WeightedColorMaps::WeightedColorMaps(const Weights<ColorMapGroup>& w)
  : ColorMaps{}
  , weights{ w }
  , weightsActive{ true }
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
  if (getOverrideColorGroup() != ColorMapGroup::_null) {
    return getOverrideColorGroup();
  }
  if (!weightsActive) {
    return ColorMaps::getRandomGroup();
  }

  return weights.getRandomWeighted();
}

ColorMap::ColorMap(const std::string& mapNm, const vivid::ColorMap& cm)
  : mapName{ mapNm }
  , cmap{ cm }
{
}

ColorMap::ColorMap(const ColorMap& other)
  : mapName(other.mapName)
  , cmap(other.cmap)
{
}

uint32_t ColorMap::getRandomColor(const ColorMap& cg)
{
  return cg.getColor(getRandInRange(0.0f, 1.0f));
}

uint32_t ColorMap::colorMix(const uint32_t col1, const uint32_t col2, const float t)
{
  const vivid::rgb_t c1 = vivid::rgb::fromRgb32(col1);
  const vivid::rgb_t c2 = vivid::rgb::fromRgb32(col2);
  return vivid::lerpHsl(c1, c2, t).rgb32();
}

uint32_t ColorMap::getLighterColor(const uint32_t color, const int incPercent)
{
  vivid::hsl_t col = vivid::hsl::fromRgb(vivid::rgb::fromRgb32(color));
  col.z *= 1.0 + float(incPercent/100.0);
  if (col.z > 1.0) {
    col.z = 1.0;
  }

  return vivid::Color(col).rgb32();
}
