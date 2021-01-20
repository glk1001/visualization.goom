#ifndef VISUALIZATION_GOOM_LIB_GOOMUTILS_COLORMAPS_H
#define VISUALIZATION_GOOM_LIB_GOOMUTILS_COLORMAPS_H

#include "goom/goom_graphic.h"
#include "goomutils/colordata/colormap_enums.h"

#include <memory>

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

  [[nodiscard]] auto GetNumColorMapNames() const -> size_t;
  using ColorMapNames = std::vector<COLOR_DATA::ColorMapName>;
  [[nodiscard]] auto GetColorMapNames(ColorMapGroup cmg) const -> const ColorMapNames&;

  [[nodiscard]] auto GetColorMap(COLOR_DATA::ColorMapName mapName) const -> const IColorMap&;

  [[nodiscard]] auto GetColorMapPtr(COLOR_DATA::ColorMapName mapName, float tRotatePoint = 0) const
      -> std::shared_ptr<const IColorMap>;

  [[nodiscard]] auto GetNumGroups() const -> size_t;

private:
  class ColorMapsImpl;
  std::unique_ptr<ColorMapsImpl> m_colorMapsImpl;
};

} // namespace GOOM::UTILS
#endif
