#include "colormaps.h"

#include "colordata/all_maps.h"
#include "colordata/colormap_enums.h"
#include "goom/goom_graphic.h"

#include <format>
#include <utility>
#include <vector>
#include <vivid/vivid.h>

namespace GOOM::UTILS
{

using COLOR_DATA::ColorMapName;

class RotatedColorMap : public ColorMapWrapper
{
public:
  explicit RotatedColorMap(const std::shared_ptr<const IColorMap>& cm, float tRotatePoint);

  [[nodiscard]] auto GetColor(float t) const -> Pixel override;

private:
  static constexpr float MIN_ROTATE_POINT = 0.0F;
  static constexpr float MAX_ROTATE_POINT = 1.0F;
  const float m_tRotatePoint;
};

RotatedColorMap::RotatedColorMap(const std::shared_ptr<const IColorMap>& cm,
                                 const float tRotatePoint)
  : ColorMapWrapper{cm}, m_tRotatePoint{tRotatePoint}
{
  if (tRotatePoint < MIN_ROTATE_POINT)
  {
    throw std::logic_error(
        std20::format("Invalid rotate point {} < {}.", tRotatePoint, MIN_ROTATE_POINT));
  }
  if (tRotatePoint > MAX_ROTATE_POINT)
  {
    throw std::logic_error(
        std20::format("Invalid rotate point {} > {}.", tRotatePoint, MAX_ROTATE_POINT));
  }
}

inline auto RotatedColorMap::GetColor(const float t) const -> Pixel
{
  float tNew = m_tRotatePoint + t;
  if (tNew > 1.0F)
  {
    tNew = 1.0F - (tNew - 1.0F);
  }
  return GetColorMap()->GetColor(tNew);
}

class TintedColorMap : public ColorMapWrapper
{
public:
  TintedColorMap(const std::shared_ptr<const IColorMap>& cm, float lightness);

  [[nodiscard]] auto GetColor(float t) const -> Pixel override;

private:
  static constexpr float MIN_LIGHTNESS = 0.1F;
  static constexpr float MAX_LIGHTNESS = 1.0F;
  const float m_lightness;
};

TintedColorMap::TintedColorMap(const std::shared_ptr<const IColorMap>& cm, const float lightness)
  : ColorMapWrapper{cm}, m_lightness{lightness}
{
  if (lightness < MIN_LIGHTNESS)
  {
    throw std::logic_error(std20::format("Invalid lightness {} < {}.", lightness, MIN_LIGHTNESS));
  }
  if (lightness > MAX_LIGHTNESS)
  {
    throw std::logic_error(std20::format("Invalid lightness {} > {}.", lightness, MAX_LIGHTNESS));
  }
}

auto TintedColorMap::GetColor(const float t) const -> Pixel
{
  const Pixel color = GetColorMap()->GetColor(t);

  vivid::hsv_t hsv{vivid::rgb::fromRgb32(color.Rgba())};
  hsv[2] = m_lightness;

  return Pixel{vivid::Color{hsv}.rgb32()};
}

class PrebuiltColorMap : public IColorMap
{
public:
  struct ColorMapAllocator : std::allocator<PrebuiltColorMap>
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

  PrebuiltColorMap() noexcept = delete;
  PrebuiltColorMap(COLOR_DATA::ColorMapName mapNm, vivid::ColorMap cm);
  ~PrebuiltColorMap() noexcept override = default;
  PrebuiltColorMap(const PrebuiltColorMap&) noexcept = delete;
  auto operator=(const PrebuiltColorMap&) -> PrebuiltColorMap& = delete;
  auto operator=(PrebuiltColorMap&&) -> PrebuiltColorMap& = delete;

  [[nodiscard]] auto GetNumStops() const -> size_t override { return m_cmap.numStops(); }
  [[nodiscard]] auto GetMapName() const -> COLOR_DATA::ColorMapName override { return m_mapName; }
  [[nodiscard]] auto GetColor(float t) const -> Pixel override;

