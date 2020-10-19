#ifndef LIBS_GOOM_INCLUDE_GOOM_TENTACLE_DRIVER_H_
#define LIBS_GOOM_INCLUDE_GOOM_TENTACLE_DRIVER_H_

#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"
#include "tentacles.h"
#include "v3d.h"

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

class SimpleWeightHandler
{
public:
  explicit SimpleWeightHandler(utils::ConstantSequenceFunction& prevYWeightFunc,
                               utils::ConstantSequenceFunction& currentYWeightFunc);
  utils::ConstantSequenceFunction& getPrevYWeightFunc() const { return *prevYWeightFunc; }
  utils::ConstantSequenceFunction& getCurrentYWeightFunc() const { return *currentYWeightFunc; }
  void weightsReset(const size_t ID,
                    const size_t numNodes,
                    const float basePrevYWeight,
                    const float baseCurrentYWeight);
  void weightsAdjust(const size_t ID,
                     const size_t iterNum,
                     const size_t nodeNum,
                     const float prevY,
                     const float currentY);

private:
  utils::ConstantSequenceFunction* prevYWeightFunc;
  utils::ConstantSequenceFunction* currentYWeightFunc;
  float basePrevYWeight = 0;
  float baseCurrentYWeight = 0;
};

class RandWeightHandler
{
public:
  explicit RandWeightHandler(utils::RandSequenceFunction& prevYWeightFunc,
                             utils::RandSequenceFunction& currentYWeightFunc,
                             const float r0,
                             const float r1);
  utils::RandSequenceFunction& getPrevYWeightFunc() const { return *prevYWeightFunc; }
  utils::RandSequenceFunction& getCurrentYWeightFunc() const { return *currentYWeightFunc; }
  void weightsReset(const size_t ID,
                    const size_t numIters,
                    const size_t numNodes,
                    const float basePrevYWeight,
                    const float baseCurrentYWeight);
  void weightsAdjust(const size_t ID,
                     const size_t iterNum,
                     const size_t nodeNum,
                     const float prevY,
                     const float currentY);

private:
  utils::RandSequenceFunction* prevYWeightFunc;
  utils::RandSequenceFunction* currentYWeightFunc;
  const float r0;
  const float r1;
};

class TentacleLayout
{
public:
  virtual ~TentacleLayout() {}
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

  explicit TentacleDriver(const utils::ColorMaps*,
                          const uint32_t screenWidth,
                          const uint32_t screenHeight);
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
              const uint32_t color,
              const uint32_t colorLow,
              Pixel* prevBuff,
              Pixel* currentBuff);

  void setRoughTentacles(const bool val);
  void setGlitchValues(const float lower, const float upper);
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
  std::vector<IterParamsGroup> iterParamsGroups;

  void updateTentaclesLayout(const TentacleLayout&);

  const uint32_t screenWidth;
  const uint32_t screenHeight;
  GoomDraw draw;
  FXBuffSettings buffSettings;
  const utils::ColorMaps* colorMaps;
  std::vector<std::unique_ptr<TentacleColorizer>> colorizers;
  utils::ConstantSequenceFunction constPrevYWeightFunc;
  utils::ConstantSequenceFunction constCurrentYWeightFunc;
  SimpleWeightHandler weightsHandler;
  // RandWeightHandler weightsHandler;

  size_t updateNum = 0;
  Tentacles3D tentacles;
  const float iterZeroYVal = 10.0;
  size_t numTentacles = 0;
  std::vector<IterationParams> tentacleParams;
  static const size_t changeCurrentColorMapGroupEveryNUpdates;
  static const size_t changeTentacleColorMapEveryNUpdates;
  std::vector<utils::ColorMapGroup> getNextColorMapGroups() const;

  IterTimer roughenTimer;
  static constexpr size_t roughenEveryNUpdates = 30000000;
  static constexpr size_t roughenIterLength = 10;

  IterTimer glitchTimer;
  static constexpr size_t doGlitchEveryNUpdates = 30;
  static constexpr size_t glitchIterLength = 10;
  const utils::ColorMap* glitchColorGroup;
  float glitchLower = -1.5;
  float glitchUpper = +1.5;

  std::unique_ptr<Tentacle2D> createNewTentacle2D(const size_t ID, const IterationParams&);
  std::unique_ptr<TentacleTweaker> createNewTweaker(
      const IterationParams& params, std::unique_ptr<utils::DampingFunction> dampingFunc);
  std::unique_ptr<utils::DampingFunction> createNewDampingFunction(const IterationParams&,
                                                                   const size_t tentacleLen) const;
  std::unique_ptr<utils::DampingFunction> createNewExpDampingFunction(
      const IterationParams&, const size_t tentacleLen) const;
  std::unique_ptr<utils::DampingFunction> createNewLinearDampingFunction(
      const IterationParams&, const size_t tentacleLen) const;
  void beforeIter(const size_t ID,
                  const size_t iterNum,
                  const std::vector<double>& xvec,
                  std::vector<double>& yvec);
  std::vector<IterTimer*> iterTimers;
  void updateIterTimers();
  void checkForTimerEvents();
  void plot3D(const Tentacle3D& tentacle,
              const uint32_t dominantColor,
              const uint32_t dominantColorLow,
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
  explicit TentacleColorMapColorizer(const utils::ColorMaps&,
                                     const utils::ColorMapGroup,
                                     const size_t numNodes);
  TentacleColorMapColorizer(const TentacleColorMapColorizer&) = delete;
  TentacleColorMapColorizer& operator=(const TentacleColorMapColorizer&) = delete;

  utils::ColorMapGroup getColorMapGroup() const override;
  void setColorMapGroup(const utils::ColorMapGroup) override;
  void changeColorMap() override;
  uint32_t getColor(const size_t nodeNum) const override;

private:
  const utils::ColorMaps* colorMaps;
  utils::ColorMapGroup currentColorMapGroup;
  const utils::ColorMap* colorMap;
  const utils::ColorMap* prevColorMap;
  static constexpr uint32_t maxCountSinceColormapChange = 100;
  static constexpr float transitionStep = 1.0 / static_cast<float>(maxCountSinceColormapChange);
  mutable float tTransition;
  const size_t numNodes;
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
  std::vector<V3d> points;
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
  std::vector<V3d> points;
};

} // namespace goom
#endif