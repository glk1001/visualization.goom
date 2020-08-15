#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_

#include "goomutils/colormap_enums.h"

#include <vivid/vivid.h>
#include <string>
#include <vector>


class ColorMap {
public:
  ColorMap(const ColorMap& other);
  ColorMap& operator=(const ColorMap& other);

  size_t getNumStops() const { return cmap.numStops(); }
  const std::string& getMapName() const { return mapName; }
  uint32_t getColor(const float t) const { return vivid::Color{ cmap.at(t) }.rgb32(); }

  static uint32_t getRandomColor(const ColorMap& cg);
  static uint32_t colorMix(const uint32_t col1, const uint32_t col2, const float t);
  static uint32_t getLighterColor(const uint32_t color, const int incPercent);
private:
  explicit ColorMap(const std::string& mapName, const vivid::ColorMap&);
  friend class ColorMaps;
  std::string mapName;
  vivid::ColorMap cmap;
};

class ColorMaps {
public:
  ColorMaps();

  size_t getNumColorMaps() const;
  static size_t getNumGroups();

  const ColorMap& getRandomColorMap() const;
  const ColorMap& getRandomColorMap(const std::vector<ColorMapName>& group) const;
  const ColorMap& getColorMap(const ColorMapName name) const;

    static const std::vector<ColorMapName>& getRandomGroup();
    static const std::vector<ColorMapName>& getPerceptuallyUniformSequentialMaps();
    static const std::vector<ColorMapName>& getSequentialMaps();
    static const std::vector<ColorMapName>& getSequential2Maps();
    static const std::vector<ColorMapName>& getCyclicMaps();
    static const std::vector<ColorMapName>& getDivergingMaps();
    static const std::vector<ColorMapName>& getDiverging_blackMaps();
    static const std::vector<ColorMapName>& getQualitativeMaps();
    static const std::vector<ColorMapName>& getMiscMaps();
private:
  std::vector<ColorMap> colorMaps;
  static std::vector<const std::vector<ColorMapName>*> groups;
  static void initGroups();
};

#endif /* LIBS_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_ */
