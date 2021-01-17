#ifndef VISUALIZATION_GOOM_COLORMAP_H
#define VISUALIZATION_GOOM_COLORMAP_H

#include "goom/goom_graphic.h"
#include "goomutils/colordata/colormap_enums.h"
#include "goomutils/goomrand.h"

#include <array>
#include <memory>
#include <utility>
#include <vector>

namespace goom::utils
{

class ColorMap
{
public:
  ColorMap() noexcept = default;
  virtual ~ColorMap() noexcept = default;
  ColorMap(const ColorMap&) noexcept = delete;
  auto operator=(const ColorMap&) -> ColorMap& = delete;
  auto operator=(const ColorMap&&) -> ColorMap& = delete;

  [[nodiscard]] virtual auto GetNumStops() const -> size_t = 0;
  [[nodiscard]] virtual auto GetMapName() const -> colordata::ColorMapName = 0;
  [[nodiscard]] virtual auto GetColor(float t) const -> Pixel = 0;

  [[nodiscard]] virtual auto GetRandomColor(float t0, float t1) const -> Pixel = 0;
  static auto GetColorMix(const Pixel& col1, const Pixel& col2, float t) -> Pixel;

private:
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
  ColorMaps() noexcept;
  virtual ~ColorMaps() noexcept;
  ColorMaps(const ColorMaps&) = delete;
  ColorMaps(const ColorMaps&&) = delete;
  auto operator=(const ColorMaps&) -> ColorMaps& = delete;
  auto operator=(const ColorMaps&&) -> ColorMaps& = delete;

  using ColorMapNames = std::vector<colordata::ColorMapName>;
  [[nodiscard]] auto GetColorMapNames(ColorMapGroup cmg) const -> const ColorMapNames&;

  [[nodiscard]] auto GetRandomColorMapName() const -> colordata::ColorMapName;
  [[nodiscard]] auto GetRandomColorMapName(ColorMapGroup cmg) const -> colordata::ColorMapName;

  [[nodiscard]] auto GetColorMap(colordata::ColorMapName mapName) const -> const ColorMap&;
  [[nodiscard]] auto GetRandomColorMap() const -> const ColorMap&;
  [[nodiscard]] auto GetRandomColorMap(ColorMapGroup cmg) const -> const ColorMap&;

  [[nodiscard]] auto GetColorMapPtr(colordata::ColorMapName mapName, float tRotatePoint = 0) const
      -> std::shared_ptr<const ColorMap>;
  [[nodiscard]] auto GetRandomColorMapPtr(bool includeRotatePoints = false) const
      -> std::shared_ptr<const ColorMap>;
  [[nodiscard]] auto GetRandomColorMapPtr(ColorMapGroup cmg, bool includeRotatePoints = false) const
      -> std::shared_ptr<const ColorMap>;

  [[nodiscard]] auto GetNumGroups() const -> size_t;
  [[nodiscard]] virtual auto GetRandomGroup() const -> ColorMapGroup;

private:
  class ColorMapsImpl;
  std::unique_ptr<ColorMapsImpl> m_colorMapImpl;
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
#endif
