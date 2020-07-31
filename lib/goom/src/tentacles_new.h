#ifndef LIBS_GOOM_INCLUDE_GOOM_TENTACLES_NEW_H_
#define LIBS_GOOM_INCLUDE_GOOM_TENTACLES_NEW_H_

#include <fmt/format.h>
#include <utils/goom_loggers.hpp>
#include <vivid/vivid.h>

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>
#include "math_utils.h"


class TentacleColorizer {
public:
  virtual ~TentacleColorizer() {}
  virtual uint32_t getColor(size_t nodeNum) const=0;
};

class TentacleTweaker {
public:
  using YVecResetterFunction = std::function<void(
      const size_t ID, const size_t iterNum, const std::vector<double>&, std::vector<double>&)>;
  using WeightFunctionsResetter = std::function<void(
      const size_t ID, const size_t numNodes,
      const float basePrevYWeight, const float baseCurrentYWeight)>;
  using WeightFunctionsAdjuster = std::function<void(
      const size_t ID,
      const size_t iterNum, const size_t nodeNum,
      const float prevY, const float currentY)>;
public:
  explicit TentacleTweaker(
      DampingFunction* dampingFunc,
      YVecResetterFunction yVecResetterFunc,
      SequenceFunction* prevYWeightFunc,
      SequenceFunction* currentYWeightFunc,
      WeightFunctionsResetter weightFuncsReset,
      WeightFunctionsAdjuster weightFuncsAdjuster);
  TentacleTweaker(const TentacleTweaker&)=delete;
  TentacleTweaker& operator=(const TentacleTweaker&)=delete;

  void resetYVector(const size_t ID, const size_t iterNum,
      const std::vector<double>& xvec, std::vector<double>& yvec);

  double getDamping(const double x);
  void resetWeightsFunction(
      const size_t ID, const size_t numNodes,
      const float basePrevYWeight, const float baseCurrentYWeight);
  void adjustWeightsFunction(
      const size_t ID,
      const size_t iterNum, const size_t nodeNum,
      const float prevY, const float currentY);
  double getNextPrevYWeight();
  double getNextCurrentYWeight();

  void setDampingFunc(DampingFunction& dampingFunc);
  void setCurrentYWeightFunc(SequenceFunction& weightFunc);
  void setCurrentYWeightFuncAdjuster(WeightFunctionsAdjuster& weightFuncAdjuster);
private:
  DampingFunction* dampingFunc;
  YVecResetterFunction yVecResetterFunc;
  SequenceFunction* prevYWeightFunc;
  SequenceFunction* currentYWeightFunc;
  WeightFunctionsResetter weightFuncsReset;
  WeightFunctionsAdjuster weightFuncsAdjuster;
  static constexpr auto emptyErrorAdjuster = [](const size_t, const size_t) {};
};

inline double TentacleTweaker::getDamping(const double x)
{
  return (*dampingFunc)(x);
}

inline void TentacleTweaker::resetYVector(
    const size_t ID, const size_t iterNum, const std::vector<double>& xvec, std::vector<double>& yvec)
{
  yVecResetterFunc(ID, iterNum, xvec, yvec);
}

inline double TentacleTweaker::getNextPrevYWeight()
{
  return prevYWeightFunc->getNext();
}

inline double TentacleTweaker::getNextCurrentYWeight()
{
  return currentYWeightFunc->getNext();
}

inline void TentacleTweaker::resetWeightsFunction(
    const size_t ID, const size_t numNodes,
    const float basePrevYWeight, const float baseCurrentYWeight)
{
  weightFuncsReset(ID, numNodes, basePrevYWeight, baseCurrentYWeight);
}

inline void TentacleTweaker::adjustWeightsFunction(
    const size_t ID,
    const size_t iterNum, const size_t nodeNum,
    const float prevY, const float currentY)
{
  weightFuncsAdjuster(ID, iterNum, nodeNum, prevY, currentY);
}

class Tentacle2D {
private:
  using XandYVectors = std::tuple<std::vector<double>&, std::vector<double>&>;

public:
  explicit Tentacle2D(const size_t ID, TentacleTweaker* tweaker);
  Tentacle2D(const Tentacle2D&)=delete;
  Tentacle2D& operator=(const Tentacle2D&)=delete;

  size_t getID() const;

  void startIterating();
  void resetYVector();
  void finishIterating();
  size_t getIterNum() const;

  void iterate();
  void iterateNTimes(const size_t n);

  const XandYVectors& getXandYVectors() const;
  const XandYVectors& getDampedXandYVectors() const;

  double getXMin() const;
  double getXMax() const;
  void setXDimensions(const double x0, const double y0);
  
  double getYMin() const;
  double getYMax() const;
  void setYDimensions(const double y0, const double y1);
  
