#ifndef VISUALIZATION_GOOM_LIB_GOOMUTILS_COLORMAP_H
#define VISUALIZATION_GOOM_LIB_GOOMUTILS_COLORMAP_H

#include "goom/goom_graphic.h"
#include "goomutils/colordata/colormap_enums.h"
#include "goomutils/goomrand.h"

#include <array>
#include <memory>
#include <utility>
#include <vector>

namespace GOOM::UTILS
{

class IColorMap
{
public:
  IColorMap() noexcept = default;
  virtual ~IColorMap() noexcept = default;
  IColorMap(const IColorMap&) noexcept = delete;
  IColorMap(IColorMap&&) noexcept = delete;
  auto operator=(const IColorMap&) -> IColorMap& = delete;
  auto operator=(IColorMap&&) -> IColorMap& = delete;

  [[nodiscard]] virtual auto GetNumStops() const -> size_t = 0;
  [[nodiscard]] virtual auto GetMapName() const -> COLOR_DATA::ColorMapName = 0;
  [[nodiscard]] virtual auto GetColor(float t) const -> Pixel = 0;

  [[nodiscard]] virtual auto GetRandomColor(float t0, float t1) const -> Pixel = 0;
  static auto GetColorMix(const Pixel& col1, const Pixel& col2, float t) -> Pixel;

private:
  friend class ColorMaps;
};

enum class ColorMapGroup : int
{
  _NULL = -1,
  PERCEPTUALLY_UNIFORM_SEQUENTIAL = 0,
  SEQUENTIAL,
  SEQUENTIAL2,
  CYCLIC,
  DIVERGING,
  DIVERGING_BLACK,
  QUALITATIVE,
  MISC,
  _SIZE // unused and marks last enum + 1
};

//constexpr size_t to_int(const ColorMapGroup i) { return static_cast<size_t>(i); }
template<class T>
constexpr T& at(std::array<T, static_cast<size_t>(ColorMapGroup::_SIZE)>& arr,
                const ColorMapGroup idx)
{
  return arr[static_cast<size_t>(idx)];
}

class ColorMaps
{
public:
  ColorMaps() noexcept;
  virtual ~ColorMaps() noexcept;
  ColorMaps(const ColorMaps&) noexcept = delete;
  ColorMaps(ColorMaps&&) noexcept = delete;
  auto operator=(const ColorMaps&) -> ColorMaps& = delete;
  auto operator=(ColorMaps&&) -> ColorMaps& = delete;

  using ColorMapNames = std::vector<COLOR_DATA::ColorMapName>;
  [[nodiscard]] auto GetColorMapNames(ColorMapGroup cmg) const -> const ColorMapNames&;

  [[nodiscard]] auto GetRandomColorMapName() const -> COLOR_DATA::ColorMapName;
  [[nodiscard]] auto GetRandomColorMapName(ColorMapGroup cmg) const -> COLOR_DATA::ColorMapName;

  [[nodiscard]] auto GetColorMap(COLOR_DATA::ColorMapName mapName) const -> const IColorMap&;
  [[nodiscard]] auto GetRandomColorMap() const -> const IColorMap&;
  [[nodiscard]] auto GetRandomColorMap(ColorMapGroup cmg) const -> const IColorMap&;

  [[nodiscard]] auto GetColorMapPtr(COLOR_DATA::ColorMapName mapName, float tRotatePoint = 0) const
      -> std::shared_ptr<const IColorMap>;
  [[nodiscard]] auto GetRandomColorMapPtr(bool includeRotatePoints = false) const
      -> std::shared_ptr<const IColorMap>;
  [[nodiscard]] auto GetRandomColorMapPtr(ColorMapGroup cmg, bool includeRotatePoints = false) const
      -> std::shared_ptr<const IColorMap>;

  [[nodiscard]] auto GetNumGroups() const -> size_t;
  [[nodiscard]] virtual auto GetRandomGroup() const -> ColorMapGroup;

private:
  class ColorMapsImpl;
  std::unique_ptr<ColorMapsImpl> m_colorMapsImpl;
};

class WeightedColorMaps : public ColorMaps
{
public:
  WeightedColorMaps();
  explicit WeightedColorMaps(const Weights<ColorMapGroup>&);
  ~WeightedColorMaps() noexcept override = default;
  WeightedColorMaps(const WeightedColorMaps&) noexcept = delete;
  WeightedColorMaps(WeightedColorMaps&&) noexcept = delete;
  auto operator=(const WeightedColorMaps&) -> WeightedColorMaps& = delete;
  auto operator=(WeightedColorMaps&&) -> WeightedColorMaps& = delete;

  [[nodiscard]] auto GetWeights() const -> const Weights<ColorMapGroup>&;
  void SetWeights(const Weights<ColorMapGroup>&);

  [[nodiscard]] auto AreWeightsActive() const -> bool;
  void SetWeightsActive(bool value);

  [[nodiscard]] auto GetRandomGroup() const -> ColorMapGroup override;

private:
  Weights<ColorMapGroup> m_weights;
  bool m_weightsActive;
};

} // namespace GOOM::UTILS
#endif
