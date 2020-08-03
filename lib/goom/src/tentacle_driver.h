#ifndef LIBS_GOOM_INCLUDE_GOOM_TENTACLE_DRIVER_H_
#define LIBS_GOOM_INCLUDE_GOOM_TENTACLE_DRIVER_H_

#include "colormap.h"
#include "goom.h"
#include "tentacles_new.h"
#include "v3d.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <stack>
#include <string>
#include <vector>


class IterTimer {
  public:
    explicit IterTimer(const size_t startCnt): startCount { startCnt } {}

    void start() { count = startCount; }
    void next() { if (count > 0) count--; }
    bool atStart() const { return count == startCount; }
    size_t getCurrentCount() const { return count; }
  private:
    const size_t startCount;
    size_t count = 0;
};

class TentacleColorMapColorizer: public TentacleColorizer {
public:
  explicit TentacleColorMapColorizer(const ColorMap& colorMap, const size_t numNodes);
  TentacleColorMapColorizer(const TentacleColorMapColorizer&)=delete;
  TentacleColorMapColorizer& operator=(const TentacleColorMapColorizer&)=delete;

  void resetColorMap(const ColorMap& colorMap);
  void pushColorMap(const ColorMap& colorMap);
  void popColorMap();

  uint32_t getColor(size_t nodeNum) const override;
private:
  const ColorMap* origColorMap;
  const ColorMap* colorMap;
  std::stack<const ColorMap*> colorStack;
  const size_t numNodes;
};

class SimpleWeightHandler {
public:
  explicit SimpleWeightHandler(
      ConstantSequenceFunction& prevYWeightFunc,
      ConstantSequenceFunction& currentYWeightFunc);
  ConstantSequenceFunction& getPrevYWeightFunc() const { return *prevYWeightFunc; }
  ConstantSequenceFunction& getCurrentYWeightFunc() const { return *currentYWeightFunc; }
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
  RandSequenceFunction& getPrevYWeightFunc() const { return *prevYWeightFunc; }
  RandSequenceFunction& getCurrentYWeightFunc() const { return *currentYWeightFunc; }
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
  explicit TentacleDriver(const ColorMaps&);
  TentacleDriver(const TentacleDriver&)=delete;
  TentacleDriver& operator=(const TentacleDriver&)=delete;

  void init();

  void startIterating();
  void stopIterating();
  void update(const float angle,
      const float distance, const float distance2,
      const uint32_t color, const uint32_t colorLow, Pixel* frontBuff, Pixel* backBuff);

  void setGlitchValues(const float lower, const float upper);
  void setReverseColorMix(const bool val);
  void multiplyIterZeroYValWaveFreq(const float val);
private:
  struct IterationParams {
    size_t numNodes = 200;
    float prevYWeight = 0.770;
    float iterZeroYValWaveFreq = 1.0;
    SineWaveMultiplier iterZeroYValWave{};
    float length = 40;
  };
  struct IterParamsGroup {
    IterationParams first;
    IterationParams last;
    IterationParams getNext(const float t) const;
  };
  std::vector<IterParamsGroup> iterParamsGroups;

  const ColorMaps* colorMaps;
  const std::vector<std::string>* currentColorMapGroup;
  std::vector<std::unique_ptr<TentacleColorMapColorizer>> colorizers;
  ConstantSequenceFunction constPrevYWeightFunc;
  ConstantSequenceFunction constCurrentYWeightFunc;
  SimpleWeightHandler weightsHandler;
  // RandWeightHandler weightsHandler;
  TentacleTweaker::WeightFunctionsResetter weightsReset;
  TentacleTweaker::WeightFunctionsAdjuster weightsAdjust;

  size_t updateNum = 0;
  Tentacles3D tentacles;
  const float iterZeroYVal = 10.0;
  static constexpr size_t xRowLen = 14;
  static_assert(xRowLen % 2 == 0); // Perspective looks bad with odd because of x=0 tentacle.
  static constexpr size_t numXRows = 6;
  static constexpr size_t numTentacles = xRowLen*numXRows;
  std::vector<IterationParams> tentacleParams{ numTentacles };
  static constexpr size_t changeCurrentColorMapGroupEveryNUpdates = 500;
  static constexpr size_t changeTentacleColorMapEveryNUpdates = 100;
  IterTimer glitchTimer;
  static constexpr size_t doGlitchEveryNUpdates = 30;
  static constexpr size_t glitchIterLength = 10;
  const ColorMap glitchColorGroup;
  float glitchLower = -1.5;
  float glitchUpper = +1.5;
  std::unique_ptr<Tentacle2D> createNewTentacle2D(const size_t ID, const IterationParams&);
  std::unique_ptr<TentacleTweaker> createNewTweaker(const size_t tentacleLen);
  void beforeIter(const size_t ID, const size_t iterNum, const std::vector<double>& xvec, std::vector<double>& yvec);
  std::vector<IterTimer*> iterTimers;
  void updateIterTimers();
  void checkForTimerEvents();
  static void plot3D(const Tentacle3D& tentacle,
                     const uint32_t dominantColor, const uint32_t dominantColorLow,
                     const float angle, const float distance, const float distance2, Pixel* frontBuff, Pixel* backBuff);
  static void project_v3d_to_v2d(const std::vector<V3d>& v3, std::vector<v2d>& v2, const float distance);
  static void y_rotate_v3d(const V3d& vi, V3d& vf, const float sina, const float cosa);
  static void translate_v3d(const V3d& vsrc, V3d& vdest);
};

#endif
