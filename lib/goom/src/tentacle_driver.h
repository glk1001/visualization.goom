#ifndef LIBS_GOOM_INCLUDE_GOOM_TENTACLE_DRIVER_H_
#define LIBS_GOOM_INCLUDE_GOOM_TENTACLE_DRIVER_H_

#include "tentacles_new.h"
#include "goom.h"

#include "fmt/format.h"
#include "utils/goom_loggers.hpp"
#include <vivid/vivid.h>

#include <cstdint>
#include <cmath>
#include <memory>
#include <stack>
#include <vector>


class IterTimer {
public:
  explicit IterTimer(const size_t startCnt): startCount{ startCnt }{}
  void start() { count = startCount; }
  void next() { if (count > 0) count--; }
  bool atStart() const { return count == startCount; }
  size_t getCurrentCount() const { return count; }
private:
  const size_t startCount;
  size_t count = 0;
};

class ColorGroup {
public:
  explicit ColorGroup(vivid::ColorMap::Preset preset);
  ColorGroup(const ColorGroup& other);
  ColorGroup& operator=(const ColorGroup& other) { colors = other.colors; return *this; }
  size_t numColors() const { return colors.size(); }
  uint32_t getColor(size_t i) const { return colors[i]; }
private:
  static constexpr size_t NumColors = 720;
  std::vector<uint32_t> colors;
};

inline ColorGroup::ColorGroup(vivid::ColorMap::Preset preset)
: colors(NumColors)
{
  logInfo(fmt::format("preset = {}", preset));
  const vivid::ColorMap cmap(preset);
  for (size_t i=0 ; i < NumColors; i++) {
    const float t = i / (NumColors - 1.0f);
    const vivid::Color rgbcol{ cmap.at(t) };
    colors[i] = rgbcol.rgb32();
  }
}

inline ColorGroup::ColorGroup(const ColorGroup& other)
: colors(other.colors)
{
}

class TentacleColorGroup: public TentacleColorizer {
public:
  explicit TentacleColorGroup(const ColorGroup& colorGroup, const size_t numNodes);
  TentacleColorGroup(const TentacleColorGroup&)=delete;
  TentacleColorGroup& operator=(const TentacleColorGroup&)=delete;

  void pushColor(const ColorGroup& colorGroup);
  void popColor();

  uint32_t getColor(size_t nodeNum) const override;
private:
  const ColorGroup* colorGroup;
  std::stack<const ColorGroup*> colorStack;
  const size_t numNodes;
};

class SimpleWeightHandler {
public:
  explicit SimpleWeightHandler(
      ConstantSequenceFunction& prevYWeightFunc,
      ConstantSequenceFunction& currentYWeightFunc);
  ConstantSequenceFunction& getPrevYWeightFunc() { return *prevYWeightFunc; }
  ConstantSequenceFunction& getCurrentYWeightFunc() { return *currentYWeightFunc; }
  void weightsReset(
      const size_t ID, const size_t numNodes,
      const float basePrevYWeight, const float baseCurrentYWeight);
  void weightsAdjust(
      const size_t ID,
      const size_t iterNum, const size_t nodeNum,
      const float prevY, const float currentY);
private:
  ConstantSequenceFunction* prevYWeightFunc;
  ConstantSequenceFunction* currentYWeightFunc;
  float basePrevYWeight = 0;
  float baseCurrentYWeight = 0;
};

class RandWeightHandler {
public:
  explicit RandWeightHandler(
      RandSequenceFunction& prevYWeightFunc,
      RandSequenceFunction& currentYWeightFunc,
      const float r0, const float r1);
  RandSequenceFunction& getPrevYWeightFunc() { return *prevYWeightFunc; }
  RandSequenceFunction& getCurrentYWeightFunc() { return *currentYWeightFunc; }
  void weightsReset(
      const size_t ID,
      const size_t numIters, const size_t numNodes,
      const float basePrevYWeight, const float baseCurrentYWeight);
  void weightsAdjust(
      const size_t ID,
      const size_t iterNum, const size_t nodeNum,
      const float prevY, const float currentY);
private:
  RandSequenceFunction* prevYWeightFunc;
  RandSequenceFunction* currentYWeightFunc;
  const float r0;
  const float r1;
};