  static auto GetColorMix(const Pixel& col1, const Pixel& col2, float t) -> Pixel;

private:
  PrebuiltColorMap(PrebuiltColorMap&& other) noexcept;
  COLOR_DATA::ColorMapName m_mapName;
  const vivid::ColorMap m_cmap;
};

class ColorMaps::ColorMapsImpl
{
public:
  ColorMapsImpl() noexcept;
  virtual ~ColorMapsImpl() noexcept = default;
  ColorMapsImpl(const ColorMapsImpl&) noexcept = delete;
  ColorMapsImpl(ColorMapsImpl&&) noexcept = delete;
  auto operator=(const ColorMapsImpl&) -> ColorMapsImpl& = delete;
  auto operator=(ColorMapsImpl&&) -> const ColorMapsImpl&& = delete;

  [[nodiscard]] auto GetNumColorMapNames() const -> size_t;
  using ColorMapNames = std::vector<COLOR_DATA::ColorMapName>;
  [[nodiscard]] static auto GetColorMapNames(ColorMapGroup groupName) -> const ColorMapNames&;

  [[nodiscard]] static auto GetColorMap(COLOR_DATA::ColorMapName mapName) -> const IColorMap&;

  [[nodiscard]] auto GetColorMapPtr(COLOR_DATA::ColorMapName mapName, float tRotatePoint) const
      -> std::shared_ptr<const IColorMap>;
  [[nodiscard]] auto GetTintedColorMapPtr(const std::shared_ptr<const IColorMap>& cm,
                                          float lightness) const
      -> std::shared_ptr<const IColorMap>;

