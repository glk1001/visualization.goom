#include "tentacles_new.h"

#include "goomutils/colormap.h"
#include "goomutils/mathutils.h"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <format>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>


TentacleTweaker::TentacleTweaker(std::unique_ptr<DampingFunction> dampingFun,
                                 BeforeIterFunction beforeIterFun,
                                 SequenceFunction* prevYWeightFun,
                                 SequenceFunction* currentYWeightFun,
                                 WeightFunctionsResetter weightFunsReset,
                                 WeightFunctionsAdjuster weightFunsAdj)
  : dampingFunc{std::move(dampingFun)},
    beforeIterFunc{beforeIterFun},
    prevYWeightFunc{prevYWeightFun},
    currentYWeightFunc{currentYWeightFun},
    weightFuncsReset{weightFunsReset},
    weightFuncsAdjuster{weightFunsAdj}
{
}


Tentacle2D::Tentacle2D(const size_t _ID, std::unique_ptr<TentacleTweaker> twker)
  : ID(_ID),
    tweaker{std::move(twker)},
    xvec(),
    yvec(),
    vecs(std::make_tuple(std::ref(xvec), std::ref(yvec))),
    dampedYVec(),
    dampingCache(),
    dampedVecs(std::make_tuple(std::ref(xvec), std::ref(dampedYVec))),
    specialPostProcessingNodes{0, 0, {}}
{
}

void Tentacle2D::validateSettings() const
{
  validateXDimensions();
  validateYDimensions();
  validateNumNodes();
  validatePrevYWeight();
  validateCurrentYWeight();
}

void Tentacle2D::validateXDimensions() const
{
  if (xmax <= xmin)
  {
    throw std::runtime_error(std20::format("xmax must be > xmin, not ({}, {}).", xmin, xmax));
  }
}

void Tentacle2D::validateYDimensions() const
{
  if (ymax <= ymin)
  {
    throw std::runtime_error(std20::format("ymax must be > ymin, not ({}, {}).", ymin, ymax));
  }
}

void Tentacle2D::validateNumNodes() const
{
  if (numNodes < minNumNodes)
  {
    throw std::runtime_error(
        std20::format("numNodes must be >= {}, not {}.", minNumNodes, numNodes));
  }
}

void Tentacle2D::validatePrevYWeight() const
{
  if (basePrevYWeight < 0.001)
  {
    throw std::runtime_error(
        std20::format("prevYWeight must be >= 0.001, not {}.", basePrevYWeight));
  }
}

void Tentacle2D::validateCurrentYWeight() const
{
  if (baseCurrentYWeight < 0.001)
  {
    throw std::runtime_error(
        std20::format("currentYWeight must be >= 0.001, not {}.", baseCurrentYWeight));
  }
}

inline double Tentacle2D::damp(const size_t nodeNum) const
{
  return dampingCache[nodeNum];
}

void Tentacle2D::iterateNTimes(const size_t n)
{
  startIterating();
  for (size_t i = 0; i < n; i++)
  {
    //    logInfo("Iteration: {}", i+1);
    iterate();
  }
  finishIterating();
}

void Tentacle2D::startIterating()
{
  validateSettings();

  tweaker->resetWeightsFunction(ID, numNodes, basePrevYWeight, baseCurrentYWeight);

  startedIterating = true;
  iterNum = 0;

  const double xstep = (xmax - xmin) / static_cast<double>(numNodes - 1);
  const double ystep =
      100.0 / static_cast<double>(numNodes - 1);// ?????????????????????????????????????????????????????

  xvec.resize(numNodes);
  yvec.resize(numNodes);
  dampedYVec.resize(numNodes);
  dampingCache.resize(numNodes);
  double x = xmin;
  double y = ymin;
  for (size_t i = 0; i < numNodes; ++i)
  {
    xvec[i] = x;
    yvec[i] = 0;
    dampingCache[i] = tweaker->getDamping(x);

    x += xstep;
    y += ystep;
  }
}

void Tentacle2D::iterate()
{
  iterNum++;

  beforeIter();

  yvec[0] = getFirstY();
  for (size_t i = 1; i < numNodes; i++)
  {
    yvec[i] = getNextY(i);
  }

  if (!doPostProcessing)
  {
    updateDampedVals(yvec);
  }
  else
  {
    updateDampedVals(postProcessYVals(yvec));
  }

  specialPostProcessingNodes.incNodes();
}

