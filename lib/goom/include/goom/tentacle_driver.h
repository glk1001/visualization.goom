#ifndef LIBS_GOOM_INCLUDE_GOOM_TENTACLE_DRIVER_H_
#define LIBS_GOOM_INCLUDE_GOOM_TENTACLE_DRIVER_H_

#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"
#include "goomutils/mathutils.h"
#include "tentacles.h"
#include "v3d.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace goom
{

class IterTimer
{
public:
  explicit IterTimer(const size_t startCnt) : startCount{startCnt} {}

  void start() { count = startCount; }
  void next()
  {
    if (count > 0)
      count--;
  }
  [[nodiscard]] bool atStart() const { return count == startCount; }
  [[nodiscard]] size_t getCurrentCount() const { return count; }

private:
  const size_t startCount;
  size_t count = 0;
};

class TentacleLayout
{
public:
  virtual ~TentacleLayout() noexcept = default;
  [[nodiscard]] virtual size_t getNumPoints() const = 0;
  [[nodiscard]] virtual const std::vector<V3d>& getPoints() const = 0;
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
  explicit TentacleDriver(uint32_t screenWidth, uint32_t screenHeight) noexcept;
  TentacleDriver(const TentacleDriver&) = delete;
  TentacleDriver& operator=(const TentacleDriver&) = delete;

  void init(const TentacleLayout&);
  [[nodiscard]] size_t getNumTentacles() const;

  [[nodiscard]] const FXBuffSettings& getBuffSettings() const;
  void setBuffSettings(const FXBuffSettings&);

  [[nodiscard]] ColorModes getColorMode() const;
  void setColorMode(ColorModes);

  void startIterating();
  void stopIterating();

  void freshStart();
  void update(float angle,
              float distance,
              float distance2,
              const Pixel& color,
              const Pixel& colorLow,
              PixelBuffer& prevBuff,
              PixelBuffer& currentBuff);

  void setReverseColorMix(bool val);
  void multiplyIterZeroYValWaveFreq(float val);

  bool operator==(const TentacleDriver&) const;

  template<class Archive>
  void serialize(Archive&);

private:
  ColorModes colorMode = ColorModes::oneGroupForAll;
  struct IterationParams
  {
    size_t numNodes = 200;
    float prevYWeight = 0.770;
    float iterZeroYValWaveFreq = 1.0;
    utils::SineWaveMultiplier iterZeroYValWave{};
    float length = 50;
    bool operator==(const IterationParams&) const;
    template<class Archive>
    void serialize(Archive&);
  };
  struct IterParamsGroup
  {
    IterationParams first{};
    IterationParams last{};
    bool operator==(const IterParamsGroup&) const;
    [[nodiscard]] IterationParams getNext(float t) const;
    template<class Archive>
    void serialize(Archive&);
  };
  std::vector<IterParamsGroup> iterParamsGroups{};

  void updateTentaclesLayout(const TentacleLayout&);

  uint32_t screenWidth = 0;
  uint32_t screenHeight = 0;
  GoomDraw draw{};
  FXBuffSettings buffSettings{};
  const utils::ColorMaps colorMaps{};
  std::vector<std::shared_ptr<TentacleColorizer>> colorizers{};

  size_t updateNum = 0;
  Tentacles3D tentacles{};
  static constexpr float iterZeroYVal = 10.0;
  size_t numTentacles = 0;
  std::vector<IterationParams> tentacleParams{};
  static const size_t changeCurrentColorMapGroupEveryNUpdates;
  static const size_t changeTentacleColorMapEveryNUpdates;
  [[nodiscard]] std::vector<utils::ColorMapGroup> getNextColorMapGroups() const;

  std::unique_ptr<Tentacle2D> createNewTentacle2D(size_t ID, const IterationParams&);
  const std::vector<IterTimer*> iterTimers{};
  void updateIterTimers();
  void checkForTimerEvents();
  void plot3D(const Tentacle3D& tentacle,
              const Pixel& dominantColor,
              const Pixel& dominantColorLow,
              float angle,
              float distance,
              float distance2,
              PixelBuffer& prevBuff,
              PixelBuffer& currentBuff);
  std::vector<v2d> projectV3dOntoV2d(const std::vector<V3d>& v3, float distance);
  static void rotateV3dAboutYAxis(float sina, float cosa, const V3d& vsrc, V3d& vdest);
  static void translateV3d(const V3d& vadd, V3d& vinOut);
};

class TentacleColorMapColorizer : public TentacleColorizer
{
public:
  TentacleColorMapColorizer() noexcept = default;
  explicit TentacleColorMapColorizer(const utils::ColorMapGroup, const size_t numNodes) noexcept;
  TentacleColorMapColorizer(const TentacleColorMapColorizer&) = delete;
  TentacleColorMapColorizer& operator=(const TentacleColorMapColorizer&) = delete;

  utils::ColorMapGroup getColorMapGroup() const override;
  void setColorMapGroup(utils::ColorMapGroup) override;
  void changeColorMap() override;
  Pixel getColor(size_t nodeNum) const override;

  bool operator==(const TentacleColorMapColorizer&) const;

  template<class Archive>
  void save(Archive&) const;
  template<class Archive>
  void load(Archive&);

private:
  size_t numNodes = 0;
  utils::ColorMapGroup currentColorMapGroup{};
  const utils::ColorMaps colorMaps{};
  const utils::ColorMap* colorMap{};
  const utils::ColorMap* prevColorMap{};
  static constexpr uint32_t maxCountSinceColormapChange = 100;
  static constexpr float transitionStep = 1.0 / static_cast<float>(maxCountSinceColormapChange);
  mutable float tTransition = 0;
};

class GridTentacleLayout : public TentacleLayout
{
public:
  GridTentacleLayout(
      float xmin, float xmax, size_t xNum, float ymin, float ymax, size_t yNum, float zConst);
  [[nodiscard]] size_t getNumPoints() const override;
  [[nodiscard]] const std::vector<V3d>& getPoints() const override;

private:
  std::vector<V3d> points{};
};

class CirclesTentacleLayout : public TentacleLayout
{
public:
  CirclesTentacleLayout(float radiusMin,
                        float radiusMax,
                        const std::vector<size_t>& numCircleSamples,
                        float zConst);
  // Order of points is outer circle to inner.
  [[nodiscard]] size_t getNumPoints() const override;
  [[nodiscard]] const std::vector<V3d>& getPoints() const override;
  static std::vector<size_t> getCircleSamples(size_t numCircles, size_t totalPoints);

private:
  std::vector<V3d> points{};
};

template<class Archive>
void TentacleDriver::serialize(Archive& ar)
{
  ar(CEREAL_NVP(colorMode), CEREAL_NVP(iterParamsGroups), CEREAL_NVP(screenWidth),
     CEREAL_NVP(screenHeight), CEREAL_NVP(draw), CEREAL_NVP(buffSettings), CEREAL_NVP(colorizers),
     CEREAL_NVP(updateNum), CEREAL_NVP(tentacles), CEREAL_NVP(numTentacles),
     CEREAL_NVP(tentacleParams));
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
  const utils::colordata::ColorMapName colorMapName = colorMap->getMapName();
  const utils::colordata::ColorMapName prevColorMapName = prevColorMap->getMapName();
  ar(CEREAL_NVP(numNodes), CEREAL_NVP(currentColorMapGroup), CEREAL_NVP(colorMapName),
     CEREAL_NVP(prevColorMapName), CEREAL_NVP(tTransition));
}

template<class Archive>
void TentacleColorMapColorizer::load(Archive& ar)
{
  utils::colordata::ColorMapName colorMapName;
  utils::colordata::ColorMapName prevColorMapName;
  ar(CEREAL_NVP(numNodes), CEREAL_NVP(currentColorMapGroup), CEREAL_NVP(colorMapName),
     CEREAL_NVP(prevColorMapName), CEREAL_NVP(tTransition));
  colorMap = &colorMaps.getColorMap(colorMapName);
  prevColorMap = &colorMaps.getColorMap(prevColorMapName);
}

} // namespace goom

// NOTE: Cereal is not happy with these calls inside the 'goom' namespace.
//   But they work OK here.
CEREAL_REGISTER_TYPE(goom::TentacleColorMapColorizer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(goom::TentacleColorizer, goom::TentacleColorMapColorizer);

#endif
