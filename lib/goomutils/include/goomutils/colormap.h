#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_

#include "goom/goom_graphic.h"
#include "goomutils/colordata/colormap_enums.h"
#include "goomutils/goomrand.h"

#include <array>
#include <memory>
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

  [[nodiscard]] size_t getNumStops() const { return cmap.numStops(); }
  [[nodiscard]] colordata::ColorMapName getMapName() const { return mapName; }
  [[nodiscard]] Pixel getColor(const float t) const
  {
    return Pixel{vivid::Color{cmap.at(t)}.rgb32()};
  }

  static Pixel getRandomColor(const ColorMap&, float t0 = 0, float t1 = 1);
  static Pixel colorMix(const Pixel& col1, const Pixel& col2, float t);
  static Pixel getLighterColor(const Pixel& color, int incPercent);

private:
  const colordata::ColorMapName mapName;
  const vivid::ColorMap cmap;
  ColorMap(colordata::ColorMapName, const vivid::ColorMap&);
  ColorMap(const ColorMap&);
  struct ColorMapAllocator : std::allocator<ColorMap>
  {
    template<class U, class... Args>
    void construct(U* p, Args&&... args)
    {
      ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
    }
    template<class U>
    struct rebind
    {
      using other = ColorMapAllocator;
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

  ColorMaps() noexcept;
  virtual ~ColorMaps() noexcept = default;
  ColorMaps(const ColorMaps&) = delete;
  ColorMaps& operator=(const ColorMaps&) = delete;

  [[nodiscard]] size_t getNumColorMaps() const;
  [[nodiscard]] const ColorMap& getColorMap(colordata::ColorMapName) const;
  [[nodiscard]] const ColorMapNames& getColorMapNames(ColorMapGroup) const;

  [[nodiscard]] colordata::ColorMapName getRandomColorMapName() const;
  [[nodiscard]] colordata::ColorMapName getRandomColorMapName(ColorMapGroup) const;

  [[nodiscard]] const ColorMap& getRandomColorMap() const;
  [[nodiscard]] const ColorMap& getRandomColorMap(ColorMapGroup) const;

  [[nodiscard]] size_t getNumGroups() const;
  [[nodiscard]] virtual ColorMapGroup getRandomGroup() const;

protected:
  using GroupColorNames =
      std::array<const ColorMapNames*, static_cast<size_t>(ColorMapGroup::_size)>;
  [[nodiscard]] const GroupColorNames& getGroups() const { return groups; }
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
  ~WeightedColorMaps() noexcept override = default;

  [[nodiscard]] const Weights<ColorMapGroup>& getWeights() const;
  void setWeights(const Weights<ColorMapGroup>&);

  [[nodiscard]] bool areWeightsActive() const;
  void setWeightsActive(bool value);

  [[nodiscard]] ColorMapGroup getRandomGroup() const override;

private:
  Weights<ColorMapGroup> weights;
  bool weightsActive;
};

} // namespace goom::utils
#endif /* LIBS_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_ */
