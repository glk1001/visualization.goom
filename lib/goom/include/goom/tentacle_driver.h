#ifndef VISUALIZATION_GOOM_TENTACLE_DRIVER_H
#define VISUALIZATION_GOOM_TENTACLE_DRIVER_H

#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_visual_fx.h"
#include "goomutils/colormaps.h"
#include "goomutils/mathutils.h"
#include "goomutils/random_colormaps.h"
#include "tentacles.h"
#include "v3d.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace GOOM
{

class IterTimer
{
public:
  explicit IterTimer(const size_t startCnt) : m_startCount{startCnt} {}

  void Start() { m_count = m_startCount; }
  void Next()
  {
    if (m_count > 0)
    {
      m_count--;
    }
  }
  [[nodiscard]] auto AtStart() const -> bool { return m_count == m_startCount; }
  [[nodiscard]] auto GetCurrentCount() const -> size_t { return m_count; }

private:
  const size_t m_startCount;
  size_t m_count = 0;
};

class ITentacleLayout
{
public:
  ITentacleLayout() noexcept = default;
  virtual ~ITentacleLayout() noexcept = default;
  ITentacleLayout(const ITentacleLayout&) noexcept = default;
  ITentacleLayout(ITentacleLayout&&) noexcept = delete;
  auto operator=(const ITentacleLayout&) -> ITentacleLayout& = delete;
  auto operator=(ITentacleLayout&&) -> ITentacleLayout& = delete;

  [[nodiscard]] virtual auto GetNumPoints() const -> size_t = 0;
  [[nodiscard]] virtual auto GetPoints() const -> const std::vector<V3d>& = 0;
};

class TentacleDriver
{
public:
  enum class ColorModes
  {
    minimal,
    oneGroupForAll,
    multiGroups,
  };

  TentacleDriver() noexcept;
  ~TentacleDriver() noexcept = default;
  TentacleDriver(uint32_t screenWidth, uint32_t screenHeight) noexcept;
  TentacleDriver(const TentacleDriver&) noexcept = delete;
  TentacleDriver(TentacleDriver&&) noexcept = delete;
  auto operator=(const TentacleDriver&) -> TentacleDriver& = delete;
  auto operator=(TentacleDriver&&) -> TentacleDriver& = delete;

  void Init(const ITentacleLayout& l);
  [[nodiscard]] auto GetNumTentacles() const -> size_t;

  [[nodiscard]] auto GetBuffSettings() const -> const FXBuffSettings&;
  void SetBuffSettings(const FXBuffSettings& settings);

  [[nodiscard]] auto GetColorMode() const -> ColorModes;
  void SetColorMode(ColorModes m);

  void StartIterating();
  [[maybe_unused]] void StopIterating();

  void FreshStart();
  void Update(float angle,
              float distance,
              float distance2,
              const Pixel& color,
              const Pixel& colorLow,
              PixelBuffer& currentBuff,
              PixelBuffer& nextBuff);

  void SetReverseColorMix(bool val);
  void MultiplyIterZeroYValWaveFreq(float val);

  auto operator==(const TentacleDriver& d) const -> bool;

  template<class Archive>
  void serialize(Archive& ar);

private:
  ColorModes m_colorMode = ColorModes::oneGroupForAll;
  struct IterationParams
  {
    size_t numNodes = 200;
    float prevYWeight = 0.770F;
    float iterZeroYValWaveFreq = 1.0F;
    UTILS::SineWaveMultiplier iterZeroYValWave{};
    float length = 50.0F;
    auto operator==(const IterationParams&) const -> bool;
    template<class Archive>
    void serialize(Archive& ar);
  };
  struct IterParamsGroup
  {
    IterationParams first{};
    IterationParams last{};
    auto operator==(const IterParamsGroup&) const -> bool;
    [[nodiscard]] auto GetNext(float t) const -> IterationParams;
    template<class Archive>
    void serialize(Archive& ar);
  };
  std::vector<IterParamsGroup> m_iterParamsGroups{};

  void UpdateTentaclesLayout(const ITentacleLayout& l);

  uint32_t m_screenWidth = 0;
  uint32_t m_screenHeight = 0;
  GoomDraw m_draw{};
  FXBuffSettings m_buffSettings{};
  const UTILS::RandomColorMaps m_colorMaps{};
  std::vector<std::shared_ptr<ITentacleColorizer>> m_colorizers{};

  size_t m_updateNum = 0;
  Tentacles3D m_tentacles{};
  static constexpr float ITER_ZERO_Y_VAL = 10.0;
  size_t m_numTentacles = 0;
  std::vector<IterationParams> m_tentacleParams{};
  static const size_t CHANGE_CURRENT_COLOR_MAP_GROUP_EVERY_N_UPDATES;
  static const size_t CHANGE_TENTACLE_COLOR_MAP_EVERY_N_UPDATES;
  [[nodiscard]] auto GetNextColorMapGroups() const -> std::vector<UTILS::ColorMapGroup>;

  static auto CreateNewTentacle2D(size_t id, const IterationParams& p)
      -> std::unique_ptr<Tentacle2D>;
  const std::vector<IterTimer*> m_iterTimers{};
  void UpdateIterTimers();
  void CheckForTimerEvents();
  void Plot3D(const Tentacle3D& tentacle,
              const Pixel& dominantColor,
              const Pixel& dominantColorLow,
              float angle,
              float distance,
              float distance2,
              PixelBuffer& currentBuff,
              PixelBuffer& nextBuff);
  [[nodiscard]] auto ProjectV3DOntoV2D(const std::vector<V3d>& v3, float distance) const
      -> std::vector<v2d>;
  static void RotateV3DAboutYAxis(float sinAngle, float cosAngle, const V3d& src, V3d& dest);
  static void TranslateV3D(const V3d& add, V3d& inOut);
};

class TentacleColorMapColorizer : public ITentacleColorizer
{
public:
  TentacleColorMapColorizer() noexcept = default;
  ~TentacleColorMapColorizer() noexcept override = default;
  explicit TentacleColorMapColorizer(UTILS::ColorMapGroup, size_t numNodes) noexcept;
  TentacleColorMapColorizer(const TentacleColorMapColorizer&) noexcept = delete;
  TentacleColorMapColorizer(TentacleColorMapColorizer&&) noexcept = delete;
  auto operator=(const TentacleColorMapColorizer&) -> TentacleColorMapColorizer& = delete;
  auto operator=(TentacleColorMapColorizer&&) -> TentacleColorMapColorizer& = delete;

  auto GetColorMapGroup() const -> UTILS::ColorMapGroup override;
  void SetColorMapGroup(UTILS::ColorMapGroup c) override;
  void ChangeColorMap() override;
  auto GetColor(size_t nodeNum) const -> Pixel override;

  auto operator==(const TentacleColorMapColorizer&) const -> bool;

  template<class Archive>
  void save(Archive&) const;
  template<class Archive>
  void load(Archive&);

private:
  size_t m_numNodes = 0;
  UTILS::ColorMapGroup m_currentColorMapGroup{};
  const UTILS::RandomColorMaps m_colorMaps{};
  std::shared_ptr<const UTILS::IColorMap> m_colorMap{};
  std::shared_ptr<const UTILS::IColorMap> m_prevColorMap{};
  static constexpr uint32_t MAX_COUNT_SINCE_COLORMAP_CHANGE = 100;
  static constexpr float TRANSITION_STEP =
      1.0 / static_cast<float>(MAX_COUNT_SINCE_COLORMAP_CHANGE);
  mutable float m_tTransition = 0.0F;
};

class GridTentacleLayout : public ITentacleLayout
{
public:
  [[maybe_unused]] GridTentacleLayout(
      float xmin, float xmax, size_t xNum, float ymin, float ymax, size_t yNum, float zConst);
  [[nodiscard]] auto GetNumPoints() const -> size_t override;
  [[nodiscard]] auto GetPoints() const -> const std::vector<V3d>& override;

private:
  std::vector<V3d> m_points{};
};

class CirclesTentacleLayout : public ITentacleLayout
{
public:
  CirclesTentacleLayout(float radiusMin,
                        float radiusMax,
                        const std::vector<size_t>& numCircleSamples,
                        float zConst);
  // Order of points is outer circle to inner.
  [[nodiscard]] auto GetNumPoints() const -> size_t override;
  [[nodiscard]] auto GetPoints() const -> const std::vector<V3d>& override;
  static auto GetCircleSamples(size_t numCircles, size_t totalPoints) -> std::vector<size_t>;

private:
  std::vector<V3d> m_points{};
};

template<class Archive>
void TentacleDriver::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_colorMode), CEREAL_NVP(m_iterParamsGroups), CEREAL_NVP(m_screenWidth),
     CEREAL_NVP(m_screenHeight), CEREAL_NVP(m_draw), CEREAL_NVP(m_buffSettings),
     CEREAL_NVP(m_colorizers), CEREAL_NVP(m_updateNum), CEREAL_NVP(m_tentacles),
     CEREAL_NVP(m_numTentacles), CEREAL_NVP(m_tentacleParams));
}