std::vector<double> Tentacle2D::postProcessYVals(const std::vector<double>& yvec) const
{
  std::vector<double> yvals(numNodes);
  for (size_t i = 0; i < numNodes; i++)
  {
    const double y = yvec[i];
    if (y == 0.0)
    {
      yvals[i] = y;
    }
    else
    {
      //      const double sign = std::signbit(y) ? -1.0 : +1.0;
      //      yvals[i] = sign*std::pow(std::fabs(y), 1.4);
      if (!specialPostProcessingNodes.isSpecialNode(i))
      {
        yvals[i] = y;
      }
      else
      {
        yvals[i] = y + getRandInRange(-0.1f, 0.1f);
      }
    }
  }
  return yvals;
}

void Tentacle2D::updateDampedVals(const std::vector<double>& yvals)
{
  constexpr size_t numSmoothNodes = std::min(10ul, minNumNodes);
  const auto tSmooth = [](const double t) { return t * (2 - t); };

  const double tStep = 1.0 / (numSmoothNodes - 1);
  double tNext = tStep;
  dampedYVec[0] = 0;
  for (size_t i = 1; i < numSmoothNodes; i++)
  {
    const double t = tSmooth(tNext);
    dampedYVec[i] = std::lerp(dampedYVec[i - 1], double(getDampedVal(i, yvals[i])), t);
    tNext += tStep;
  }

  for (size_t i = numSmoothNodes; i < numNodes; i++)
  {
    dampedYVec[i] = getDampedVal(i, yvals[i]);
  }
}

const Tentacle2D::XandYVectors& Tentacle2D::getXandYVectors() const
{
  if (std::get<0>(vecs).size() < 2)
  {
    throw std::runtime_error(std20::format("getXandYVectors: xvec.size() must be >= 2, not {}.",
                                            std::get<0>(vecs).size()));
  }
  if (xvec.size() < 2)
  {
    throw std::runtime_error(
        std20::format("getXandYVectors: xvec.size() must be >= 2, not {}.", xvec.size()));
  }
  if (yvec.size() < 2)
  {
    throw std::runtime_error(
        std20::format("getXandYVectors: yvec.size() must be >= 2, not {}.", yvec.size()));
  }

  return vecs;
}

inline float Tentacle2D::getFirstY()
{
  return std::lerp(yvec[0], iterZeroYVal, iterZeroLerpFactor);
}

inline float Tentacle2D::getNextY(const size_t nodeNum)
{
  const double prevY = yvec[nodeNum - 1];
  const double currentY = yvec[nodeNum];

  if (doCurrentYWeightAdjust)
  {
    tweaker->adjustWeightsFunction(ID, iterNum, nodeNum, prevY, currentY);
  }

  const float prevYWgt = doPrevYWeightAdjust ? tweaker->getNextPrevYWeight() : basePrevYWeight;
  //  logInfo("node {}: prevYWgt = {:.2}, prevYWeight = {:.2}", nodeNum, prevYWgt, basePrevYWeight);

  const float currentYWgt =
      doCurrentYWeightAdjust ? tweaker->getNextCurrentYWeight() : baseCurrentYWeight;
  //  logInfo("node {}: currentYWgt = {:.2}, currentYWeight = {:.2}", nodeNum, currentYWgt, baseCurrentYWeight);

  return prevYWgt * prevY + currentYWgt * currentY;
}

inline float Tentacle2D::getDampedVal(const size_t nodeNum, const float val) const
{
  if (!doDamping)
  {
    return val;
  }
  return damp(nodeNum) * val;
}

const Tentacle2D::XandYVectors& Tentacle2D::getDampedXandYVectors() const
{
  if (xvec.size() < 2)
  {
    throw std::runtime_error(
        std20::format("getDampedXandYVectors: xvec.size() must be >= 2, not {}.", xvec.size()));
  }
  if (dampedYVec.size() < 2)
  {
    throw std::runtime_error(std20::format(
        "getDampedXandYVectors: yvec.size() must be >= 2, not {}.", dampedYVec.size()));
  }
  if (std::get<0>(vecs).size() < 2)
  {
    throw std::runtime_error(
        std20::format("getDampedXandYVectors: std::get<0>(vecs).size() must be >= 2, not {}.",
                       std::get<0>(vecs).size()));
  }

  return dampedVecs;
}

Tentacle3D::Tentacle3D(std::unique_ptr<Tentacle2D> t,
                       const uint32_t headCol,
                       const uint32_t headColLow,
                       const V3d& h)
  : specialColorNodes { 0, tentacle->getNumNodes(), { } },
    tentacle { std::move(t) },
    head { h },
    colorizer { nullptr },
    specialNodesColorMap { nullptr },
    headColor { headCol },
    headColorLow { headColLow }
{
}

Tentacle3D::Tentacle3D(std::unique_ptr<Tentacle2D> t,
                       const TentacleColorizer& col,
                       const uint32_t headCol,
                       const uint32_t headColLow,
                       const V3d& h)
  : tentacle{std::move(t)},
    colorizer{&col},
    specialColorNodes{0, tentacle->getNumNodes(), {}},
    specialNodesColorMap{nullptr},
    headColor{headCol},
    headColorLow{headColLow},
    head{h}
{
}

