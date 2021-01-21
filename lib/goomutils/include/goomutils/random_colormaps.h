#ifndef VISUALIZATION_GOOM_LIB_GOOMUTILS_RANDOM_COLORMAP_H
#define VISUALIZATION_GOOM_LIB_GOOMUTILS_RANDOM_COLORMAP_H

#include "goom/goom_graphic.h"
#include "goomutils/colordata/colormap_enums.h"
#include "goomutils/colormaps.h"
#include "goomutils/goomrand.h"

#include <array>
#include <memory>
#include <utility>
#include <vector>

namespace GOOM::UTILS
{

class RandomColorMaps : public ColorMaps
{
public:
  RandomColorMaps() noexcept;
  ~RandomColorMaps() noexcept override;
  RandomColorMaps(const RandomColorMaps&) noexcept = delete;
  RandomColorMaps(RandomColorMaps&&) noexcept = delete;
  auto operator=(const RandomColorMaps&) -> RandomColorMaps& = delete;
  auto operator=(RandomColorMaps&&) -> RandomColorMaps& = delete;

  [[nodiscard]] auto GetRandomColorMapName() const -> COLOR_DATA::ColorMapName;
  [[nodiscard]] auto GetRandomColorMapName(ColorMapGroup cmg) const -> COLOR_DATA::ColorMapName;

  [[nodiscard]] auto GetRandomColorMap() const -> const IColorMap&;
  [[nodiscard]] auto GetRandomColorMap(ColorMapGroup cmg) const -> const IColorMap&;

  [[nodiscard]] auto GetRandomColorMapPtr(bool includeRotatePoints = false) const
      -> std::shared_ptr<const IColorMap>;
  [[nodiscard]] auto GetRandomColorMapPtr(ColorMapGroup cmg, bool includeRotatePoints = false) const
      -> std::shared_ptr<const IColorMap>;

  [[nodiscard]] auto GetRandomTintedColorMapPtr() const -> std::shared_ptr<const IColorMap>;
  [[nodiscard]] auto GetRandomTintedColorMapPtr(ColorMapGroup cmg) const
      -> std::shared_ptr<const IColorMap>;
  [[nodiscard]] auto GetRandomTintedColorMapPtr(COLOR_DATA::ColorMapName cmName) const
      -> std::shared_ptr<const IColorMap>;

  [[nodiscard]] virtual auto GetRandomGroup() const -> ColorMapGroup;

  [[nodiscard]] static auto GetRandomColor(const IColorMap& colorMap, float t0, float t1) -> Pixel;
};

class WeightedColorMaps : public RandomColorMaps
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
