#include "colormap.h"

#include "colordata/all_maps.h"
#include "colordata/colormap_enums.h"
#include "goom/goom_graphic.h"
#include "goomrand.h"

#include <utility>
#include <vector>
#include <vivid/vivid.h>

namespace goom::utils
{

using colordata::ColorMapName;

class ColorMapWrapper : public ColorMap
{
public:
  ColorMapWrapper() noexcept = delete;
  explicit ColorMapWrapper(const ColorMap& cm) noexcept;
  ~ColorMapWrapper() noexcept override = default;
  ColorMapWrapper(const ColorMapWrapper&) noexcept = delete;
  auto operator=(const ColorMapWrapper&) -> ColorMapWrapper& = delete;
  auto operator=(const ColorMapWrapper&&) -> ColorMapWrapper& = delete;

  [[nodiscard]] auto GetNumStops() const -> size_t override;
  [[nodiscard]] auto GetMapName() const -> colordata::ColorMapName override;
  [[nodiscard]] auto GetColor(float t) const -> Pixel override;

  [[nodiscard]] auto GetRandomColor(float t0, float t1) const -> Pixel override;

protected:
  [[nodiscard]] auto GetColorMap() const -> const ColorMap& { return m_colorMap; }

private:
  const ColorMap& m_colorMap;
};

ColorMapWrapper::ColorMapWrapper(const ColorMap& cm) noexcept : m_colorMap{cm}
{
}

inline auto ColorMapWrapper::GetNumStops() const -> size_t
{
  return m_colorMap.GetNumStops();
}

inline auto ColorMapWrapper::GetMapName() const -> colordata::ColorMapName
{
  return m_colorMap.GetMapName();
}

inline auto ColorMapWrapper::GetColor(float t) const -> Pixel
{
  return m_colorMap.GetColor(t);
}

inline auto ColorMapWrapper::GetRandomColor(float t0, float t1) const -> Pixel
{
  return m_colorMap.GetRandomColor(t0, t1);
}

class RotatedColorMap : public ColorMapWrapper
{
public:
  explicit RotatedColorMap(const ColorMap& cm, float tRotatePoint);

  [[nodiscard]] auto GetColor(float t) const -> Pixel override;

private:
  static constexpr float MIN_ROTATE_POINT = 0.0F;
  static constexpr float MAX_ROTATE_POINT = 1.0F;
  const float m_tRotatePoint;
};

RotatedColorMap::RotatedColorMap(const ColorMap& cm, const float tRotatePoint)
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
    tNew = 1.0F - tNew;
  }
  return GetColorMap().GetColor(tNew);
}

class PrebuiltColorMap : public ColorMap
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
  PrebuiltColorMap(colordata::ColorMapName mapNm, vivid::ColorMap cm);
  ~PrebuiltColorMap() noexcept override = default;
  PrebuiltColorMap(const PrebuiltColorMap&) = delete;
  auto operator=(const PrebuiltColorMap&) -> PrebuiltColorMap& = delete;
  auto operator=(const PrebuiltColorMap&&) -> PrebuiltColorMap& = delete;

  [[nodiscard]] auto GetNumStops() const -> size_t override { return m_cmap.numStops(); }
  [[nodiscard]] auto GetMapName() const -> colordata::ColorMapName override { return m_mapName; }
  [[nodiscard]] auto GetColor(float t) const -> Pixel override;

  [[nodiscard]] auto GetRandomColor(float t0, float t1) const -> Pixel override;
  static auto GetColorMix(const Pixel& col1, const Pixel& col2, float t) -> Pixel;

private:
  PrebuiltColorMap(PrebuiltColorMap&& other) noexcept;
  colordata::ColorMapName m_mapName;
  const vivid::ColorMap m_cmap;
};

class ColorMaps::ColorMapsImpl
{
public:
  ColorMapsImpl() noexcept;
  virtual ~ColorMapsImpl() noexcept = default;
  ColorMapsImpl(const ColorMaps&) = delete;
  ColorMapsImpl(const ColorMaps&&) = delete;
  auto operator=(const ColorMapsImpl&) -> ColorMapsImpl& = delete;
  auto operator=(const ColorMapsImpl&&) -> const ColorMapsImpl&& = delete;

  using ColorMapNames = std::vector<colordata::ColorMapName>;
  [[nodiscard]] static auto GetColorMapNames(ColorMapGroup groupName) -> const ColorMapNames&;

  [[nodiscard]] static auto GetRandomColorMapName() -> colordata::ColorMapName;
  [[nodiscard]] static auto GetRandomColorMapName(ColorMapGroup groupName)
      -> colordata::ColorMapName;

  [[nodiscard]] static auto GetColorMap(colordata::ColorMapName mapName) -> const ColorMap&;
  [[nodiscard]] static auto GetRandomColorMap() -> const ColorMap&;
  [[nodiscard]] static auto GetRandomColorMap(ColorMapGroup groupName) -> const ColorMap&;