  size_t getNumNodes() const;
  void setNumNodes(const size_t val);

  double getPrevYWeight() const;
  void setPrevYWeight(const double val);

  double getCurrentYWeight() const;
  void setCurrentYWeight(const double val);

  double getIterZeroYVal() const;
  void setIterZeroYVal(const double val);

  double getIterZeroLerpFactor() const;
  void setIterZeroLerpFactor(const double val);

  double getYScale() const;
  void setYScale(const double val);

  bool getDoDamping() const;
  void setDoDamping(const bool val);

  bool getDoPrevYWeightAdjust() const;
  void setDoPrevYWeightAdjust(const bool val);

  bool getDoCurrentYWeightAdjust() const;
  void setDoCurrentYWeightAdjust(const bool val);

  bool getDoErrorAdjust() const;
  void setDoErrorAdjust(const bool val);

  void turnOffAllAdjusts();
  void turnOnAllAdjusts();
private:
  const size_t ID;
  TentacleTweaker* tweaker;
  size_t iterNum = 0;
  size_t numNodes = 0;
  bool startedIterating = false;
  std::vector<double> xvec;
  std::vector<double> yvec;
  XandYVectors vecs;
  std::vector<double> dampedYVec;
  std::vector<double> dampingCache;
  XandYVectors dampedVecs;
  double xmin = 0;
  double xmax = 0;
  double ymin = 0;
  double ymax = 0;
  double yScale = 1.0;
  double basePrevYWeight = 0.770;
  double baseCurrentYWeight = 0.230;
  double iterZeroYVal = 0.9;
  double iterZeroLerpFactor = 0.8;
  bool doDamping = true;
  bool doPrevYWeightAdjust = false;
  bool doCurrentYWeightAdjust = false;
  float getFirstY();
  float getNextY(const size_t nodeNum);
  float getDampedY(const size_t nodeNum);
  double damp(const size_t nodeNum);
  void validateSettings() const;
  void validateXDimensions() const;
  void validateYDimensions() const;
  void validateNumNodes() const;
  void validateYScale() const;
  void validatePrevYWeight() const;
  void validateCurrentYWeight() const;
};

inline size_t Tentacle2D::getID() const
{
  return ID;
}

inline double Tentacle2D::damp(const size_t nodeNum)
{
  return dampingCache[nodeNum];
}

inline double Tentacle2D::getYScale() const { return yScale; }

inline void Tentacle2D::setYScale(const double val)
{
  yScale = val;
  validateYScale();
}

inline double Tentacle2D::getIterZeroYVal() const { return iterZeroYVal; }
inline double Tentacle2D::getIterZeroLerpFactor() const { return iterZeroLerpFactor; }

inline void Tentacle2D::setIterZeroYVal(const double val)
{
  iterZeroYVal = val;
}

inline void Tentacle2D::setIterZeroLerpFactor(const double val)
{
  iterZeroLerpFactor = val;
}

inline size_t Tentacle2D::getNumNodes() const { return numNodes; }

inline void Tentacle2D::setNumNodes(const size_t val)
{
  if (startedIterating) {
    throw std::runtime_error("Can't set numNodes dimensions after iteration start.");
  }

  numNodes = val; 
  validateNumNodes();
}

inline double Tentacle2D::getPrevYWeight() const { return basePrevYWeight; }

inline void Tentacle2D::setPrevYWeight(const double val)
{
  basePrevYWeight = val;
  validatePrevYWeight();
}

inline double Tentacle2D::getCurrentYWeight() const { return baseCurrentYWeight; }

inline void Tentacle2D::setCurrentYWeight(const double val)
{
  baseCurrentYWeight = val;
  validateCurrentYWeight();
}

inline size_t Tentacle2D::getIterNum() const { return iterNum; }

inline double Tentacle2D::getXMin() const { return xmin; }
inline double Tentacle2D::getXMax() const { return xmax; }

inline void Tentacle2D::setXDimensions(const double x0, const double x1)
{
  if (startedIterating) {
    throw std::runtime_error("Can't set x dimensions after iteration start.");
  }

  xmin = x0;
  xmax = x1;
  validateXDimensions();
}

inline double Tentacle2D::getYMin() const { return ymin; }
inline double Tentacle2D::getYMax() const { return ymax; }

inline void Tentacle2D::setYDimensions(const double y0, const double y1)
{
  if (startedIterating) {
    throw std::runtime_error("Can't set y dimensions after iteration start.");
  }

  ymin = y0;
  ymax = y1;
  validateYDimensions();
}

inline bool Tentacle2D::getDoDamping() const { return doDamping; };
inline void Tentacle2D::setDoDamping(const bool val) { doDamping = val; };

