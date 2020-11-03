#include "tentacles.h"

#include "colorutils.h"
#include "goom_graphic.h"
#include "goomutils/colormap.h"
#include "goomutils/goomrand.h"
#include "goomutils/mathutils.h"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <format>
#include <limits>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace goom
{

using namespace goom::utils;


Tentacle2D::Tentacle2D(const size_t _ID,
                       const size_t _numNodes,
                       const double _xmin,
                       const double _xmax,
                       const double _ymin,
                       const double _ymax,
                       const double _basePrevYWeight,
                       const double _baseCurrentYWeight) noexcept
  : ID{_ID},
    numNodes{_numNodes},
    xmin{_xmin},
    xmax{_xmax},
    ymin{_ymin},
    ymax{_ymax},
    basePrevYWeight{_basePrevYWeight},
    baseCurrentYWeight{_baseCurrentYWeight},
    dampingFunc{createDampingFunc(basePrevYWeight, xmin, xmax)}
{
}

void Tentacle2D::setXDimensions(const double x0, const double x1)
{
  if (startedIterating)
  {
    throw std::runtime_error("Can't set x dimensions after iteration start.");
  }

  if (std::fabs(xmin - x0) < 0.00001 && std::fabs(xmax - x1) < 0.00001)
  {
    return;
  }

  xmin = x0;
  xmax = x1;
  validateXDimensions();

  dampingFunc = createDampingFunc(basePrevYWeight, xmin, xmax);
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
    iterate();
  }
  finishIterating();
}

void Tentacle2D::startIterating()
{
  validateSettings();

  startedIterating = true;
  iterNum = 0;

  const double xstep = (xmax - xmin) / static_cast<double>(numNodes - 1);
  const double ystep =
      100.0 /
      static_cast<double>(numNodes - 1); // ?????????????????????????????????????????????????????

  xvec.resize(numNodes);
  yvec.resize(numNodes);
  dampedYVec.resize(numNodes);
  dampingCache.resize(numNodes);
  double x = xmin;
  double y = ymin;
  for (size_t i = 0; i < numNodes; ++i)
  {
    dampingCache[i] = getDamping(x);
    xvec[i] = x;
    yvec[i] = 0.1 * dampingCache[i];

    x += xstep;
    y += ystep;
  }
}

void Tentacle2D::iterate()
{
  iterNum++;

  yvec[0] = getFirstY();
  for (size_t i = 1; i < numNodes; i++)
  {
    yvec[i] = getNextY(i);
  }

  updateDampedVals(yvec);
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
    dampedYVec[i] = std::lerp(dampedYVec[i - 1], static_cast<double>(getDampedVal(i, yvals[i])), t);
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

  return basePrevYWeight * prevY + baseCurrentYWeight * currentY;
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

Tentacle2D::DampingFuncPtr Tentacle2D::createDampingFunc(const double prevYWeight,
                                                         const double xmin,
                                                         const double xmax)
{
  if (prevYWeight < 0.6)
  {
    return createLinearDampingFunc(xmin, xmax);
  }
  return createExpDampingFunc(xmin, xmax);
}

Tentacle2D::DampingFuncPtr Tentacle2D::createExpDampingFunc(const double xmin, const double xmax)
{
  const double xRiseStart = xmin + 0.25 * xmax;
  constexpr double dampStart = 5;
  constexpr double dampMax = 30;

  return DampingFuncPtr{new ExpDampingFunction{0.1, xRiseStart, dampStart, xmax, dampMax}};
}

Tentacle2D::DampingFuncPtr Tentacle2D::createLinearDampingFunc(const double xmin, const double xmax)
{
  constexpr float yScale = 30;

  std::vector<std::tuple<double, double, DampingFuncPtr>> pieces{};
  pieces.emplace_back(
      std::make_tuple(xmin, 0.1 * xmax, DampingFuncPtr{new FlatDampingFunction{0.1}}));
  pieces.emplace_back(
      std::make_tuple(0.1 * xmax, 10 * xmax,
                      DampingFuncPtr{new LinearDampingFunction{0.1 * xmax, 0.1, xmax, yScale}}));

  return DampingFuncPtr{new PiecewiseDampingFunction{pieces}};
}

Tentacle3D::Tentacle3D(std::unique_ptr<Tentacle2D> t,
                       const Pixel& headCol,
                       const Pixel& headColLow,
                       const V3d& h) noexcept
  : tentacle{std::move(t)},
    headColor{headCol},
    headColorLow{headColLow},
    head{h}
{
}

Tentacle3D::Tentacle3D(std::unique_ptr<Tentacle2D> t,
                       const TentacleColorizer& col,
                       const Pixel& headCol,
                       const Pixel& headColLow,
                       const V3d& h) noexcept
  : tentacle{std::move(t)},
    colorizer{&col},
    headColor{headCol},
    headColorLow{headColLow},
    head{h}
{
}

Tentacle3D::Tentacle3D(Tentacle3D&& o) noexcept
  : tentacle{std::move(o.tentacle)},
    colorizer{o.colorizer},
    headColor{o.headColor},
    headColorLow{o.headColorLow},
    head{o.head}
{
}

Pixel Tentacle3D::getColor(const size_t nodeNum) const
{
  return colorizer->getColor(nodeNum);
}

std::tuple<Pixel, Pixel> Tentacle3D::getMixedColors(const size_t nodeNum,
                                                    const Pixel& color,
                                                    const Pixel& colorLow) const
{
  if (nodeNum < Tentacle2D::minNumNodes)
  {
    // Color the tentacle head
    const float t = 0.5 * (1.0 + static_cast<float>(nodeNum + 1) /
                                     static_cast<float>(Tentacle2D::minNumNodes + 1));
    const Pixel mixedHeadColor = ColorMap::colorMix(headColor, color, t);
    const Pixel mixedHeadColorLow = ColorMap::colorMix(headColorLow, colorLow, t);
    return std::make_tuple(mixedHeadColor, mixedHeadColorLow);
  }

  float t = static_cast<float>(nodeNum + 1) / static_cast<float>(get2DTentacle().getNumNodes());
  if (reverseColorMix)
  {
    t = 1 - t;
  }

  const Pixel segmentColor = getColor(nodeNum);
  const Pixel mixedColor = ColorMap::colorMix(color, segmentColor, t);
  const Pixel mixedColorLow = ColorMap::colorMix(colorLow, segmentColor, t);
  return std::make_tuple(mixedColor, mixedColorLow);
}

std::tuple<Pixel, Pixel> Tentacle3D::getMixedColors(const size_t nodeNum,
                                                    const Pixel& color,
                                                    const Pixel& colorLow,
                                                    const float brightness) const
{
  if (nodeNum < Tentacle2D::minNumNodes)
  {
    return getMixedColors(nodeNum, color, colorLow);
  }

  const auto [mixedColor, mixedColorLow] = getMixedColors(nodeNum, color, colorLow);
  const Pixel mixedColorPixel = mixedColor;
  const Pixel mixedColorLowPixel = mixedColorLow;
  return std::make_tuple(getBrighterColor(brightness, mixedColorPixel, allowOverexposed),
                         getBrighterColor(brightness, mixedColorLowPixel, allowOverexposed));
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

Tentacles3D::Tentacles3D() noexcept : tentacles()
{
}

void Tentacles3D::addTentacle(Tentacle3D&& t)
{
  tentacles.push_back(std::move(t));
}

void Tentacles3D::setAllowOverexposed(const bool val)
{
  for (auto& t : tentacles)
  {
    t.setAllowOverexposed(val);
  }
}

} // namespace goom