  [[nodiscard]] auto GetColorMapPtr(colordata::ColorMapName mapName, float tRotatePoint) const
      -> std::shared_ptr<const ColorMap>;
  [[nodiscard]] auto GetRandomColorMapPtr(bool includeRotatePoints) const
      -> std::shared_ptr<const ColorMap>;
  [[nodiscard]] auto GetRandomColorMapPtr(ColorMapGroup cmg, bool includeRotatePoints) const
      -> std::shared_ptr<const ColorMap>;

  [[nodiscard]] static auto GetNumGroups() -> size_t;
  [[nodiscard]] virtual auto GetRandomGroup() const -> ColorMapGroup;

protected:
  using GroupColorNames =
      std::array<const ColorMapNames*, static_cast<size_t>(ColorMapGroup::_size)>;
  [[nodiscard]] static auto GetGroups() -> const GroupColorNames& { return s_groups; }
  static void InitGroups();

private:
  static std::vector<PrebuiltColorMap, PrebuiltColorMap::ColorMapAllocator> s_preBuiltColorMaps;
  static GroupColorNames s_groups;
  static void InitPrebuiltColorMaps();
};

auto ColorMap::GetColorMix(const Pixel& col1, const Pixel& col2, float t) -> Pixel
{
  return PrebuiltColorMap::GetColorMix(col1, col2, t);
}

ColorMaps::ColorMaps() noexcept : m_colorMapImpl{std::make_unique<ColorMapsImpl>()}
{
}

ColorMaps::~ColorMaps() noexcept = default;

auto ColorMaps::GetColorMapNames(ColorMapGroup cmg) const -> const ColorMaps::ColorMapNames&
{
  return m_colorMapImpl->GetColorMapNames(cmg);
}

auto ColorMaps::GetRandomColorMapName() const -> colordata::ColorMapName
{
  return m_colorMapImpl->GetRandomColorMapName();
}

auto ColorMaps::GetRandomColorMapName(ColorMapGroup cmg) const -> colordata::ColorMapName
{
  return m_colorMapImpl->GetRandomColorMapName(cmg);
}

auto ColorMaps::GetColorMap(colordata::ColorMapName mapName) const -> const ColorMap&
{
  return m_colorMapImpl->GetColorMap(mapName);
}

auto ColorMaps::GetRandomColorMap() const -> const ColorMap&
{
  return m_colorMapImpl->GetRandomColorMap();
}

auto ColorMaps::GetRandomColorMap(ColorMapGroup cmg) const -> const ColorMap&
{
  return m_colorMapImpl->GetRandomColorMap(cmg);
}

auto ColorMaps::GetColorMapPtr(colordata::ColorMapName mapName, const float tRotatePoint) const
    -> std::shared_ptr<const ColorMap>
{
  return m_colorMapImpl->GetColorMapPtr(mapName, tRotatePoint);
}

auto ColorMaps::GetRandomColorMapPtr(const bool includeRotatePoints) const
    -> std::shared_ptr<const ColorMap>
{
  return m_colorMapImpl->GetRandomColorMapPtr(includeRotatePoints);
}

auto ColorMaps::GetRandomColorMapPtr(ColorMapGroup cmg, const bool includeRotatePoints) const
    -> std::shared_ptr<const ColorMap>
{
  return m_colorMapImpl->GetRandomColorMapPtr(cmg, includeRotatePoints);
}

auto ColorMaps::GetNumGroups() const -> size_t
{
  return m_colorMapImpl->GetNumGroups();
}

auto ColorMaps::GetRandomGroup() const -> ColorMapGroup
{
  return m_colorMapImpl->GetRandomGroup();
}

std::vector<PrebuiltColorMap, PrebuiltColorMap::ColorMapAllocator>
    ColorMaps::ColorMapsImpl::s_preBuiltColorMaps{};
ColorMaps::ColorMapsImpl::GroupColorNames ColorMaps::ColorMapsImpl::s_groups{};

ColorMaps::ColorMapsImpl::ColorMapsImpl() noexcept = default;

auto ColorMaps::ColorMapsImpl::GetRandomColorMapName() -> ColorMapName
{
  InitPrebuiltColorMaps();

  return static_cast<ColorMapName>(getRandInRange(0U, s_preBuiltColorMaps.size()));
}

auto ColorMaps::ColorMapsImpl::GetRandomColorMapName(const ColorMapGroup groupName) -> ColorMapName
{
  InitGroups();
  InitPrebuiltColorMaps();

  const std::vector<ColorMapName>* group = at(s_groups, groupName);
  return group->at(getRandInRange(0U, group->size()));
}

auto ColorMaps::ColorMapsImpl::GetRandomColorMap() -> const ColorMap&
{
  InitPrebuiltColorMaps();

  return s_preBuiltColorMaps[getRandInRange(0U, s_preBuiltColorMaps.size())];
}

