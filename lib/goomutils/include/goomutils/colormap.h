#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_

#include <vivid/vivid.h>
#include <map>
#include <string>
#include <vector>


class ColorMap {
public:
  explicit ColorMap(const vivid::ColorMap&);
  ColorMap(const ColorMap& other);
  ColorMap& operator=(const ColorMap& other) { colors = other.colors; return *this; }

  size_t numColors() const { return colors.size(); }
  uint32_t getColor(const size_t i) const { return colors[i]; }

  static uint32_t getRandomColor(const ColorMap& cg);
  static uint32_t colorMix(const uint32_t col1, const uint32_t col2, const float t);
  static uint32_t getLighterColor(const uint32_t color, const int incPercent);
private:
  static constexpr size_t NumColors = 720;
  std::vector<uint32_t> colors;
};

class ColorMaps {
public:
  ColorMaps();
  const ColorMap& getRandomColorMap() const;
  const ColorMap& getRandomColorMap(const std::vector<std::string>& group) const;
  const ColorMap& getColorMap(const std::string& name) const;

    static const std::vector<std::string>& getRandomGroup();
    static const std::vector<std::string>& getSequentialsMaps();
    static const std::vector<std::string>& getSequentials2Maps();
    static const std::vector<std::string>& getDivergingMaps();
    static const std::vector<std::string>& getDiverging_blackMaps();
    static const std::vector<std::string>& getQualitativeMaps();
    static const std::vector<std::string>& getMiscMaps();
    static const std::vector<std::string>& getUngroupedMaps();
private:
  std::vector<std::string> colorMapKeys;
  std::map<std::string, ColorMap> colorMaps;
  static std::vector<const std::vector<std::string>*> groups;
};

#endif /* LIBS_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_ */
