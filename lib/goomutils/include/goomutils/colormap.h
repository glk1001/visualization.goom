#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_

#include "goomutils/colordata/colormap_enums.h"
#include "goomutils/goomrand.h"

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <vivid/vivid.h>

namespace goom::utils
{

class ColorMap
{
public:
  ColorMap() noexcept = delete;
  ColorMap& operator=(const ColorMap&) = delete;

  size_t getNumStops() const { return cmap.numStops(); }
  colordata::ColorMapName getMapName() const { return mapName; }
  uint32_t getColor(const float t) const { return vivid::Color{cmap.at(t)}.rgb32(); }

  static uint32_t getRandomColor(const ColorMap&, const float t0 = 0, const float t1 = 1);
  static uint32_t colorMix(const uint32_t col1, const uint32_t col2, const float t);
  static uint32_t getLighterColor(const uint32_t color, const int incPercent);

private:
  const colordata::ColorMapName mapName;
  const vivid::ColorMap cmap;
  ColorMap(const colordata::ColorMapName, const vivid::ColorMap&);
  ColorMap(const ColorMap&);
  struct ColorMapAllocator : std::allocator<ColorMap>
  {
    template<class U, class... Args>
    void construct(U* p, Args&&... args)
    {
      ::new ((void*)p) U(std::forward<Args>(args)...);
    }
    template<class U>
    struct rebind
    {
      typedef ColorMapAllocator other;
    };
  };
  friend class ColorMaps;
};

enum class ColorMapGroup : int
{
  _null = -1,
  perceptuallyUniformSequential = 0,
  sequential,
  sequential2,
  cyclic,
  diverging,
  diverging_black,
  qualitative,
  misc,
  _size // unused and marks last enum + 1
};

//constexpr size_t to_int(const ColorMapGroup i) { return static_cast<size_t>(i); }
template<class T>
constexpr T& at(std::array<T, static_cast<size_t>(ColorMapGroup::_size)>& arr,
                const ColorMapGroup idx)
{
  return arr[static_cast<size_t>(idx)];
}

class ColorMaps
{
public:
  using ColorMapNames = std::vector<colordata::ColorMapName>;

  ColorMaps();
  ColorMaps(const ColorMaps&) = delete;
  virtual ~ColorMaps() noexcept = default;

  ColorMaps& operator=(const ColorMaps&) = delete;

  size_t getNumColorMaps() const;
  const ColorMap& getColorMap(const colordata::ColorMapName) const;
  const ColorMapNames& getColorMapNames(const ColorMapGroup) const;

  colordata::ColorMapName getRandomColorMapName() const;
  colordata::ColorMapName getRandomColorMapName(const ColorMapGroup) const;

  const ColorMap& getRandomColorMap() const;
  const ColorMap& getRandomColorMap(const ColorMapGroup) const;

  size_t getNumGroups() const;
  virtual ColorMapGroup getRandomGroup() const;

protected:
  using GroupColorNames =
      std::array<const ColorMapNames*, static_cast<size_t>(ColorMapGroup::_size)>;
  const GroupColorNames& getGroups() const { return groups; }
  static void initGroups();

private:
  static std::vector<ColorMap, ColorMap::ColorMapAllocator> colorMaps;
  static GroupColorNames groups;
  static void initColorMaps();
};

class WeightedColorMaps : public ColorMaps
{
public:
  WeightedColorMaps();
  explicit WeightedColorMaps(const Weights<ColorMapGroup>&);
  virtual ~WeightedColorMaps() noexcept override = default;

  const Weights<ColorMapGroup>& getWeights() const;
  void setWeights(const Weights<ColorMapGroup>&);

  bool areWeightsActive() const;
  void setWeightsActive(const bool value);

  ColorMapGroup getRandomGroup() const override;

private:
  Weights<ColorMapGroup> weights;
  bool weightsActive;
};

} // namespace goom::utils
#endif /* LIBS_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_ */
