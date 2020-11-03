#ifndef LIBS_GOOM_INCLUDE_GOOM_TENTACLE_DRIVER_H_
#define LIBS_GOOM_INCLUDE_GOOM_TENTACLE_DRIVER_H_

#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"
#include "tentacles.h"
#include "v3d.h"

#include <cereal/archives/json.hpp>
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
  bool atStart() const { return count == startCount; }
  size_t getCurrentCount() const { return count; }

private:
  const size_t startCount;
  size_t count = 0;
};

class TentacleLayout
{
public:
  virtual ~TentacleLayout() noexcept = default;
  virtual size_t getNumPoints() const = 0;
  const virtual std::vector<V3d>& getPoints() const = 0;
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
  explicit TentacleDriver(const uint32_t screenWidth, const uint32_t screenHeight) noexcept;
  TentacleDriver(const TentacleDriver&) = delete;
  TentacleDriver& operator=(const TentacleDriver&) = delete;

  void init(const TentacleLayout&);
  size_t getNumTentacles() const;

  void setBuffSettings(const FXBuffSettings&);

  ColorModes getColorMode() const;
  void setColorMode(const ColorModes);

  void startIterating();
  void stopIterating();

  void freshStart();
  void update(const float angle,
              const float distance,
              const float distance2,
              const Pixel& color,
              const Pixel& colorLow,
              Pixel* prevBuff,
              Pixel* currentBuff);

  void setReverseColorMix(const bool val);
  void multiplyIterZeroYValWaveFreq(const float val);

private:
  ColorModes colorMode = ColorModes::multiGroups;
  struct IterationParams
  {
    size_t numNodes = 200;
    float prevYWeight = 0.770;
    float iterZeroYValWaveFreq = 1.0;
    utils::SineWaveMultiplier iterZeroYValWave{};
    float length = 50;
  };
  struct IterParamsGroup
  {
    IterationParams first;
    IterationParams last;
    IterationParams getNext(const float t) const;
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
  const float iterZeroYVal = 10.0;
  size_t numTentacles = 0;
  std::vector<IterationParams> tentacleParams{};
  static const size_t changeCurrentColorMapGroupEveryNUpdates;
  static const size_t changeTentacleColorMapEveryNUpdates;
  std::vector<utils::ColorMapGroup> getNextColorMapGroups() const;

  std::unique_ptr<Tentacle2D> createNewTentacle2D(const size_t ID, const IterationParams&);
  const std::vector<IterTimer*> iterTimers{};
  void updateIterTimers();
  void checkForTimerEvents();
  void plot3D(const Tentacle3D& tentacle,
              const Pixel& dominantColor,
              const Pixel& dominantColorLow,
              const float angle,
              const float distance,
              const float distance2,
              Pixel* prevBuff,
              Pixel* currentBuff);
  std::vector<v2d> projectV3dOntoV2d(const std::vector<V3d>& v3, const float distance);
  static void rotateV3dAboutYAxis(const float sina, const float cosa, const V3d& vsrc, V3d& vdest);
  static void translateV3d(const V3d& vadd, V3d& vinOut);
};

class TentacleColorMapColorizer : public TentacleColorizer
{
public:
  explicit TentacleColorMapColorizer(const utils::ColorMapGroup, const size_t numNodes);
  TentacleColorMapColorizer(const TentacleColorMapColorizer&) = delete;
  TentacleColorMapColorizer& operator=(const TentacleColorMapColorizer&) = delete;

  utils::ColorMapGroup getColorMapGroup() const override;
  void setColorMapGroup(const utils::ColorMapGroup) override;
  void changeColorMap() override;
  Pixel getColor(const size_t nodeNum) const override;

private:
  const size_t numNodes;
  utils::ColorMapGroup currentColorMapGroup;
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
  GridTentacleLayout(const float xmin,
                     const float xmax,
                     const size_t xNum,
                     const float ymin,
                     const float ymax,
                     const size_t yNum,
                     const float zConst);
  size_t getNumPoints() const override;
  const std::vector<V3d>& getPoints() const override;

private:
  std::vector<V3d> points{};
};

class CirclesTentacleLayout : public TentacleLayout
{
public:
  CirclesTentacleLayout(const float radiusMin,
                        const float radiusMax,
                        const std::vector<size_t>& numCircleSamples,
                        const float zConst);
  // Order of points is outer circle to inner.
  size_t getNumPoints() const override;
  const std::vector<V3d>& getPoints() const override;
  static std::vector<size_t> getCircleSamples(const size_t numCircles, const size_t totalPoints);

private:
  std::vector<V3d> points{};
};

} // namespace goom
#endif