auto ColorMaps::ColorMapsImpl::GetRandomColorMap(const ColorMapGroup groupName) -> const ColorMap&
{
  InitGroups();
  InitPrebuiltColorMaps();

  const std::vector<ColorMapName>* group = at(s_groups, groupName);
  return GetColorMap(group->at(getRandInRange(0U, group->size())));
}

auto ColorMaps::ColorMapsImpl::GetColorMap(const ColorMapName name) -> const ColorMap&
{
  InitPrebuiltColorMaps();
  return s_preBuiltColorMaps[static_cast<size_t>(name)];
}

auto ColorMaps::ColorMapsImpl::GetColorMapPtr(colordata::ColorMapName mapName,
                                              const float tRotatePoint) const
    -> std::shared_ptr<const ColorMap>
{
  if (tRotatePoint > 0.0F)
  {
    return std::make_shared<ColorMapWrapper>(GetColorMap(mapName));
  }
  return std::make_shared<RotatedColorMap>(GetColorMap(mapName), tRotatePoint);
}

auto ColorMaps::ColorMapsImpl::GetRandomColorMapPtr(const bool includeRotatePoints) const
    -> std::shared_ptr<const ColorMap>
{
  if (!includeRotatePoints)
  {
    return std::make_shared<ColorMapWrapper>(GetRandomColorMap());
  }
  return std::make_shared<RotatedColorMap>(GetRandomColorMap(), getRandInRange(0.0F, 1.0F));
}

auto ColorMaps::ColorMapsImpl::GetRandomColorMapPtr(ColorMapGroup cmg,
                                                    const bool includeRotatePoints) const
    -> std::shared_ptr<const ColorMap>
{
  if (!includeRotatePoints)
  {
    return std::make_shared<ColorMapWrapper>(GetRandomColorMap(cmg));
  }
  return std::make_shared<RotatedColorMap>(GetRandomColorMap(cmg), getRandInRange(0.0F, 1.0F));
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
  s_preBuiltColorMaps.reserve(colordata::allMaps.size());
  for (const auto& [name, vividMap] : colordata::allMaps)
  {
    (void)s_preBuiltColorMaps.emplace_back(name, vividMap);
  }
}

auto ColorMaps::ColorMapsImpl::GetNumGroups() -> size_t
{
  InitGroups();
  return s_groups.size();
}

auto ColorMaps::ColorMapsImpl::GetRandomGroup() const -> ColorMapGroup
{
  InitGroups();
  return static_cast<ColorMapGroup>(getRandInRange(0U, static_cast<size_t>(ColorMapGroup::_size)));
}

void ColorMaps::ColorMapsImpl::InitGroups()
{
  if (s_groups[0] != nullptr)
  {
    return;
  }
  at(s_groups, ColorMapGroup::perceptuallyUniformSequential) = &colordata::perc_unif_sequentialMaps;
  at(s_groups, ColorMapGroup::sequential) = &colordata::sequentialMaps;
  at(s_groups, ColorMapGroup::sequential2) = &colordata::sequential2Maps;
  at(s_groups, ColorMapGroup::cyclic) = &colordata::cyclicMaps;
  at(s_groups, ColorMapGroup::diverging) = &colordata::divergingMaps;
  at(s_groups, ColorMapGroup::diverging_black) = &colordata::diverging_blackMaps;
  at(s_groups, ColorMapGroup::qualitative) = &colordata::qualitativeMaps;
  at(s_groups, ColorMapGroup::misc) = &colordata::miscMaps;
}

WeightedColorMaps::WeightedColorMaps() : ColorMaps{}, m_weights{}, m_weightsActive{true}
{
}

WeightedColorMaps::WeightedColorMaps(const Weights<ColorMapGroup>& w)
  : ColorMaps{}, m_weights{w}, m_weightsActive{true}
{
}

auto WeightedColorMaps::GetWeights() const -> const Weights<ColorMapGroup>&
{
  return m_weights;
}

void WeightedColorMaps::SetWeights(const Weights<ColorMapGroup>& w)
{
  m_weights = w;
}

auto WeightedColorMaps::AreWeightsActive() const -> bool
{
  return m_weightsActive;
}

void WeightedColorMaps::SetWeightsActive(const bool value)
{
  m_weightsActive = value;
}

auto WeightedColorMaps::GetRandomGroup() const -> ColorMapGroup
{
  if (!m_weightsActive)
  {
    return ColorMaps::GetRandomGroup();
  }

  return m_weights.getRandomWeighted();
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

auto PrebuiltColorMap::GetRandomColor(const float t0, const float t1) const -> Pixel
{
  return GetColor(getRandInRange(t0, t1));
}

auto PrebuiltColorMap::GetColorMix(const Pixel& col1, const Pixel& col2, float t) -> Pixel
{
  const vivid::rgb_t c1 = vivid::rgb::fromRgb32(col1.Rgba());
  const vivid::rgb_t c2 = vivid::rgb::fromRgb32(col2.Rgba());
  return Pixel{vivid::lerpHsl(c1, c2, t).rgb32()};
}

} // namespace goom::utils
