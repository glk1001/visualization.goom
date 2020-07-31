#include "tentacles_new.h"
#include "math_utils.h"

#include <fmt/format.h>
#include <utils/goom_loggers.hpp>
#include <assert.h>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>


TentacleTweaker::TentacleTweaker(
    DampingFunction* dampingFun,
    YVecResetterFunction yVecResetterFun,
    SequenceFunction* prevYWeightFun,
    SequenceFunction* currentYWeightFun,
    WeightFunctionsResetter weightFunsReset,
    WeightFunctionsAdjuster weightFunsAdj)
  : dampingFunc{ dampingFun }
  , yVecResetterFunc{ yVecResetterFun }
  , prevYWeightFunc{ prevYWeightFun }
  , currentYWeightFunc{ currentYWeightFun }
  , weightFuncsReset{ weightFunsReset }
  , weightFuncsAdjuster{ weightFunsAdj }
{
}

Tentacle2D::Tentacle2D(const size_t _ID, TentacleTweaker* twker)
  : ID(_ID)
  , tweaker{ twker }
  , xvec()
  , yvec()
  , vecs(std::make_tuple(std::ref(xvec), std::ref(yvec)))
  , dampedYVec()
  , dampingCache()
  , dampedVecs(std::make_tuple(std::ref(xvec), std::ref(dampedYVec)))
{
}

void Tentacle2D::validateSettings() const
{
  validateXDimensions();
  validateYDimensions();
  validateNumNodes();
  validateYScale();
  validatePrevYWeight();
  validateCurrentYWeight();
}

void Tentacle2D::validateXDimensions() const
{
  if (xmax <= xmin) {
    throw std::runtime_error(fmt::format("xmax must be > xmin, not ({}, {}).", xmin, xmax));
  }
}

void Tentacle2D::validateYDimensions() const
{
  if (ymax <= ymin) {
    throw std::runtime_error(fmt::format("ymax must be > ymin, not ({}, {}).", ymin, ymax));
  }
}

void Tentacle2D::validateNumNodes() const
{
  if (numNodes < 2) {
    throw std::runtime_error(fmt::format("numNodes must be >= 2, not {}.", numNodes));
  }
}

void Tentacle2D::validateYScale() const
{
  if (yScale < 0.00001) {
    throw std::runtime_error(fmt::format("yScale must be >= 0.00001, not {}.", yScale));
  }
}

void Tentacle2D::validatePrevYWeight() const
{
  if (basePrevYWeight < 0.001) {
    throw std::runtime_error(fmt::format("prevYWeight must be >= 0.001, not {}.", basePrevYWeight));
  }
}

void Tentacle2D::validateCurrentYWeight() const
{
  if (baseCurrentYWeight < 0.001) {
    throw std::runtime_error(fmt::format("currentYWeight must be >= 0.001, not {}.", baseCurrentYWeight));
  }
}