template<class Archive>
void TentacleDriver::IterationParams::serialize(Archive& ar)
{
  ar(CEREAL_NVP(numNodes), CEREAL_NVP(prevYWeight), CEREAL_NVP(iterZeroYValWave),
     CEREAL_NVP(length));
}

template<class Archive>
void TentacleDriver::IterParamsGroup::serialize(Archive& ar)
{
  ar(CEREAL_NVP(first), CEREAL_NVP(last));
}

template<class Archive>
void TentacleColorMapColorizer::save(Archive& ar) const
{
  const UTILS::COLOR_DATA::ColorMapName colorMapName = m_colorMap->GetMapName();
  const UTILS::COLOR_DATA::ColorMapName prevColorMapName = m_prevColorMap->GetMapName();
  ar(CEREAL_NVP(m_numNodes), CEREAL_NVP(m_currentColorMapGroup), CEREAL_NVP(colorMapName),
     CEREAL_NVP(prevColorMapName), CEREAL_NVP(m_tTransition));
}

template<class Archive>
void TentacleColorMapColorizer::load(Archive& ar)
{
  UTILS::COLOR_DATA::ColorMapName colorMapName;
  UTILS::COLOR_DATA::ColorMapName prevColorMapName;
  ar(CEREAL_NVP(m_numNodes), CEREAL_NVP(m_currentColorMapGroup), CEREAL_NVP(colorMapName),
     CEREAL_NVP(prevColorMapName), CEREAL_NVP(m_tTransition));
  m_colorMap = m_colorMaps.GetColorMapPtr(colorMapName);
  m_prevColorMap = m_colorMaps.GetColorMapPtr(prevColorMapName);
}

} // namespace GOOM

// NOTE: Cereal is not happy with these calls inside the 'goom' namespace.
//   But they work OK here.
CEREAL_REGISTER_TYPE(GOOM::TentacleColorMapColorizer)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GOOM::ITentacleColorizer, GOOM::TentacleColorMapColorizer)

#endif