Tentacle3D::Tentacle3D(Tentacle3D&& o)
  : tentacle{std::move(o.tentacle)},
    colorizer{o.colorizer},
    specialColorNodes{o.specialColorNodes},
    specialNodesColorMap{o.specialNodesColorMap},
    headColor{ o.headColor },
    headColorLow{ o.headColorLow },
    head{ o.head }
{
}

void Tentacle3D::setSpecialColorNodes(const ColorMap& cm, const std::vector<size_t>& nodes)
{
  specialNodesColorMap = &cm;
  specialColorNodes = SpecialNodes{0, tentacle->getNumNodes(), nodes};
  specialColorNodes.setIncAmount(5);
}

uint32_t Tentacle3D::getColor(const size_t nodeNum) const
{
  return colorizer->getColor(nodeNum);
}

std::tuple<uint32_t, uint32_t> Tentacle3D::getMixedColors(const size_t nodeNum,
                                                          const uint32_t color,
                                                          const uint32_t colorLow) const
{
  //  if (specialColorNodes.isSpecialNode(nodeNum)) {
  //    const size_t colorNum = specialNodesColorMap->numColors()*nodeNum/get2DTentacle().getNumNodes();
  //    const uint32_t specialColor = specialNodesColorMap->getColor(colorNum);
  //    return std::make_tuple(specialColor, specialColor);
  //  }

  float t = float(nodeNum + 1) / float(get2DTentacle().getNumNodes());
  if (reverseColorMix)
  {
    t = 1 - t;
  }
  const uint32_t segmentColor = getColor(nodeNum);
  const uint32_t mixedColor = ColorMap::colorMix(color, segmentColor, t);
  const uint32_t mixedColorLow = ColorMap::colorMix(colorLow, segmentColor, t);

  if (nodeNum < Tentacle2D::minNumNodes)
  {
    const float t = 0.5 * (1.0 + float(nodeNum + 1) / float(Tentacle2D::minNumNodes + 1));
    const uint32_t mixedHeadColor = ColorMap::colorMix(headColor, color, t);
    const uint32_t mixedHeadColorLow = ColorMap::colorMix(headColorLow, colorLow, t);
    return std::make_tuple(mixedHeadColor, mixedHeadColorLow);
  }

  return std::make_tuple(mixedColor, mixedColorLow);
}

double getMin(const std::vector<double>& vec)
{
  double vmin = std::numeric_limits<double>::max();
  for (const auto& v : vec)
  {
    if (v < vmin)
    {
      vmin = v;
    }
  }
  return vmin;
}

std::vector<V3d> Tentacle3D::getVertices() const
{
  const auto [xvec2d, yvec2d] = tentacle->getDampedXandYVectors();
  const size_t n = xvec2d.size();

  //  logInfo("Tentacle: {}, head.x = {}, head.y = {}, head.z = {}", "x", head.x, head.y, head.z);

  std::vector<V3d> vec3d(n);
  const float x0 = head.x;
  const float y0 = head.y - yvec2d[0];
  const float z0 = head.z - xvec2d[0];
  for (size_t i = 0; i < n; i++)
  {
    vec3d[i].x = x0;
    vec3d[i].z = z0 + xvec2d[i];
    vec3d[i].y = y0 + yvec2d[i];
  }

  return vec3d;
}

Tentacles3D::Tentacles3D() : tentacles()
{
}

void Tentacles3D::addTentacle(Tentacle3D&& t)
{
  tentacles.push_back(std::move(t));
}

SpecialNodes::SpecialNodes(const size_t v0, const size_t v1, const std::vector<size_t>& nods)
  : minVal(v0), maxVal(v1), incAmount{1}, incDirection{+1}, nodes{nods}
{
}

void SpecialNodes::clear()
{
  nodes.clear();
}

bool SpecialNodes::isSpecialNode(const size_t checkNode) const
{
  for (const size_t i : nodes)
  {
    if (i == checkNode)
    {
      return true;
    }
  }
  return false;
}

void SpecialNodes::incNodes()
{
  if (incDirection > 0)
  {
    if (static_cast<long>(nodes[nodes.size() - 1] + incAmount) > static_cast<long>(maxVal))
    {
      incDirection = -1;
      return;
    }
  }
  else if (incDirection < 0)
  {
    if (static_cast<long>(nodes[0] - incAmount) < static_cast<long>(minVal))
    {
      incDirection = +1;
      return;
    }
  }

  for (size_t& i : nodes)
  {
    i = static_cast<size_t>(static_cast<long>(i) + incDirection * static_cast<long>(incAmount));
  }
}
