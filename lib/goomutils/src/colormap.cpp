#include "goomutils/colormap.h"

#include "goomutils/colormap_enums.h"
#include "goomutils/math_utils.h"
#include "goomutils/colordata/all_maps.h"

#include <vivid/vivid.h>

#include <string>
#include <vector>

std::vector<const std::vector<ColorMapName>*> ColorMaps::groups{};

ColorMaps::ColorMaps()
: colorMaps{}
{
  for(const auto& [name, vividMap] : colordata::allMaps) {
    colorMaps.push_back(ColorMap{ name, vividMap });
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
  return colorMaps[getRandInRange(0, colorMaps.size())];
}

const ColorMap& ColorMaps::getRandomColorMap(const std::vector<ColorMapName>& group) const
{
  return getColorMap(group[getRandInRange(0, group.size())]);
}

const ColorMap& ColorMaps::getColorMap(const ColorMapName name) const
{
  return colorMaps[static_cast<size_t>(name)];
}

size_t ColorMaps::getNumColorMaps() const
{
  return colordata::allMaps.size();
}

size_t ColorMaps::getNumGroups()
{
  initGroups();
  return groups.size();
}

const std::vector<ColorMapName>& ColorMaps::getRandomGroup()
{
  initGroups();
  return *groups[getRandInRange(0, groups.size())];
}

void ColorMaps::initGroups()
{
  if (groups.size() != 0) {
    return;
  }
  groups = {
    &colordata::perc_unif_sequentialMaps,
    &colordata::sequentialMaps,
    &colordata::sequential2Maps,
    &colordata::cyclicMaps,
    &colordata::divergingMaps,
    &colordata::diverging_blackMaps,
    &colordata::qualitativeMaps,
    &colordata::miscMaps,
  };
}

const std::vector<ColorMapName>& ColorMaps::getPerceptuallyUniformSequentialMaps()
{
  return colordata::perc_unif_sequentialMaps;
}

const std::vector<ColorMapName>& ColorMaps::getSequentialMaps()
{
  return colordata::sequentialMaps;
}

const std::vector<ColorMapName>& ColorMaps::getSequential2Maps()
{
  return colordata::sequential2Maps;
}

const std::vector<ColorMapName>& ColorMaps::getCyclicMaps()
{
  return colordata::cyclicMaps;
}

const std::vector<ColorMapName>& ColorMaps::getDivergingMaps()
{
  return colordata::divergingMaps;
}

const std::vector<ColorMapName>& ColorMaps::getDiverging_blackMaps()
{
  return colordata::diverging_blackMaps;
}

const std::vector<ColorMapName>& ColorMaps::getQualitativeMaps()
{
  return colordata::qualitativeMaps;
}

const std::vector<ColorMapName>& ColorMaps::getMiscMaps()
{
  return colordata::miscMaps;
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

ColorMap& ColorMap::operator=(const ColorMap& other)
{
  mapName = other.mapName;
  cmap = other.cmap;
  return *this;
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
