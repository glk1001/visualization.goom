#ifndef VISUALIZATION_GOOM_COLORMAP_H
#define VISUALIZATION_GOOM_COLORMAP_H

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
  ~ColorMap() noexcept = default;
  auto operator=(const ColorMap&) -> ColorMap& = delete;
  auto operator=(const ColorMap&&) -> ColorMap& = delete;

  [[nodiscard]] auto GetNumStops() const -> size_t { return m_cmap.numStops(); }
  [[nodiscard]] auto GetMapName() const -> colordata::ColorMapName { return m_mapName; }
  [[nodiscard]] auto GetColor(const float t) const -> Pixel;

  auto GetRandomColor(float t0 = 0.0F, float t1 = 1.0F) const -> Pixel;
  static auto ColorMix(const Pixel& col1, const Pixel& col2, float t) -> Pixel;

private:
  const colordata::ColorMapName m_mapName;
  const vivid::ColorMap m_cmap;
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

inline auto ColorMap::GetColor(const float t) const -> Pixel
{
  return Pixel{vivid::Color{m_cmap.at(t)}.rgb32()};
}

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
  ColorMaps() noexcept;
  virtual ~ColorMaps() noexcept = default;
  ColorMaps(const ColorMaps&) = delete;
  ColorMaps(const ColorMaps&&) = delete;
  auto operator=(const ColorMaps&) -> ColorMaps& = delete;
  auto operator=(const ColorMaps&&) -> ColorMaps& = delete;

  [[nodiscard]] auto GetNumColorMaps() const -> size_t;
  [[nodiscard]] auto GetColorMap(colordata::ColorMapName) const -> const ColorMap&;
  using ColorMapNames = std::vector<colordata::ColorMapName>;
  [[nodiscard]] auto GetColorMapNames(ColorMapGroup) const -> const ColorMapNames&;

  [[nodiscard]] auto GetRandomColorMapName() const -> colordata::ColorMapName;
  [[nodiscard]] auto GetRandomColorMapName(ColorMapGroup) const -> colordata::ColorMapName;

  [[nodiscard]] auto GetRandomColorMap() const -> const ColorMap&;
  [[nodiscard]] auto GetRandomColorMap(ColorMapGroup) const -> const ColorMap&;

  [[nodiscard]] auto GetNumGroups() const -> size_t;
  [[nodiscard]] virtual auto GetRandomGroup() const -> ColorMapGroup;

protected:
  using GroupColorNames =
      std::array<const ColorMapNames*, static_cast<size_t>(ColorMapGroup::_size)>;
  [[nodiscard]] auto GetGroups() const -> const GroupColorNames& { return s_groups; }
  static void InitGroups();

private:
  static std::vector<ColorMap, ColorMap::ColorMapAllocator> s_colorMaps;
  static GroupColorNames s_groups;
  static void InitColorMaps();
};

class WeightedColorMaps : public ColorMaps
{
public:
  WeightedColorMaps();
  explicit WeightedColorMaps(const Weights<ColorMapGroup>&);
  ~WeightedColorMaps() noexcept override = default;
  WeightedColorMaps(const WeightedColorMaps&) noexcept = delete;
  WeightedColorMaps(const WeightedColorMaps&&) noexcept = delete;
  auto operator=(const WeightedColorMaps&) -> WeightedColorMaps& = delete;
  auto operator=(const WeightedColorMaps&&) -> WeightedColorMaps& = delete;

  [[nodiscard]] auto GetWeights() const -> const Weights<ColorMapGroup>&;
  void SetWeights(const Weights<ColorMapGroup>&);

  [[nodiscard]] auto AreWeightsActive() const -> bool;
  void SetWeightsActive(bool value);

  [[nodiscard]] auto GetRandomGroup() const -> ColorMapGroup override;

private:
  Weights<ColorMapGroup> m_weights;
  bool m_weightsActive;
};

} // namespace goom::utils
#endif /* LIBS_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_ */
