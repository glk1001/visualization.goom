#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_

#include "goomutils/colormap_enums.h"

#include <vivid/vivid.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>


class ColorMap {
public:
  ColorMap() noexcept=delete;
  ColorMap& operator=(const ColorMap&)=delete;

  size_t getNumStops() const { return cmap.numStops(); }
  const std::string& getMapName() const { return mapName; }
  uint32_t getColor(const float t) const { return vivid::Color{ cmap.at(t) }.rgb32(); }

  static uint32_t getRandomColor(const ColorMap&);
  static uint32_t colorMix(const uint32_t col1, const uint32_t col2, const float t);
  static uint32_t getLighterColor(const uint32_t color, const int incPercent);
private:
  const std::string mapName;
  const vivid::ColorMap cmap;
  ColorMap(const std::string& mapName, const vivid::ColorMap&);
  ColorMap(const ColorMap&);
  struct ColorMapAllocator: std::allocator<ColorMap> {
    template <class U, class... Args>
      void construct(U* p, Args&&... args) { ::new((void *)p)U(std::forward<Args>(args)...); }
    template <class U> struct rebind { typedef ColorMapAllocator other; };
  };
  friend class ColorMaps;
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

  static size_t getNumColorMaps();
  static size_t getNumGroups();

    static const ColorMap& getRandomColorMap();
    static const ColorMap& getRandomColorMap(const std::vector<ColorMapName> &group);
    static const ColorMap& getColorMap(const ColorMapName name);

  void setRandomGroupWeights(const std::vector<size_t>& weights);
  const std::vector<ColorMapName>& getRandomWeightedGroup() const;

    static const std::vector<ColorMapName>& getRandomGroup();
    static const std::vector<ColorMapName>& getGroup(const ColorMapGroup);
private:
  std::vector<size_t> weights;
  size_t sumOfWeights;
  static std::vector<ColorMap, ColorMap::ColorMapAllocator> colorMaps;
  static std::vector<const std::vector<ColorMapName>*> groups;
  static void initColorMaps();
  static void initGroups();
};

#endif /* LIBS_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_ */
