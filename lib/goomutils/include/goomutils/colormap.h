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

enum class ColorMapGroup: int {
  perceptuallyUniformSequential=0,
  sequential,
  sequential2,
  cyclic,
  diverging,
  diverging_black,
  qualitative,
  misc,
  size = misc+1
};
inline size_t to_int(const ColorMapGroup i) { return static_cast<size_t>(i); }

class ColorMaps {
public:
  ColorMaps();

  size_t getNumColorMaps() const;
  static size_t getNumGroups();

  const ColorMap& getRandomColorMap() const;
  const ColorMap& getRandomColorMap(const std::vector<ColorMapName>& group) const;
  const ColorMap& getColorMap(const ColorMapName name) const;

  void setRandomGroupWeights(const std::vector<size_t>& weights);
  const std::vector<ColorMapName>& getRandomWeightedGroup() const;

    static const std::vector<ColorMapName>& getRandomGroup();
    static const std::vector<ColorMapName>& getGroup(const ColorMapGroup);
private:
  std::vector<ColorMap> colorMaps;
  std::vector<size_t> weights;
  size_t sumOfWeights;
  static std::vector<const std::vector<ColorMapName>*> groups;
  static void initGroups();
};

#endif /* LIBS_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_ */