inline bool Tentacle2D::getDoPrevYWeightAdjust() const { return doPrevYWeightAdjust; }
inline void Tentacle2D::setDoPrevYWeightAdjust(const bool val) { doPrevYWeightAdjust = val; }

inline bool Tentacle2D::getDoCurrentYWeightAdjust() const { return doCurrentYWeightAdjust; }
inline void Tentacle2D::setDoCurrentYWeightAdjust(const bool val) { doCurrentYWeightAdjust = val; }

inline void Tentacle2D::turnOffAllAdjusts()
{
  setDoDamping(false);
  setDoPrevYWeightAdjust(false);
  setDoCurrentYWeightAdjust(false);
}

inline void Tentacle2D::turnOnAllAdjusts()
{
  setDoDamping(true);
  setDoPrevYWeightAdjust(true);
  setDoCurrentYWeightAdjust(true);
}

inline void Tentacle2D::resetYVector()
{
  tweaker->resetYVector(ID, iterNum, xvec, yvec);
}

inline void Tentacle2D::finishIterating()
{
  startedIterating = false;
}

inline float Tentacle2D::getFirstY()
{
  return std::lerp(yvec[0], iterZeroYVal, iterZeroLerpFactor);
}

inline float Tentacle2D::getNextY(const size_t nodeNum)
{
  const double prevY = yvec[nodeNum-1];
  const double currentY = yvec[nodeNum];

  if (doCurrentYWeightAdjust) {
    tweaker->adjustWeightsFunction(ID, iterNum, nodeNum, prevY, currentY);
  }

  const float prevYWgt = doPrevYWeightAdjust ? tweaker->getNextPrevYWeight() : basePrevYWeight;
  logInfo(fmt::format("node {}: prevYWgt = {:.2}, prevYWeight = {:.2}", nodeNum, prevYWgt, basePrevYWeight));

  const float currentYWgt = doCurrentYWeightAdjust ? tweaker->getNextCurrentYWeight() : baseCurrentYWeight;
  logInfo(fmt::format("node {}: currentYWgt = {:.2}, currentYWeight = {:.2}", nodeNum, currentYWgt, baseCurrentYWeight));

  return prevYWgt*prevY + currentYWgt*currentY;
}

inline float Tentacle2D::getDampedY(const size_t nodeNum)
{
  if (!doDamping) {
    return yScale*yvec[nodeNum];
  }
  // TODO Should be able to cache damp call ???????????????????????????????????????????????????????????????????????????
  return yScale*damp(nodeNum)*yvec[nodeNum];
}

struct V3d {
  float x = 0;
  float y = 0;
  float z = 0;
  bool ignore = false;
};

class Tentacle3D {
public:
  explicit Tentacle3D(std::unique_ptr<Tentacle2D> t, const V3d& head);
  explicit Tentacle3D(std::unique_ptr<Tentacle2D> t, const TentacleColorizer& colorizer, const V3d& head);
  Tentacle3D(Tentacle3D&&);
  Tentacle3D(const Tentacle3D&)=delete;
  Tentacle3D& operator=(const Tentacle3D&)=delete;

  Tentacle2D& get2DTentacle() { return *tentacle; }
  const Tentacle2D& get2DTentacle() const { return *tentacle; }
  size_t getID() const { return tentacle->getID(); }
  uint32_t getColor(const size_t nodeNum) const;
  const V3d& getHead() const { return head; }
  void setHead(const V3d& val) { head = val; }

  std::vector<V3d> getVertices() const;
private:
  std::unique_ptr<Tentacle2D> tentacle;
  const TentacleColorizer* const colorizer;
  V3d head;
};

class Tentacles3D {
  class Iter {
  public:
    Iter(Tentacles3D* const tents, const size_t p): pos(p), tentacles(tents){}
    bool operator!=(const Iter& other) const { return pos != other.pos; }
    Tentacle3D& operator*() const;
    const Iter& operator++(){ ++pos; return *this; }
  private:
    size_t pos;
    Tentacles3D* tentacles;
  };

public:
  Tentacles3D();

  void addTentacle(std::unique_ptr<Tentacle2D> t, const V3d& h);
  void addTentacle(std::unique_ptr<Tentacle2D> t, const TentacleColorizer& colorizer, const V3d& h);

  Iter begin() { return Iter(this, 0); }
  Iter end() { return Iter(this, tentacles.size()); }

  const Tentacle3D& operator[](const size_t i) const { return tentacles[i]; }
  Tentacle3D& operator[](const size_t i) { return tentacles[i]; }
private:
  std::vector<Tentacle3D> tentacles;
};

inline Tentacle3D& Tentacles3D::Iter::operator*() const
{
  return (*tentacles)[pos];
}

#endif