  [[nodiscard]] static auto GetNumGroups() -> size_t;

protected:
  using GroupColorNames =
      std::array<const ColorMapNames*, static_cast<size_t>(ColorMapGroup::_SIZE)>;
  [[nodiscard]] static auto GetGroups() -> const GroupColorNames& { return s_groups; }
  static void InitGroups();

private:
  static std::vector<PrebuiltColorMap, PrebuiltColorMap::ColorMapAllocator> s_preBuiltColorMaps;
  static GroupColorNames s_groups;
  static void InitPrebuiltColorMaps();
};

auto IColorMap::GetColorMix(const Pixel& col1, const Pixel& col2, float t) -> Pixel
{
  return PrebuiltColorMap::GetColorMix(col1, col2, t);
}

ColorMaps::ColorMaps() noexcept : m_colorMapsImpl{std::make_unique<ColorMapsImpl>()}
{
}

ColorMaps::~ColorMaps() noexcept = default;

auto ColorMaps::GetNumColorMapNames() const -> size_t
{
  return m_colorMapsImpl->GetNumColorMapNames();
}

auto ColorMaps::GetColorMapNames(ColorMapGroup cmg) const -> const ColorMaps::ColorMapNames&
{
  return m_colorMapsImpl->GetColorMapNames(cmg);
}

auto ColorMaps::GetColorMap(COLOR_DATA::ColorMapName mapName) const -> const IColorMap&
{
  return m_colorMapsImpl->GetColorMap(mapName);
}

auto ColorMaps::GetTintedColorMapPtr(const std::shared_ptr<const IColorMap>& cm,
                                     const float tintValue) const
    -> std::shared_ptr<const IColorMap>
{
  return m_colorMapsImpl->GetTintedColorMapPtr(cm, tintValue);
}

auto ColorMaps::GetColorMapPtr(COLOR_DATA::ColorMapName mapName, const float tRotatePoint) const
    -> std::shared_ptr<const IColorMap>
{
  return m_colorMapsImpl->GetColorMapPtr(mapName, tRotatePoint);
}

auto ColorMaps::GetNumGroups() const -> size_t
{
  return m_colorMapsImpl->GetNumGroups();
}

std::vector<PrebuiltColorMap, PrebuiltColorMap::ColorMapAllocator>
    ColorMaps::ColorMapsImpl::s_preBuiltColorMaps{};
ColorMaps::ColorMapsImpl::GroupColorNames ColorMaps::ColorMapsImpl::s_groups{};

ColorMaps::ColorMapsImpl::ColorMapsImpl() noexcept = default;

auto ColorMaps::ColorMapsImpl::GetColorMap(const ColorMapName name) -> const IColorMap&
{
  InitPrebuiltColorMaps();
  return s_preBuiltColorMaps[static_cast<size_t>(name)];
}

auto ColorMaps::ColorMapsImpl::GetColorMapPtr(COLOR_DATA::ColorMapName mapName,
                                              const float tRotatePoint) const
    -> std::shared_ptr<const IColorMap>
{
  const auto makeSharedOfAddr = [&](const IColorMap* cm) {
    return std::shared_ptr<const IColorMap>{cm, []([[maybe_unused]] const IColorMap* cm) {}};
  };

  if (tRotatePoint > 0.0F)
  {
    return makeSharedOfAddr(&GetColorMap(mapName));
  }
  return std::make_shared<RotatedColorMap>(makeSharedOfAddr(&GetColorMap(mapName)), tRotatePoint);
}

auto ColorMaps::ColorMapsImpl::GetTintedColorMapPtr(const std::shared_ptr<const IColorMap>& cm,
                                                    const float lightness) const
    -> std::shared_ptr<const IColorMap>
{
  return std::make_shared<TintedColorMap>(cm, lightness);
}

auto ColorMaps::ColorMapsImpl::GetNumColorMapNames() const -> size_t
{
  InitGroups();
  InitPrebuiltColorMaps();
  return s_preBuiltColorMaps.size();
}

auto ColorMaps::ColorMapsImpl::GetColorMapNames(const ColorMapGroup groupName)
    -> const ColorMapNames&
{
  InitGroups();
  return *at(s_groups, groupName);
}

void ColorMaps::ColorMapsImpl::InitPrebuiltColorMaps()
{
  if (!s_preBuiltColorMaps.empty())
  {
    return;
  }
  s_preBuiltColorMaps.reserve(COLOR_DATA::allMaps.size());
  for (const auto& [name, vividMap] : COLOR_DATA::allMaps)
  {
    (void)s_preBuiltColorMaps.emplace_back(name, vividMap);
  }
}

auto ColorMaps::ColorMapsImpl::GetNumGroups() -> size_t
{
  InitGroups();
  return s_groups.size();
}

void ColorMaps::ColorMapsImpl::InitGroups()
{
  if (s_groups[0] != nullptr)
  {
    return;
  }
  at(s_groups, ColorMapGroup::PERCEPTUALLY_UNIFORM_SEQUENTIAL) =
      &COLOR_DATA::perc_unif_sequentialMaps;
  at(s_groups, ColorMapGroup::SEQUENTIAL) = &COLOR_DATA::sequentialMaps;
  at(s_groups, ColorMapGroup::SEQUENTIAL2) = &COLOR_DATA::sequential2Maps;
  at(s_groups, ColorMapGroup::CYCLIC) = &COLOR_DATA::cyclicMaps;
  at(s_groups, ColorMapGroup::DIVERGING) = &COLOR_DATA::divergingMaps;
  at(s_groups, ColorMapGroup::DIVERGING_BLACK) = &COLOR_DATA::diverging_blackMaps;
  at(s_groups, ColorMapGroup::QUALITATIVE) = &COLOR_DATA::qualitativeMaps;
  at(s_groups, ColorMapGroup::MISC) = &COLOR_DATA::miscMaps;
}

PrebuiltColorMap::PrebuiltColorMap(const ColorMapName mapNm, vivid::ColorMap cm)
  : m_mapName{mapNm}, m_cmap{std::move(cm)}
{
}

PrebuiltColorMap::PrebuiltColorMap(PrebuiltColorMap&& other) noexcept
  : m_mapName{other.m_mapName}, m_cmap{other.m_cmap}
{
}

auto PrebuiltColorMap::GetColor(const float t) const -> Pixel
{
  return Pixel{vivid::Color{m_cmap.at(t)}.rgb32()};
}

auto PrebuiltColorMap::GetColorMix(const Pixel& col1, const Pixel& col2, float t) -> Pixel
{
  const vivid::rgb_t c1 = vivid::rgb::fromRgb32(col1.Rgba());
  const vivid::rgb_t c2 = vivid::rgb::fromRgb32(col2.Rgba());
  return Pixel{vivid::lerpHsl(c1, c2, t).rgb32()};
}

} // namespace GOOM::UTILS
