#include "goomutils/colormap.h"
#include "goomutils/math_utils.h"
#include "goomutils/all_maps.h"

#include <vivid/vivid.h>

#include <string>
#include <stdexcept>
#include <vector>

std::vector<const std::vector<std::string>*> ColorMaps::groups{};

ColorMaps::ColorMaps()
  : colorMaps{}
{
  for(const auto& [key, vividMap] : colordata::allMaps) {
    colorMaps.emplace(key, ColorMap{ vividMap });
    colorMapKeys.push_back(key);
  }
}

/**
const ColorMap& ColorMaps::at(const size_t i) const
{
  return colorMaps[i];
}
**/

const ColorMap& ColorMaps::getRandomColorMap() const
{
  const std::string& key = colorMapKeys[getRandInRange(0, colorMapKeys.size())];
  return colorMaps.at(key);
}

const ColorMap& ColorMaps::getRandomColorMap(const std::vector<std::string>& group) const
{
  const std::string& key = group[getRandInRange(0, group.size())];
  const auto iter = colorMaps.find(key);
  if (iter == colorMaps.end()) {
    throw std::runtime_error(std::string("Could not find color map key \"") + key + "\".");
  }
  return iter->second;
}

const ColorMap& ColorMaps::getColorMap(const std::string& name) const
{
  const auto iter = colorMaps.find(name);
  if (iter == colorMaps.end()) {
    throw std::runtime_error(std::string("Could not find color map key \"") + name + "\".");
  }
  return iter->second;
}

const std::vector<std::string>& ColorMaps::getRandomGroup()
{
  if (groups.size() == 0) {
    groups = {
      &colordata::sequentialsMaps,
      &colordata::sequentials2Maps,
      &colordata::divergingMaps,
      &colordata::diverging_blackMaps,
      &colordata::qualitativeMaps,
      &colordata::miscMaps,
      &colordata::ungroupedMaps,
    };
  }
  return *groups[getRandInRange(0, groups.size())];
}

const std::vector<std::string>& ColorMaps::getSequentialsMaps()
{
  return colordata::sequentialsMaps;
}

const std::vector<std::string>& ColorMaps::getSequentials2Maps()
{
  return colordata::sequentials2Maps;
}

const std::vector<std::string>& ColorMaps::getDivergingMaps()
{
  return colordata::divergingMaps;
}

const std::vector<std::string>& ColorMaps::getDiverging_blackMaps()
{
  return colordata::diverging_blackMaps;
}

const std::vector<std::string>& ColorMaps::getQualitativeMaps()
{
  return colordata::qualitativeMaps;
}

const std::vector<std::string>& ColorMaps::getMiscMaps()
{
  return colordata::miscMaps;
}

const std::vector<std::string>& ColorMaps::getUngroupedMaps()
{
  return colordata::ungroupedMaps;
}


/**
ColorMap::ColorMap(const vivid::ColorMap::Preset preset)
  : colors(NumColors)
{
//  logInfo(stdnew::format("preset = {}", preset));
  const vivid::ColorMap cmap(preset);
  for (size_t i=0 ; i < NumColors; i++) {
    const float t = i / (NumColors - 1.0f);
    const vivid::Color rgbcol{ cmap.at(t) };
    colors[i] = rgbcol.rgb32();
  }
}
**/

ColorMap::ColorMap(const vivid::ColorMap& cmap)
  : colors(NumColors)
{
  for (size_t i=0 ; i < NumColors; i++) {
    const float t = i / (NumColors - 1.0f);
    const vivid::Color rgbcol{ cmap.at(t) };
    colors[i] = rgbcol.rgb32();
  }
}

ColorMap::ColorMap(const ColorMap& other)
: colors(other.colors)
{
}

uint32_t ColorMap::getRandomColor(const ColorMap& cg)
{
  return cg.getColor(getRandInRange(0, cg.numColors()));
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