void Tentacle2D::iterateNTimes(const size_t n)
{
  startIterating();
  for (size_t i=0; i < n; i++) {
    logInfo(fmt::format("Iteration: {}", i+1));
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

  const double xstep = (xmax - xmin)/double(numNodes - 1);
  const double ystep = 100.0/double(numNodes - 1); // ?????????????????????????????????????????????????????

  xvec.resize(numNodes);
  yvec.resize(numNodes);
  dampedYVec.resize(numNodes);
  dampingCache.resize(numNodes);
  double x = xmin;
  double y = ymin;
  for(size_t i=0; i < numNodes; ++i) {
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

  resetYVector();

  yvec[0] = getFirstY();

  for(size_t i=1; i < numNodes; i++) {
    logInfo("");
    logInfo(fmt::format("iter {}, node {}, {:.2}: Before       - yvec[{}] = {:.2}, yvec[{}] = {:.2}", iterNum, i, xvec[i], i-1, yvec[i-1], i, yvec[i]));

    yvec[i] = getNextY(i);
    logInfo(fmt::format("iter {}, node {}, {:.2}: After        - yvec[{}] = {:.2}, yvec[{}] = {:.2}", iterNum, i, xvec[i], i-1, yvec[i-1], i, yvec[i]));

    dampedYVec[i] = getDampedY(i);
    logInfo(fmt::format("iter {}, node {}, {:.2}: After damp   - dvec[{}] = {:.2}, dvec[{}] = {:.2}", iterNum, i, xvec[i], i-1, dampedYVec[i-1], i, dampedYVec[i]));
  }
}

const Tentacle2D::XandYVectors& Tentacle2D::getXandYVectors() const
{
  if (std::get<0>(vecs).size() < 2) {
    throw std::runtime_error(fmt::format("getXandYVectors: xvec.size() must be >= 2, not {}.", std::get<0>(vecs).size()));
  }
  if (xvec.size() < 2) {
    throw std::runtime_error(fmt::format("getXandYVectors: xvec.size() must be >= 2, not {}.", xvec.size()));
  }
  if (yvec.size() < 2) {
    throw std::runtime_error(fmt::format("getXandYVectors: yvec.size() must be >= 2, not {}.", yvec.size()));
  }

  return vecs;
}

const Tentacle2D::XandYVectors& Tentacle2D::getDampedXandYVectors() const
{
  if (xvec.size() < 2) {
    throw std::runtime_error(fmt::format("getDampedXandYVectors: xvec.size() must be >= 2, not {}.", xvec.size()));
  }
  if (dampedYVec.size() < 2) {
    throw std::runtime_error(fmt::format("getDampedXandYVectors: yvec.size() must be >= 2, not {}.", dampedYVec.size()));
  }
  if (std::get<0>(vecs).size() < 2) {
    throw std::runtime_error(fmt::format("getDampedXandYVectors: std::get<0>(vecs).size() must be >= 2, not {}.",
        std::get<0>(vecs).size()));
  }

  return dampedVecs;
}

Tentacle3D::Tentacle3D(std::unique_ptr<Tentacle2D> t, const V3d& h)
  : tentacle{ std::move(t) }
  , colorizer{ nullptr }
  , head{ h }
{
}

Tentacle3D::Tentacle3D(std::unique_ptr<Tentacle2D> t, const TentacleColorizer& col, const V3d& h)
  : tentacle{ std::move(t) }
  , colorizer{ &col }
  , head{ h }
{
}

Tentacle3D::Tentacle3D(Tentacle3D&& o)
: tentacle{ std::move(o.tentacle) }
, colorizer{ o.colorizer }
, head{ o.head }
{
}

uint32_t Tentacle3D::getColor(const size_t nodeNum) const
{
  return colorizer->getColor(nodeNum);
}

double getMin(const std::vector<double>& vec)
{
  double vmin = std::numeric_limits<double>::max();
  for (const auto& v : vec) {
    if (v < vmin) {
      vmin = v;
    }
  }
  return vmin;
}

std::vector<V3d> Tentacle3D::getVertices() const
{
  const std::vector<double>& xvec2d = std::get<0>(tentacle->getDampedXandYVectors());
  const std::vector<double>& yvec2d = std::get<1>(tentacle->getDampedXandYVectors());
  const size_t n = xvec2d.size();

//  logInfo(fmt::format("Tentacle: {}, head.x = {}, head.y = {}, head.z = {}", "x", head.x, head.y, head.z));

  std::vector<V3d> vec3d(n);
  const float x0 = head.x;
  const float y0 = head.y - yvec2d[0];
  const float z0 = head.z - xvec2d[0];
  for (size_t i=0; i < n; i++) {
    vec3d[i].x = x0;
    vec3d[i].z = z0 + xvec2d[i];
    vec3d[i].y = y0 + yvec2d[i];
  }

  return vec3d;
}

Tentacles3D::Tentacles3D()
  : tentacles()
{
}

void Tentacles3D::addTentacle(std::unique_ptr<Tentacle2D> t, const V3d& h)
{
  tentacles.push_back(Tentacle3D(std::move(t), h));
}

void Tentacles3D::addTentacle(std::unique_ptr<Tentacle2D> t, const TentacleColorizer& colorizer, const V3d& h)
{
  tentacles.push_back(Tentacle3D(std::move(t), colorizer, h));
}
