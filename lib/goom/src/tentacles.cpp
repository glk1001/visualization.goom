#include "tentacles.h"

#include "goom_graphic.h"
#include "goomutils/colormaps.h"
#include "goomutils/colorutils.h"
#include "goomutils/logging_control.h"
//#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"

#include <cmath>
#include <format>
#include <limits>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace GOOM
{

using namespace GOOM::UTILS;

Tentacle2D::Tentacle2D() noexcept = default;

Tentacle2D::Tentacle2D(const size_t id,
                       const size_t numNodes,
                       const double xmin,
                       const double xmax,
                       const double ymin,
                       const double ymax,
                       const double basePrevYWeight,
                       const double baseCurrentYWeight) noexcept
  : m_id{id},
    m_numNodes{numNodes},
    m_xmin{xmin},
    m_xmax{xmax},
    m_ymin{ymin},
    m_ymax{ymax},
    m_basePrevYWeight{basePrevYWeight},
    m_baseCurrentYWeight{baseCurrentYWeight},
    m_dampingFunc{CreateDampingFunc(m_basePrevYWeight, m_xmin, m_xmax)}
{
}

auto Tentacle2D::operator==(const Tentacle2D& t) const -> bool
{
  const bool result =
      m_id == t.m_id && m_numNodes == t.m_numNodes && m_xmin == t.m_xmin && m_xmax == t.m_xmax &&
      m_ymin == t.m_ymin && m_ymax == t.m_ymax && m_basePrevYWeight == t.m_basePrevYWeight &&
      m_baseCurrentYWeight == t.m_baseCurrentYWeight && m_iterZeroYVal == t.m_iterZeroYVal &&
      m_iterZeroLerpFactor == t.m_iterZeroLerpFactor && m_iterNum == t.m_iterNum &&
      m_startedIterating == t.m_startedIterating && m_xvec == t.m_xvec && m_yvec == t.m_yvec &&
      // vecs{std::make_tuple(std::ref(xvec), std::ref(yvec))};
      m_dampedYVec == t.m_dampedYVec && m_dampingCache == t.m_dampingCache &&
      // dampedVecs{std::make_tuple(std::ref(xvec), std::ref(dampedYVec))};
      //    *dampingFunc == *t.dampingFunc &&
      m_doDamping == t.m_doDamping;

  if (!result)
  {
    logInfo("Tentacle2D result == {}", result);
    logInfo("ID = {}, t.ID = {}", m_id, t.m_id);
    logInfo("numNodes = {}, t.numNodes = {}", m_numNodes, t.m_numNodes);
    logInfo("xmin = {}, t.xmin = {}", m_xmin, t.m_xmin);
    logInfo("xmax = {}, t.xmax = {}", m_xmax, t.m_xmax);
    logInfo("ymin = {}, t.ymin = {}", m_ymin, t.m_ymin);
    logInfo("ymax = {}, t.ymax = {}", m_ymax, t.m_ymax);
    logInfo("basePrevYWeight = {}, t.basePrevYWeight = {}", m_basePrevYWeight, t.m_basePrevYWeight);
    logInfo("baseCurrentYWeight = {}, t.baseCurrentYWeight = {}", m_baseCurrentYWeight,
            t.m_baseCurrentYWeight);
    logInfo("iterZeroYVal = {}, t.iterZeroYVal = {}", m_iterZeroYVal, t.m_iterZeroYVal);
    logInfo("iterZeroLerpFactor = {}, t.iterZeroLerpFactor = {}", m_iterZeroLerpFactor,
            t.m_iterZeroLerpFactor);
    logInfo("iterNum = {}, t.iterNum = {}", m_iterNum, t.m_iterNum);
    logInfo("startedIterating = {}, t.startedIterating = {}", m_startedIterating,
            t.m_startedIterating);
    logInfo("xvec == t.xvec = {}", m_xvec == t.m_xvec);
    logInfo("yvec == t.yvec = {}", m_yvec == t.m_yvec);
    logInfo("dampedYVec == t.dampedYVec = {}", m_dampedYVec == t.m_dampedYVec);
    logInfo("dampingCache == t.dampingCache = {}", m_dampingCache == t.m_dampingCache);
    logInfo("doDamping = {}, t.doDamping = {}", m_doDamping, t.m_doDamping);
  }

  return result;
}

void Tentacle2D::SetXDimensions(const double x0, const double x1)
{
  if (m_startedIterating)
  {
    throw std::runtime_error("Can't set x dimensions after iteration start.");
  }

  m_xmin = x0;
  m_xmax = x1;
  ValidateXDimensions();

  m_dampingFunc = CreateDampingFunc(m_basePrevYWeight, m_xmin, m_xmax);
}

void Tentacle2D::ValidateSettings() const
{
  ValidateXDimensions();
  ValidateYDimensions();
  ValidateNumNodes();
  ValidatePrevYWeight();
  ValidateCurrentYWeight();
}

void Tentacle2D::ValidateXDimensions() const
{
  if (m_xmax <= m_xmin)
  {
    throw std::runtime_error(std20::format("xmax must be > xmin, not ({}, {}).", m_xmin, m_xmax));
  }
}

void Tentacle2D::ValidateYDimensions() const
{
  if (m_ymax <= m_ymin)
  {
    throw std::runtime_error(std20::format("ymax must be > ymin, not ({}, {}).", m_ymin, m_ymax));
  }
}

void Tentacle2D::ValidateNumNodes() const
{
  if (m_numNodes < MIN_NUM_NODES)
  {
    throw std::runtime_error(
        std20::format("numNodes must be >= {}, not {}.", MIN_NUM_NODES, m_numNodes));
  }
}

void Tentacle2D::ValidatePrevYWeight() const
{
  if (m_basePrevYWeight < 0.001)
  {
    throw std::runtime_error(
        std20::format("prevYWeight must be >= 0.001, not {}.", m_basePrevYWeight));
  }
}

void Tentacle2D::ValidateCurrentYWeight() const
{
  if (m_baseCurrentYWeight < 0.001)
  {
    throw std::runtime_error(
        std20::format("currentYWeight must be >= 0.001, not {}.", m_baseCurrentYWeight));
  }
}

inline auto Tentacle2D::Damp(size_t nodeNum) const -> double
{
  return m_dampingCache[nodeNum];
}

void Tentacle2D::IterateNTimes(size_t n)
{
  StartIterating();
  for (size_t i = 0; i < n; i++)
  {
    Iterate();
  }
  FinishIterating();
}

void Tentacle2D::StartIterating()
{
  ValidateSettings();

  m_startedIterating = true;
  m_iterNum = 0;

  const double xstep = (m_xmax - m_xmin) / static_cast<double>(m_numNodes - 1);
  const double ystep =
      100.0 /
      static_cast<double>(m_numNodes - 1); // ?????????????????????????????????????????????????????

  m_xvec.resize(m_numNodes);
  m_yvec.resize(m_numNodes);
  m_dampedYVec.resize(m_numNodes);
  m_dampingCache.resize(m_numNodes);
  double x = m_xmin;
  double y = m_ymin;
  for (size_t i = 0; i < m_numNodes; ++i)
  {
    m_dampingCache[i] = GetDamping(x);
    m_xvec[i] = x;
    m_yvec[i] = 0.1 * m_dampingCache[i];

    x += xstep;
    y += ystep;
  }
}

void Tentacle2D::Iterate()
{
  m_iterNum++;

  m_yvec[0] = GetFirstY();
  for (size_t i = 1; i < m_numNodes; i++)
  {
    m_yvec[i] = GetNextY(i);
  }

  UpdateDampedVals(m_yvec);
}

void Tentacle2D::UpdateDampedVals(const std::vector<double>& yvals)
{
  constexpr size_t NUM_SMOOTH_NODES = std::min(10UL, MIN_NUM_NODES);
  const auto tSmooth = [](const double t) { return t * (2.0 - t); };

  const double tStep = 1.0 / (NUM_SMOOTH_NODES - 1);
  double tNext = tStep;
  m_dampedYVec[0] = 0.0;
  for (size_t i = 1; i < NUM_SMOOTH_NODES; i++)
  {
    const double t = tSmooth(tNext);
    m_dampedYVec[i] =
        std::lerp(m_dampedYVec[i - 1], static_cast<double>(GetDampedVal(i, yvals[i])), t);
    tNext += tStep;
  }

  for (size_t i = NUM_SMOOTH_NODES; i < m_numNodes; i++)
  {
    m_dampedYVec[i] = GetDampedVal(i, yvals[i]);
  }
}

auto Tentacle2D::GetXAndYVectors() const -> const Tentacle2D::XAndYVectors&
{
  if (std::get<0>(m_vecs).size() < 2)
  {
    throw std::runtime_error(std20::format("GetXAndYVectors: xvec.size() must be >= 2, not {}.",
                                           std::get<0>(m_vecs).size()));
  }
  if (m_xvec.size() < 2)
  {
    throw std::runtime_error(
        std20::format("GetXAndYVectors: xvec.size() must be >= 2, not {}.", m_xvec.size()));
  }
  if (m_yvec.size() < 2)
  {
    throw std::runtime_error(
        std20::format("GetXAndYVectors: yvec.size() must be >= 2, not {}.", m_yvec.size()));
  }

  return m_vecs;
}

inline auto Tentacle2D::GetFirstY() -> float
{
  return std::lerp(m_yvec[0], m_iterZeroYVal, m_iterZeroLerpFactor);
}

inline auto Tentacle2D::GetNextY(size_t nodeNum) -> float
{
  const double prevY = m_yvec[nodeNum - 1];
  const double currentY = m_yvec[nodeNum];

  return m_basePrevYWeight * prevY + m_baseCurrentYWeight * currentY;
}

inline auto Tentacle2D::GetDampedVal(size_t nodeNum, float val) const -> float
{
  if (!m_doDamping)
  {
    return val;
  }
  return Damp(nodeNum) * val;
}

auto Tentacle2D::GetDampedXAndYVectors() const -> const Tentacle2D::XAndYVectors&
{
  if (m_xvec.size() < 2)
  {
    throw std::runtime_error(
        std20::format("GetDampedXAndYVectors: xvec.size() must be >= 2, not {}.", m_xvec.size()));
  }
  if (m_dampedYVec.size() < 2)
  {
    throw std::runtime_error(std20::format(
        "GetDampedXAndYVectors: yvec.size() must be >= 2, not {}.", m_dampedYVec.size()));
  }
  if (std::get<0>(m_vecs).size() < 2)
  {
    throw std::runtime_error(
        std20::format("GetDampedXAndYVectors: std::get<0>(vecs).size() must be >= 2, not {}.",
                      std::get<0>(m_vecs).size()));
  }

  return m_dampedVecs;
}

auto Tentacle2D::CreateDampingFunc(const double prevYWeight, const double xmin, const double xmax)
    -> Tentacle2D::DampingFuncPtr
{
  if (prevYWeight < 0.6)
  {
    return CreateLinearDampingFunc(xmin, xmax);
  }
  return CreateExpDampingFunc(xmin, xmax);
}

auto Tentacle2D::CreateExpDampingFunc(const double xmin, const double xmax)
    -> Tentacle2D::DampingFuncPtr
{
  const double xRiseStart = xmin + 0.25 * xmax;
  constexpr double DAMP_START = 5.0;
  constexpr double DAMP_MAX = 30.0;

  return DampingFuncPtr{new ExpDampingFunction{0.1, xRiseStart, DAMP_START, xmax, DAMP_MAX}};
}

auto Tentacle2D::CreateLinearDampingFunc(const double xmin, const double xmax)
    -> Tentacle2D::DampingFuncPtr
{
  constexpr float Y_SCALE = 30.0;

  std::vector<std::tuple<double, double, DampingFuncPtr>> pieces{};
  (void)pieces.emplace_back(
      std::make_tuple(xmin, 0.1 * xmax, DampingFuncPtr{new FlatDampingFunction{0.1}}));
  (void)pieces.emplace_back(
      std::make_tuple(0.1 * xmax, 10 * xmax,
                      DampingFuncPtr{new LinearDampingFunction{0.1 * xmax, 0.1, xmax, Y_SCALE}}));

  return DampingFuncPtr{new PiecewiseDampingFunction{pieces}};
}

Tentacle3D::Tentacle3D(std::unique_ptr<Tentacle2D> t,
                       const Pixel& headCol,
                       const Pixel& headColLow,
                       const V3dFlt& h,
                       const size_t numHdNodes) noexcept
  : m_tentacle{std::move(t)},
    m_headColor{headCol},
    m_headColorLow{headColLow},
    m_head{h},
    m_numHeadNodes{numHdNodes}
{
}

Tentacle3D::Tentacle3D(std::unique_ptr<Tentacle2D> t,
                       std::shared_ptr<const ITentacleColorizer> col,
                       const Pixel& headCol,
                       const Pixel& headColLow,
                       const V3dFlt& h,
                       const size_t numHdNodes) noexcept
  : m_tentacle{std::move(t)},
    m_colorizer{std::move(col)},
    m_headColor{headCol},
    m_headColorLow{headColLow},
    m_head{h},
    m_numHeadNodes{numHdNodes}
{
}

Tentacle3D::Tentacle3D(Tentacle3D&& o) noexcept
  : m_tentacle{std::move(o.m_tentacle)},
    m_colorizer{o.m_colorizer},
    m_headColor{o.m_headColor},
    m_headColorLow{o.m_headColorLow},
    m_head{o.m_head},
    m_numHeadNodes{o.m_numHeadNodes}
{
}

auto Tentacle3D::operator==(const Tentacle3D& t) const -> bool
{
  const bool result = *m_tentacle == *t.m_tentacle && m_headColor == t.m_headColor &&
                      m_headColorLow == t.m_headColorLow && m_head == t.m_head &&
                      m_numHeadNodes == t.m_numHeadNodes &&
                      m_reverseColorMix == t.m_reverseColorMix &&
                      m_allowOverexposed == t.m_allowOverexposed;

  if (result)
  {
    if (typeid(*m_colorizer) != typeid(*t.m_colorizer))
    {
      logInfo("Tentacle3D colorizer not of same type");
      return false;
    }
    /**
    const TentacleColorMapColorizer* c1 =
        dynamic_cast<const TentacleColorMapColorizer*>(colorizer.get());
    if (c1 == nullptr)
    {
      continue;
    }
    const TentacleColorMapColorizer* c2 =
        dynamic_cast<const TentacleColorMapColorizer*>(t.colorizer.get());
    if (c2 == nullptr)
    {
      logInfo("Tentacle3D colorizer not of same type");
      return false;
    }
    if (*c1 != *c2)
    {
      logInfo("Tentacle3D colorizer not the same");
      return false;
    }
    **/
  }

  if (!result)
  {
    logInfo("Tentacle3D result == {}", result);
    logInfo("*tentacle == *t.tentacle = {}", *m_tentacle == *t.m_tentacle);
    //    logInfo("*colorizer == *t.colorizer = {}", *colorizer == *t.colorizer);
    logInfo("headColor = {}, t.headColor = {}", m_headColor.Rgba(), t.m_headColor.Rgba());
    logInfo("headColorLow = {}, t.headColorLow = {}", m_headColorLow.Rgba(),
            t.m_headColorLow.Rgba());
    logInfo("head == t.head = {}", m_head == t.m_head);
    logInfo("reverseColorMix = {}, t.reverseColorMix = {}", m_reverseColorMix, t.m_reverseColorMix);
    logInfo("allowOverexposed = {}, t.allowOverexposed = {}", m_allowOverexposed,
            t.m_allowOverexposed);
  }

  return result;
}

auto Tentacle3D::GetColor(size_t nodeNum) const -> Pixel
{
  return m_colorizer->GetColor(nodeNum);
}

auto Tentacle3D::GetMixedColors(size_t nodeNum, const Pixel& color, const Pixel& colorLow) const
    -> std::tuple<Pixel, Pixel>
{
  if (nodeNum < GetNumHeadNodes())
  {
    // Color the tentacle head
    const float t =
        0.5F * (1.0F + static_cast<float>(nodeNum + 1) / static_cast<float>(GetNumHeadNodes() + 1));
    const Pixel mixedHeadColor = IColorMap::GetColorMix(m_headColor, color, t);
    const Pixel mixedHeadColorLow = IColorMap::GetColorMix(m_headColorLow, colorLow, t);
    return std::make_tuple(mixedHeadColor, mixedHeadColorLow);
  }

  float t = static_cast<float>(nodeNum + 1) / static_cast<float>(Get2DTentacle().GetNumNodes());
  if (m_reverseColorMix)
  {
    t = 1 - t;
  }

  const Pixel segmentColor = GetColor(nodeNum);
  const Pixel mixedColor = IColorMap::GetColorMix(color, segmentColor, t);
  const Pixel mixedColorLow = IColorMap::GetColorMix(colorLow, segmentColor, t);

  if (std::abs(GetHead().x) < 10)
  {
    const float brightnessCut = t * t;
    return std::make_tuple(GetBrighterColor(brightnessCut, mixedColor, true),
                           GetBrighterColor(brightnessCut, mixedColorLow, true));
  }

  return std::make_tuple(mixedColor, mixedColorLow);
}

auto Tentacle3D::GetMixedColors(size_t nodeNum,
                                const Pixel& color,
                                const Pixel& colorLow,
                                float brightness) const -> std::tuple<Pixel, Pixel>
{
  if (nodeNum < GetNumHeadNodes())
  {
    return GetMixedColors(nodeNum, color, colorLow);
  }

  const auto [mixedColor, mixedColorLow] = GetMixedColors(nodeNum, color, colorLow);
  const Pixel mixedColorPixel = mixedColor;
  const Pixel mixedColorLowPixel = mixedColorLow;
  //constexpr float gamma = 4.2;
  //const float brightnessWithGamma = std::pow(brightness, 1.0F / gamma);
  const float brightnessWithGamma = brightness;
  //  const float brightnessWithGamma =
  //      std::max(0.4F, std::pow(brightness * getLuma(color), 1.0F / gamma));
  //  const float brightnessWithGammaLow =
  //      std::max(0.4F, std::pow(brightness * getLuma(colorLow), 1.0F / gamma));
  return std::make_tuple(
      GetBrighterColor(brightnessWithGamma, mixedColorPixel, m_allowOverexposed),
      GetBrighterColor(brightnessWithGamma, mixedColorLowPixel, m_allowOverexposed));
}

auto Tentacle3D::GetVertices() const -> std::vector<V3dFlt>
{
  const auto [xvec2D, yvec2D] = m_tentacle->GetDampedXAndYVectors();
  const size_t n = xvec2D.size();

  //  logInfo("Tentacle: {}, head.x = {}, head.y = {}, head.z = {}", "x", head.x, head.y, head.z);

  std::vector<V3dFlt> vec3d(n);
  const float x0 = m_head.x;
  const float y0 = m_head.y - yvec2D[0];
  const float z0 = m_head.z - xvec2D[0];
  float xStep = 0.0;
  if (std::abs(x0) < 10.0)
  {
    const float xn = 0.1 * x0;
    xStep = (x0 - xn) / static_cast<float>(n);
  }
  float x = x0;
  for (size_t i = 0; i < n; i++)
  {
    vec3d[i].x = x;
    vec3d[i].z = z0 + xvec2D[i];
    vec3d[i].y = y0 + yvec2D[i];

    x -= xStep;
  }

  return vec3d;
}

auto Tentacles3D::operator==(const Tentacles3D& t) const -> bool
{
  const bool result = m_tentacles == t.m_tentacles;

  if (!result)
  {
    logInfo("Tentacles3D result == {}", result);
    logInfo("tentacles == t.tentacles = {}", tentacles == t.tentacles);
  }

  return result;
}

void Tentacles3D::AddTentacle(Tentacle3D&& t)
{
  (void)m_tentacles.emplace_back(std::move(t));
}

void Tentacles3D::SetAllowOverexposed(const bool val)
{
  for (auto& t : m_tentacles)
  {
    t.SetAllowOverexposed(val);
  }
}

} // namespace GOOM