class TentacleLayout {
public:
  virtual ~TentacleLayout() {}
  virtual size_t getNumPoints() const=0;
  virtual V3d getNextPosition()=0;
  virtual bool finished() const=0;
  virtual void reset()=0;
};

class GridTentacleLayout: public TentacleLayout {
public:
  GridTentacleLayout(
      const float xmin, const float xmax, const size_t xNum,
      const float ymin, const float ymax, const size_t yNum,
      const float zConst);
  size_t getNumPoints() const override;
  V3d getNextPosition() override;
  bool finished() const override;
  void reset() override;
private:
  const float xmin;
  const float xmax;
  const size_t xNum;
  const float ymin;
  const float ymax;
  const size_t yNum;
  const float zConst;
  const float xStep;
  const float yStep;
  float x;
  float y;
};

class TentacleDriver {
public:
  TentacleDriver();
  TentacleDriver(const TentacleDriver&)=delete;
  TentacleDriver& operator=(const TentacleDriver&)=delete;

  void init();

  void startIterating();
  void stopIterating();
  void update(const float angle,
      const float distance, const float distance2,
      const uint32_t color, const uint32_t colorLow, Pixel* frontBuff, Pixel* backBuff);

  const ColorGroup& getRandomColorGroup() const;
  uint32_t getRandomColor(const ColorGroup& cg) const;
private:
  struct IterationParams {
    size_t numNodes = 200;
    float prevYWeight = 0.770;
    float currentYWeight = 0.230;
    SineWaveMultiplier iterZeroYValWave{};
    float length = 30;
  };
  using IterParamsGroup = std::vector<IterationParams>;
  std::vector<IterParamsGroup> iterParamsGroups;

  std::vector<ColorGroup> colorGroups;
  std::vector<std::unique_ptr<TentacleColorGroup>> colorizers;
  ConstantSequenceFunction constPrevYWeightFunc;
  ConstantSequenceFunction constCurrentYWeightFunc;
  SimpleWeightHandler weightsHandler;
  TentacleTweaker::WeightFunctionsResetter weightsReset;
  TentacleTweaker::WeightFunctionsAdjuster weightsAdjust;

//  RandSequenceFunction randPrevYWeightFunc;
//  RandSequenceFunction randCurrentYWeightFunc;
//  RandWeightHandler weightsHandler { randPrevYWeightFunc, randCurrentYWeightFunc, 0.9, 1.1 };
//  TentacleTweaker::WeightFunctionsResetter weightsReset =
//      std::bind(&RandWeightHandler::weightsReset, &weightsHandler, _1, _2, _3, _4);
//  TentacleTweaker::WeightFunctionsAdjuster weightsAdjust =
//      std::bind(&RandWeightHandler::weightsAdjust, &weightsHandler, _1, _2, _3, _4);

  //  LogDampingFunction dampingFunc{ 0.1, xmin };
  std::unique_ptr<ExpDampingFunction> dampingFunc;
  //  NoDampingFunction dampingFunc;

  size_t updateNum = 0;
  Tentacles3D tentacles;
  std::unique_ptr<TentacleTweaker> tweaker;
  const float iterZeroYVal = 10.0;
  static constexpr size_t xRowLen = 13;
  static constexpr size_t mult = 6;
  static constexpr size_t numTentacles = xRowLen*mult;
  std::vector<IterationParams> tentacleParams{ numTentacles };
  static constexpr size_t doDominantColorEveryNUpdates = 10;
//  IterTimer dominantColorTimer;
  const ColorGroup* dominantColorGroup;
  uint32_t dominantColor = 0;
  IterTimer glitchTimer;
  static constexpr size_t doGlitchEveryNUpdates = 30;
  static constexpr size_t glitchIterLength = 10;
  const ColorGroup glitchColorGroup;
  void resetYVec(const size_t ID, const size_t iterNum, const std::vector<double>& xvec, std::vector<double>& yvec);
  std::vector<IterTimer*> iterTimers;
  void updateIterTimers();
  void checkForTimerEvents();
};

#endif

