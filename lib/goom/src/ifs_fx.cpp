/*
 * ifs.c --- modified iterated functions system for goom.
 */

/*-
 * Copyright (c) 1997 by Massimino Pascal <Pascal.Massimon@ens.fr>
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * If this mode is weird and you have an old MetroX server, it is buggy.
 * There is a free SuSE-enhanced MetroX X server that is fine.
 *
 * When shown ifs, Diana Rose (4 years old) said, "It looks like dancing."
 *
 * Revision History:
 * 13-Dec-2003: Added some goom specific stuffs (to make ifs a VisualFX).
 * 11-Apr-2002: jeko@ios-software.com: Make ifs.c system-indendant. (ifs.h added)
 * 01-Nov-2000: Allocation checks
 * 10-May-1997: jwz@jwz.org: turned into a standalone program.
 *              Made it render into an offscreen bitmap and then copy
 *              that onto the screen, to reduce flicker.
 */

/* #ifdef STANDALONE */
#include "ifs_fx.h"

#include "goom_config.h"
#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_testing.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"
#include "goomutils/colorutils.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
// #undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "goomutils/strutils.h"

#include <array>

//#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cereal/archives/json.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

CEREAL_REGISTER_TYPE(goom::IfsFx);
CEREAL_REGISTER_POLYMORPHIC_RELATION(goom::VisualFx, goom::IfsFx);

namespace goom
{

using namespace goom::utils;

class IfsStats
{
public:
  IfsStats() noexcept = default;

  void reset();
  void log(const StatsLogValueFunc&) const;
  void updateStart();
  void updateEnd();

  void updateIfsIncr(int val);
  void setlastIfsIncr(int val);

private:
  uint32_t numUpdates = 0;
  uint64_t totalTimeInUpdatesMs = 0;
  uint32_t minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  uint32_t maxTimeInUpdatesMs = 0;
  std::chrono::high_resolution_clock::time_point timeNowHiRes{};
  int maxIfsIncr = -1000;
  int lastIfsIncr = 0;
};

void IfsStats::reset()
{
  numUpdates = 0;
  totalTimeInUpdatesMs = 0;
  minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  maxTimeInUpdatesMs = 0;
  timeNowHiRes = std::chrono::high_resolution_clock::now();
  maxIfsIncr = -1000;
  lastIfsIncr = 0;
}

void IfsStats::log(const StatsLogValueFunc& logVal) const
{
  const constexpr char* module = "Ifs";

  const int32_t avTimeInUpdateMs = std::lround(
      numUpdates == 0 ? -1.0
                      : static_cast<float>(totalTimeInUpdatesMs) / static_cast<float>(numUpdates));
  logVal(module, "avTimeInUpdateMs", avTimeInUpdateMs);
  logVal(module, "minTimeInUpdatesMs", minTimeInUpdatesMs);
  logVal(module, "maxTimeInUpdatesMs", maxTimeInUpdatesMs);
  logVal(module, "maxIfsIncr", maxIfsIncr);
  logVal(module, "lastIfsIncr", lastIfsIncr);
}

inline void IfsStats::updateStart()
{
  timeNowHiRes = std::chrono::high_resolution_clock::now();
  numUpdates++;
}

inline void IfsStats::updateEnd()
{
  const auto timeNow = std::chrono::high_resolution_clock::now();

  using ms = std::chrono::milliseconds;
  const ms diff = std::chrono::duration_cast<ms>(timeNow - timeNowHiRes);
  const auto timeInUpdateMs = static_cast<uint32_t>(diff.count());
  if (timeInUpdateMs < minTimeInUpdatesMs)
  {
    minTimeInUpdatesMs = timeInUpdateMs;
  }
  else if (timeInUpdateMs > maxTimeInUpdatesMs)
  {
    maxTimeInUpdatesMs = timeInUpdateMs;
  }
  totalTimeInUpdatesMs += timeInUpdateMs;
}

inline void IfsStats::updateIfsIncr(int val)
{
  if (val > maxIfsIncr)
  {
    maxIfsIncr = val;
  }
}

inline void IfsStats::setlastIfsIncr(int val)
{
  lastIfsIncr = val;
}

inline bool megaChangeColorMapEvent()
{
  return probabilityOfMInN(9, 10);
}

inline bool allowOverexposedEvent()
{
  return probabilityOfMInN(10, 50);
}

struct IfsPoint
{
  uint32_t x = 0;
  uint32_t y = 0;
  uint32_t count = 0;
  Pixel color{0U};

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(x, y);
  };
};

using Dbl = float;
using Flt = int;

struct FltPoint
{
  Flt x = 0;
  Flt y = 0;
};

static constexpr int fix = 12;

inline Flt dbl_to_flt(const Dbl x)
{
  constexpr int unit = 1 << fix;
  return static_cast<Flt>(static_cast<Dbl>(unit) * x);
}

inline Flt div_by_unit(const Flt x)
{
  return x >> fix;
}

inline Flt div_by_2units(const Flt x)
{
  return x >> (fix + 1);
}

struct CentreType
{
  uint32_t depth;
  Dbl r1Mean;
  Dbl r2Mean;
  Dbl dr1Mean;
  Dbl dr2Mean;
};
// clang-format off
static const std::vector<CentreType> centreList = {
  { .depth = 10, .r1Mean = 0.7, .r2Mean = 0.0, .dr1Mean = 0.3, .dr2Mean = 0.4 },
  { .depth =  6, .r1Mean = 0.6, .r2Mean = 0.0, .dr1Mean = 0.4, .dr2Mean = 0.3 },
  { .depth =  4, .r1Mean = 0.5, .r2Mean = 0.0, .dr1Mean = 0.4, .dr2Mean = 0.3 },
  { .depth =  2, .r1Mean = 0.5, .r2Mean = 0.0, .dr1Mean = 0.4, .dr2Mean = 0.3 },
};
// clang-format on

struct Similitude
{
  Dbl dbl_cx = 0;
  Dbl dbl_cy = 0;
  Dbl dbl_r1 = 0;
  Dbl dbl_r2 = 0;
  Dbl A1 = 0;
  Dbl A2 = 0;
  Flt cosA1 = 0;
  Flt sinA1 = 0;
  Flt cosA2 = 0;
  Flt sinA2 = 0;
  Flt cx = 0;
  Flt cy = 0;
  Flt r1 = 0;
  Flt r2 = 0;
  Pixel color{0U};

  bool operator==(const Similitude& s) const = default;
  /**
  bool operator==(const Similitude& s) const
  {
    return dbl_cx == s.dbl_cx && dbl_cy == s.dbl_cy && r == s.r && dbl_r2 == s.dbl_r2 && A == s.A && A2 == s.A2 &&
           Ct == s.Ct && St == s.St && cosA2 == s.cosA2 && sinA2 == s.sinA2 && cx == s.cx && cy == s.cy &&
           R == s.R && r2 == s.r2;
  }
  **/

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(dbl_cx, dbl_cy, dbl_r1, dbl_r2, A1, A2, cosA1, sinA1, cosA2, sinA2, cx, cy, r1, r2, color);
  };
};

class FractalHits
{
public:
  FractalHits() noexcept = default;
  FractalHits(uint32_t width, uint32_t height) noexcept;
  void reset();
  void addHit(uint32_t x, uint32_t y, const Pixel&);
  [[nodiscard]] const std::vector<IfsPoint>& getBuffer();
  [[nodiscard]] uint32_t getMaxHitCount() const { return maxHitCount; }

private:
  const uint32_t width = 0;
  const uint32_t height = 0;
  struct HitInfo
  {
    uint32_t count = 0;
    Pixel color{0U};
  };
  std::vector<std::vector<HitInfo>> hitInfo{};
  uint32_t maxHitCount = 0;
  std::vector<IfsPoint> hits{};
  std::vector<IfsPoint> buffer{};
};

FractalHits::FractalHits(const uint32_t w, const uint32_t h) noexcept
  : width{w}, height{h}, hitInfo(height)
{
  for (auto& xHit : hitInfo)
  {
    xHit.resize(width);
  }
  hits.reserve(1000);
}

void FractalHits::reset()
{
  maxHitCount = 0;
  hits.resize(0);
  for (auto& xHit : hitInfo)
  {
    // Optimization makes sense here:
    // std::fill(xHit.begin(), xHit.end(), HitInfo{0, Pixel{0U}});
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
    std::memset(xHit.data(), 0, xHit.size() * sizeof(HitInfo));
#pragma GCC diagnostic pop
  }
}

void FractalHits::addHit(const uint32_t x, const uint32_t y, const Pixel& color)
{
  const uint32_t ux = x & 0x7fffffff;
  const uint32_t uy = y & 0x7fffffff;

  if (ux >= width || uy >= height)
  {
    return;
  }

  HitInfo& h = hitInfo[uy][ux];

  h.color = getColorAverage(h.color, color);
  h.count++;

  if (h.count > maxHitCount)
  {
    maxHitCount = h.count;
  }

  if (h.count == 1)
  {
    hits.emplace_back(IfsPoint{ux, uy, 1});
  }
}

const std::vector<IfsPoint>& FractalHits::getBuffer()
{
  buffer.resize(hits.size());
  for (size_t i = 0; i < hits.size(); i++)
  {
    IfsPoint pt = hits[i];
    pt.count = hitInfo[pt.y][pt.x].count;
    pt.color = hitInfo[pt.y][pt.x].color;
    buffer[i] = pt;
  }
  return buffer;
}

class Fractal
{
public:
  Fractal() noexcept = default;
  Fractal(const std::shared_ptr<const PluginInfo>&, const ColorMaps&) noexcept;
  Fractal(const Fractal&) noexcept = delete;
  Fractal& operator=(const Fractal&) = delete;

  void init();
  void resetCurrentIFSFunc();

  [[nodiscard]] uint32_t getSpeed() const { return speed; }
  void setSpeed(const uint32_t val) { speed = val; }

  [[nodiscard]] uint32_t getMaxHitCount() const { return curHits->getMaxHitCount(); }

  [[nodiscard]] const std::vector<IfsPoint>& drawIfs();

  bool operator==(const Fractal&) const;

  template<class Archive>
  void serialize(Archive&);

private:
  static constexpr size_t maxSimi = 6;
  static constexpr size_t maxCountTimesSpeed = 1000;

  const ColorMaps* colorMaps = nullptr;

  uint32_t lx = 0;
  uint32_t ly = 0;
  uint32_t numSimi = 0;
  uint32_t depth = 0;
  uint32_t count = 0;
  uint32_t speed = 6;
  Dbl r1Mean = 0;
  Dbl r2Mean = 0;
  Dbl dr1Mean = 0;
  Dbl dr2Mean = 0;

  std::vector<Similitude> components{};

  FractalHits hits1{};
  FractalHits hits2{};
  FractalHits* prevHits = nullptr;
  FractalHits* curHits = nullptr;

  [[nodiscard]] Flt getLx() const { return static_cast<Flt>(lx); }
  [[nodiscard]] Flt getLy() const { return static_cast<Flt>(ly); }
  void drawFractal();
  void randomSimis(size_t start, size_t num);
  void trace(uint32_t curDepth, const FltPoint& p0);
  void updateHits(const Similitude&, const FltPoint&);
  using IFSFunc =
      std::function<FltPoint(const Similitude& simi, float x1, float y1, float x2, float y2)>;
  IFSFunc curFunc{};
  FltPoint transform(const Similitude&, const FltPoint& p0);
  static Dbl gaussRand(Dbl c, Dbl S, Dbl A_mult_1_minus_exp_neg_S);
  static Dbl halfGaussRand(Dbl c, Dbl S, Dbl A_mult_1_minus_exp_neg_S);
  static constexpr Dbl get_1_minus_exp_neg_S(Dbl S);
};

Fractal::Fractal(const std::shared_ptr<const PluginInfo>& goomInfo, const ColorMaps& cm) noexcept
  : colorMaps{&cm},
    lx{(goomInfo->getScreenInfo().width - 1) / 2},
    ly{(goomInfo->getScreenInfo().height - 1) / 2},
    components(5 * maxSimi),
    hits1{goomInfo->getScreenInfo().width, goomInfo->getScreenInfo().height},
    hits2{goomInfo->getScreenInfo().width, goomInfo->getScreenInfo().height},
    prevHits{&hits1},
    curHits{&hits2}
{
  init();
  resetCurrentIFSFunc();
}

void Fractal::init()
{
  prevHits->reset();
  curHits->reset();

  // clang-format off
  static const Weights<size_t> centreWeights{{
    {0, 10},
    {1,  5},
    {2,  3},
    {3,  1},
  }};
  // clang-format on
  assert(centreWeights.getNumElements() == centreList.size());

  const size_t numCentres = 2 + centreWeights.getRandomWeighted();

  depth = centreList.at(numCentres - 2).depth;
  r1Mean = centreList[numCentres - 2].r1Mean;
  r2Mean = centreList[numCentres - 2].r2Mean;
  dr1Mean = centreList[numCentres - 2].dr1Mean;
  dr2Mean = centreList[numCentres - 2].dr2Mean;

  numSimi = numCentres;

  for (size_t i = 0; i < 5; i++)
  {
    randomSimis(i * maxSimi, maxSimi);
  }
}

void Fractal::resetCurrentIFSFunc()
{
  if (probabilityOfMInN(0, 10))
  {
    curFunc = [&](const Similitude& simi, const float x1, const float y1, const float x2,
                  const float y2) -> FltPoint {
      return {
          div_by_unit(x1 * simi.sinA1 - y1 * simi.cosA1 + x2 * simi.sinA2 - y2 * simi.cosA2) +
              simi.cx,
          div_by_unit(x1 * simi.cosA1 + y1 * simi.sinA1 + x2 * simi.cosA2 + y2 * simi.sinA2) +
              simi.cy,
      };
    };
  }
  else
  {
    curFunc = [&](const Similitude& simi, const float x1, const float y1, const float x2,
                  const float y2) -> FltPoint {
      return {
          div_by_unit(x1 * simi.cosA1 - y1 * simi.sinA1 + x2 * simi.cosA2 - y2 * simi.sinA2) +
              simi.cx,
          div_by_unit(x1 * simi.sinA1 + y1 * simi.cosA1 + x2 * simi.sinA2 + y2 * simi.cosA2) +
              simi.cy,
      };
    };
  }
}

bool Fractal::operator==(const Fractal& f) const
{
  return numSimi == f.numSimi && components == f.components && depth == f.depth &&
         count == f.count && speed == f.speed && lx == f.lx && ly == f.ly && r1Mean == f.r1Mean &&
         r2Mean == f.r2Mean && dr1Mean == f.dr1Mean && dr2Mean == f.dr2Mean;
}

template<class Archive>
void Fractal::serialize(Archive& ar)
{
  ar(numSimi, components, depth, count, speed, lx, ly, r1Mean, r2Mean, dr1Mean, dr2Mean);
};

const std::vector<IfsPoint>& Fractal::drawIfs()
{
  const Dbl u = static_cast<Dbl>(count * speed) / static_cast<Dbl>(maxCountTimesSpeed);
  const Dbl uSq = u * u;
  const Dbl v = 1.0 - u;
  const Dbl vSq = v * v;
  const Dbl u0 = vSq * v;
  const Dbl u1 = 3.0 * vSq * u;
  const Dbl u2 = 3.0 * v * uSq;
  const Dbl u3 = u * uSq;

  Similitude* s = components.data();
  Similitude* s0 = s + numSimi;
  Similitude* s1 = s0 + numSimi;
  Similitude* s2 = s1 + numSimi;
  Similitude* s3 = s2 + numSimi;

  for (size_t i = 0; i < numSimi; i++)
  {
    s[i].dbl_cx = u0 * s0[i].dbl_cx + u1 * s1[i].dbl_cx + u2 * s2[i].dbl_cx + u3 * s3[i].dbl_cx;
    s[i].dbl_cy = u0 * s0[i].dbl_cy + u1 * s1[i].dbl_cy + u2 * s2[i].dbl_cy + u3 * s3[i].dbl_cy;

    s[i].dbl_r1 = u0 * s0[i].dbl_r1 + u1 * s1[i].dbl_r1 + u2 * s2[i].dbl_r1 + u3 * s3[i].dbl_r1;
    s[i].dbl_r2 = u0 * s0[i].dbl_r2 + u1 * s1[i].dbl_r2 + u2 * s2[i].dbl_r2 + u3 * s3[i].dbl_r2;

    s[i].A1 = u0 * s0[i].A1 + u1 * s1[i].A1 + u2 * s2[i].A1 + u3 * s3[i].A1;
    s[i].A2 = u0 * s0[i].A2 + u1 * s1[i].A2 + u2 * s2[i].A2 + u3 * s3[i].A2;
  }

  curHits->reset();
  drawFractal();
  const std::vector<IfsPoint>& curBuffer = curHits->getBuffer();
  std::swap(prevHits, curHits);

  if (count < maxCountTimesSpeed / speed)
  {
    count++;
  }
  else
  {
    s = components.data();
    s0 = s + numSimi;
    s1 = s0 + numSimi;
    s2 = s1 + numSimi;
    s3 = s2 + numSimi;

    for (size_t i = 0; i < numSimi; i++)
    {
      s1[i].dbl_cx = (2.0 * s3[i].dbl_cx) - s2[i].dbl_cx;
      s1[i].dbl_cy = (2.0 * s3[i].dbl_cy) - s2[i].dbl_cy;

      s1[i].dbl_r1 = (2.0 * s3[i].dbl_r1) - s2[i].dbl_r1;
      s1[i].dbl_r2 = (2.0 * s3[i].dbl_r2) - s2[i].dbl_r2;

      s1[i].A1 = (2.0 * s3[i].A1) - s2[i].A1;
      s1[i].A2 = (2.0 * s3[i].A2) - s2[i].A2;

      s0[i] = s3[i];
    }

    randomSimis(3 * numSimi, numSimi);
    randomSimis(4 * numSimi, numSimi);

    count = 0;
  }

  return curBuffer;
}

void Fractal::drawFractal()
{
  for (size_t i = 0; i < numSimi; i++)
  {
    Similitude& simi = components[i];

    simi.cx = dbl_to_flt(simi.dbl_cx);
    simi.cy = dbl_to_flt(simi.dbl_cy);

    simi.cosA1 = dbl_to_flt(std::cos(simi.A1));
    simi.sinA1 = dbl_to_flt(std::sin(simi.A1));
    simi.cosA2 = dbl_to_flt(std::cos(simi.A2));
    simi.sinA2 = dbl_to_flt(std::sin(simi.A2));

    simi.r1 = dbl_to_flt(simi.dbl_r1);
    simi.r2 = dbl_to_flt(simi.dbl_r2);
  }

  for (size_t i = 0; i < numSimi; i++)
  {
    const FltPoint p0{components[i].cx, components[i].cy};

    for (size_t j = 0; j < numSimi; j++)
    {
      if (i != j)
      {
        const FltPoint p = transform(components[j], p0);
        trace(depth, p);
      }
    }
  }
}

constexpr Dbl Fractal::get_1_minus_exp_neg_S(const Dbl S)
{
  return 1.0 - std::exp(-S);
}

Dbl Fractal::gaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S)
{
  const Dbl x = getRandInRange(0.0f, 1.0f);
  const Dbl y = A_mult_1_minus_exp_neg_S * (1.0 - std::exp(-x * x * S));
  return probabilityOfMInN(1, 2) ? c + y : c - y;
}

Dbl Fractal::halfGaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S)
{
  const Dbl x = getRandInRange(0.0f, 1.0f);
  const Dbl y = A_mult_1_minus_exp_neg_S * (1.0 - std::exp(-x * x * S));
  return c + y;
}

void Fractal::randomSimis(const size_t start, const size_t num)
{
  static const constinit Dbl c_factor = 0.8f * get_1_minus_exp_neg_S(4.0);
  static const constinit Dbl r1_1_minus_exp_neg_S = get_1_minus_exp_neg_S(3.0);
  static const constinit Dbl r2_1_minus_exp_neg_S = get_1_minus_exp_neg_S(2.0);
  static const constinit Dbl A1_factor = 360.0F * get_1_minus_exp_neg_S(4.0);
  static const constinit Dbl A2_factor = A1_factor;

  const Dbl r1_factor = dr1Mean * r1_1_minus_exp_neg_S;
  const Dbl r2_factor = dr2Mean * r2_1_minus_exp_neg_S;

  const ColorMapGroup colorMapGroup = colorMaps->getRandomGroup();

  for (size_t i = start; i < start + num; i++)
  {
    components[i].dbl_cx = gaussRand(0.0, 4.0, c_factor);
    components[i].dbl_cy = gaussRand(0.0, 4.0, c_factor);
    components[i].dbl_r1 = gaussRand(r1Mean, 3.0, r1_factor);
    components[i].dbl_r2 = halfGaussRand(r2Mean, 2.0, r2_factor);
    components[i].A1 = gaussRand(0.0, 4.0, A1_factor) * (m_pi / 180.0);
    components[i].A2 = gaussRand(0.0, 4.0, A2_factor) * (m_pi / 180.0);
    components[i].cosA1 = 0;
    components[i].sinA1 = 0;
    components[i].cosA2 = 0;
    components[i].sinA2 = 0;
    components[i].cx = 0;
    components[i].cy = 0;
    components[i].r1 = 0;
    components[i].r2 = 0;

    components[i].color = ColorMap::getRandomColor(colorMaps->getRandomColorMap(colorMapGroup));
  }
}

void Fractal::trace(const uint32_t curDepth, const FltPoint& p0)
{
  for (size_t i = 0; i < numSimi; i++)
  {
    const FltPoint p = transform(components[i], p0);

    updateHits(components[i], p);

    if (!curDepth)
    {
      continue;
    }
    if (((p.x - p0.x) >> 4) == 0 || ((p.y - p0.y) >> 4) == 0)
    {
      continue;
    }

    trace(curDepth - 1, p);
  }
}

inline FltPoint Fractal::transform(const Similitude& simi, const FltPoint& p0)
{
  const Flt x1 = div_by_unit((p0.x - simi.cx) * simi.r1);
  const Flt y1 = div_by_unit((p0.y - simi.cy) * simi.r1);

  const Flt x2 = div_by_unit((+x1 - simi.cx) * simi.r2);
  const Flt y2 = div_by_unit((-y1 - simi.cy) * simi.r2);

  return curFunc(simi, x1, y1, x2, y2);
}

inline void Fractal::updateHits(const Similitude& simi, const FltPoint& p)
{
  const auto x = static_cast<uint32_t>(getLx() + div_by_2units(p.x * getLx()));
  const auto y = static_cast<uint32_t>(getLy() - div_by_2units(p.y * getLy()));
  curHits->addHit(x, y, simi.color);
}

class Colorizer
{
public:
  Colorizer() noexcept;
  Colorizer(const Colorizer&) = delete;
  Colorizer& operator=(const Colorizer&) = delete;

  const ColorMaps& getColorMaps() const;

  IfsFx::ColorMode getColorMode() const;
  void setForcedColorMode(IfsFx::ColorMode);
  void changeColorMode();

  void changeColorMaps();

  void setMaxHitCount(uint32_t val);

  [[nodiscard]] Pixel getMixedColor(
      const Pixel& baseColor, uint32_t hitCount, float tmix, float x, float y);

  bool operator==(const Colorizer&) const;

  template<class Archive>
  void serialize(Archive&);

private:
  WeightedColorMaps colorMaps{Weights<ColorMapGroup>{{
      {ColorMapGroup::perceptuallyUniformSequential, 10},
      {ColorMapGroup::sequential, 10},
      {ColorMapGroup::sequential2, 10},
      {ColorMapGroup::cyclic, 10},
      {ColorMapGroup::diverging, 20},
      {ColorMapGroup::diverging_black, 1},
      {ColorMapGroup::qualitative, 10},
      {ColorMapGroup::misc, 20},
  }}};
  const ColorMap* mixerMap1;
  const ColorMap* prevMixerMap1;
  const ColorMap* mixerMap2;
  const ColorMap* prevMixerMap2;
  mutable uint32_t countSinceColorMapChange = 0;
  static constexpr uint32_t minColorMapChangeCompleted = 100;
  static constexpr uint32_t maxColorMapChangeCompleted = 10000;
  uint32_t colorMapChangeCompleted = minColorMapChangeCompleted;

  IfsFx::ColorMode colorMode = IfsFx::ColorMode::mapColors;
  IfsFx::ColorMode forcedColorMode = IfsFx::ColorMode::_null;
  uint32_t maxHitCount = 0;
  float logMaxHitCount = 0;
  float tBetweenColors = 0.5; // in [0, 1]
  static IfsFx::ColorMode getNextColorMode();
  [[nodiscard]] Pixel getNextMixerMapColor(float t, float x, float y) const;
};

Colorizer::Colorizer() noexcept
  : mixerMap1{&colorMaps.getRandomColorMap()},
    prevMixerMap1{mixerMap1},
    mixerMap2{&colorMaps.getRandomColorMap()},
    prevMixerMap2{mixerMap2}
{
}

bool Colorizer::operator==(const Colorizer& c) const
{
  return countSinceColorMapChange == c.countSinceColorMapChange &&
         colorMapChangeCompleted == c.colorMapChangeCompleted && colorMode == c.colorMode &&
         tBetweenColors == c.tBetweenColors;
}

template<class Archive>
void Colorizer::serialize(Archive& ar)
{
  ar(countSinceColorMapChange, colorMapChangeCompleted, colorMode, tBetweenColors);

  auto mixerMapName1 = mixerMap1->getMapName();
  ar(cereal::make_nvp("mixerMap1", mixerMapName1));
  auto prevMixerMapName1 = prevMixerMap1->getMapName();
  ar(cereal::make_nvp("prevMixerMap1", prevMixerMapName1));
  mixerMap1 = &colorMaps.getColorMap(mixerMapName1);
  prevMixerMap1 = &colorMaps.getColorMap(prevMixerMapName1);

  auto mixerMapName2 = mixerMap2->getMapName();
  ar(cereal::make_nvp("mixerMap2", mixerMapName2));
  auto prevMixerMapName2 = prevMixerMap2->getMapName();
  ar(cereal::make_nvp("prevMixerMap2", prevMixerMapName2));
  mixerMap2 = &colorMaps.getColorMap(mixerMapName2);
  prevMixerMap2 = &colorMaps.getColorMap(prevMixerMapName2);
};

inline void Colorizer::setMaxHitCount(const uint32_t val)
{
  maxHitCount = val;
  logMaxHitCount = std::log(maxHitCount);
}

inline const ColorMaps& Colorizer::getColorMaps() const
{
  return colorMaps;
}

inline IfsFx::ColorMode Colorizer::getColorMode() const
{
  return colorMode;
}

inline void Colorizer::setForcedColorMode(const IfsFx::ColorMode c)
{
  forcedColorMode = c;
}

void Colorizer::changeColorMode()
{
  if (forcedColorMode != IfsFx::ColorMode::_null)
  {
    colorMode = forcedColorMode;
  }
  else
  {
    colorMode = getNextColorMode();
  }
}

IfsFx::ColorMode Colorizer::getNextColorMode()
{
  // clang-format off
  static const Weights<IfsFx::ColorMode> colorModeWeights{{
    { IfsFx::ColorMode::mapColors,           15 },
    { IfsFx::ColorMode::megaMapColorChange,  20 },
    { IfsFx::ColorMode::mixColors,           10 },
    { IfsFx::ColorMode::megaMixColorChange,  15 },
    { IfsFx::ColorMode::reverseMixColors,    10 },
    { IfsFx::ColorMode::singleColors,         5 },
    { IfsFx::ColorMode::sineMixColors,       15 },
    { IfsFx::ColorMode::sineMapColors,       15 },
  }};
  // clang-format on

  return colorModeWeights.getRandomWeighted();
}

void Colorizer::changeColorMaps()
{
  prevMixerMap1 = mixerMap1;
  mixerMap1 = &colorMaps.getRandomColorMap();
  prevMixerMap2 = mixerMap2;
  mixerMap2 = &colorMaps.getRandomColorMap();
  //  logInfo("prevMixerMap = {}", enumToString(prevMixerMap->getMapName()));
  //  logInfo("mixerMap = {}", enumToString(mixerMap->getMapName()));
  colorMapChangeCompleted = getRandInRange(minColorMapChangeCompleted, maxColorMapChangeCompleted);
  tBetweenColors = getRandInRange(0.2F, 0.8F);
  countSinceColorMapChange = colorMapChangeCompleted;
}

inline Pixel Colorizer::getNextMixerMapColor(const float t, const float x, const float y) const
{
  const Pixel nextColor = ColorMap::colorMix(mixerMap1->getColor(x), mixerMap2->getColor(y), t);
  if (countSinceColorMapChange == 0)
  {
    return nextColor;
  }

  const float tTransition =
      static_cast<float>(countSinceColorMapChange) / static_cast<float>(colorMapChangeCompleted);
  countSinceColorMapChange--;
  const Pixel prevNextColor =
      ColorMap::colorMix(prevMixerMap1->getColor(x), prevMixerMap2->getColor(y), t);
  return ColorMap::colorMix(nextColor, prevNextColor, tTransition);
}

inline Pixel Colorizer::getMixedColor(
    const Pixel& baseColor, const uint32_t hitCount, const float tmix, const float x, const float y)
{
  constexpr float brightness = 1.9;
  const float logAlpha = maxHitCount <= 1
                             ? 1.0F
                             : brightness * std::log(static_cast<float>(hitCount)) / logMaxHitCount;

  Pixel mixColor;
  float t = tBetweenColors;

  switch (colorMode)
  {
    case IfsFx::ColorMode::mapColors:
    case IfsFx::ColorMode::megaMapColorChange:
      mixColor = getNextMixerMapColor(logAlpha, x, y);
      t = tBetweenColors;
      break;

    case IfsFx::ColorMode::mixColors:
    case IfsFx::ColorMode::reverseMixColors:
    case IfsFx::ColorMode::megaMixColorChange:
      mixColor = getNextMixerMapColor(tmix, x, y);
      break;

    case IfsFx::ColorMode::singleColors:
      mixColor = baseColor;
      break;

    case IfsFx::ColorMode::sineMixColors:
    case IfsFx::ColorMode::sineMapColors:
    {
      static float freq = 20;
      static const float zStep = 0.1;
      static float z = 0;

      mixColor = getNextMixerMapColor(0.5F * (1.0F + std::sin(freq * z)), x, y);
      z += zStep;
      if (colorMode == IfsFx::ColorMode::sineMapColors)
      {
        t = tBetweenColors;
      }
      break;
    }

    default:
      throw std::logic_error("Unknown ColorMode");
  }

  if (colorMode == IfsFx::ColorMode::reverseMixColors)
  {
    mixColor = ColorMap::colorMix(baseColor, mixColor, t);
  }
  else
  {
    mixColor = ColorMap::colorMix(mixColor, baseColor, t);
  }

  static GammaCorrection gammaCorrect{4.2, 0.01};

  return gammaCorrect.getCorrection(logAlpha, mixColor);
}

class LowDensityBlurrer
{
public:
  LowDensityBlurrer() noexcept : screenWidth{0}, screenHeight{0}, width{0} {}
  LowDensityBlurrer(uint32_t screenWidth, uint32_t screenHeight, uint32_t width) noexcept;

  [[nodiscard]] uint32_t getWidth() const { return width; }
  void setWidth(uint32_t val);

  void doBlur(std::vector<IfsPoint>&, PixelBuffer&) const;

private:
  const uint32_t screenWidth;
  const uint32_t screenHeight;
  uint32_t width;
};

inline LowDensityBlurrer::LowDensityBlurrer(const uint32_t screenW,
                                            const uint32_t screenH,
                                            const uint32_t w) noexcept
  : screenWidth{screenW}, screenHeight{screenH}, width{w}
{
}

void LowDensityBlurrer::setWidth(const uint32_t val)
{
  if (val != 3 && val != 5 && val != 7)
  {
    throw std::logic_error(std20::format("Invalid blur width {}.", val));
  }
  width = val;
}

void LowDensityBlurrer::doBlur(std::vector<IfsPoint>& lowDensityPoints, PixelBuffer& buff) const
{
  std::vector<Pixel> neighbours(width * width);

  for (auto& p : lowDensityPoints)
  {
    const uint32_t x = p.x & 0x7fffffff;
    const uint32_t y = p.y & 0x7fffffff;

    if (x < (width / 2) || y < (width / 2) || x >= screenWidth - (width / 2) ||
        y >= screenHeight - (width / 2))
    {
      p.count = 0; // just signal that no need to set buff
      continue;
    }

    size_t n = 0;
    uint32_t neighY = y - width / 2;
    for (size_t i = 0; i < width; i++)
    {
      uint32_t neighX = x - width / 2;
      for (size_t j = 0; j < width; j++)
      {
        neighbours[n] = buff(neighX, neighY);
        n++;
        neighX++;
      }
      neighY++;
    }
    p.color = getColorAverage(neighbours);
  }

  for (const auto& p : lowDensityPoints)
  {
    if (p.count == 0)
    {
      continue;
    }
    const uint32_t x = p.x & 0x7fffffff;
    const uint32_t y = p.y & 0x7fffffff;
    buff(x, y) = p.color;
  }
}

enum class ModType
{
  MOD_MER,
  MOD_FEU,
  MOD_MERVER
};

constexpr size_t numChannels = 4;
using Int32ChannelArray = std::array<int32_t, numChannels>;

static_assert(numChannels == 4);

inline Pixel getPixel(const Int32ChannelArray& col)
{
  return Pixel{{.r = static_cast<uint8_t>(col[ROUGE]),
                .g = static_cast<uint8_t>(col[VERT]),
                .b = static_cast<uint8_t>(col[BLEU]),
                .a = static_cast<uint8_t>(col[ALPHA])}};
}

inline Int32ChannelArray getChannelArray(const Pixel& p)
{
  Int32ChannelArray a;

  a[ROUGE] = p.r();
  a[VERT] = p.g();
  a[BLEU] = p.b();
  a[ALPHA] = p.a();

  return a;
}

struct IfsUpdateData
{
  Pixel couleur{0xc0c0c0c0};
  Int32ChannelArray v = {2, 4, 3, 2};
  Int32ChannelArray col = {2, 4, 3, 2};
  int justChanged = 0;
  ModType mode = ModType::MOD_MERVER;
  int cycle = 0;

  bool operator==(const IfsUpdateData& u) const
  {
    return couleur.rgba() == u.couleur.rgba() && v == u.v && col == u.col &&
           justChanged == u.justChanged && mode == u.mode && cycle == u.cycle;
  }

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(couleur, v, col, justChanged, mode, cycle);
  };
};

class IfsFx::IfsImpl
{
public:
  IfsImpl() noexcept = default;
  explicit IfsImpl(std::shared_ptr<const PluginInfo>) noexcept;
  ~IfsImpl() noexcept;
  IfsImpl(const IfsImpl&) = delete;
  IfsImpl& operator=(const IfsImpl&) = delete;

  void init();

  void applyNoDraw();
  void updateIfs(PixelBuffer& prevBuff, PixelBuffer& currentBuff);
  void setBuffSettings(const FXBuffSettings&);
  void updateLowDensityThreshold();
  IfsFx::ColorMode getColorMode() const;
  void setColorMode(IfsFx::ColorMode);
  void renew();
  void updateIncr();

  void finish();
  void log(const StatsLogValueFunc&) const;

  bool operator==(const IfsImpl&) const;

private:
  static constexpr int maxCountBeforeNextUpdate = 1000;

  std::shared_ptr<const PluginInfo> goomInfo{};

  GoomDraw draw{};
  Colorizer colorizer{};
  FXBuffSettings buffSettings{};

  bool allowOverexposed = true;
  uint32_t countSinceOverexposed = 0;
  static constexpr uint32_t maxCountSinceOverexposed = 100;
  void updateAllowOverexposed();

  std::unique_ptr<Fractal> fractal = nullptr;

  bool initialized = false;
  int ifs_incr = 1; // dessiner l'ifs (0 = non: > = increment)
  int decay_ifs = 0; // disparition de l'ifs
  int recay_ifs = 0; // dedisparition de l'ifs
  void updateDecay();
  void updateDecayAndRecay();

  IfsUpdateData updateData{};

  IfsStats stats{};

  void changeColormaps();
  static constexpr uint32_t maxDensityCount = 20;
  static constexpr uint32_t minDensityCount = 5;
  uint32_t lowDensityCount = 10;
  LowDensityBlurrer blurrer{};
  void updatePixelBuffers(PixelBuffer& prevBuff,
                          PixelBuffer& currentBuff,
                          const std::vector<IfsPoint>& points,
                          uint32_t maxHitCount,
                          const Pixel& color);
  void drawPixel(PixelBuffer& prevBuff,
                 PixelBuffer& currentBuff,
                 const IfsPoint&,
                 const Pixel& ifsColor,
                 float tMix);
  void updateColors();
  void updateColorsModeMer();
  void updateColorsModeMerver();
  void updateColorsModeFeu();
  [[nodiscard]] int getIfsIncr() const;

  friend class cereal::access;
  template<class Archive>
  void save(Archive&) const;
  template<class Archive>
  void load(Archive&);
};


IfsFx::IfsFx() noexcept : fxImpl{new IfsImpl{}}
{
}

IfsFx::IfsFx(const std::shared_ptr<const PluginInfo>& info) noexcept : fxImpl{new IfsImpl{info}}
{
}

IfsFx::~IfsFx() noexcept = default;

void IfsFx::init()
{
  fxImpl->init();
}

bool IfsFx::operator==(const IfsFx& i) const
{
  return fxImpl->operator==(*i.fxImpl);
}

void IfsFx::setBuffSettings(const FXBuffSettings& settings)
{
  fxImpl->setBuffSettings(settings);
}

void IfsFx::start()
{
}

void IfsFx::finish()
{
  fxImpl->finish();
}

void IfsFx::log(const StatsLogValueFunc& logVal) const
{
  fxImpl->log(logVal);
}

std::string IfsFx::getFxName() const
{
  return "IFS FX";
}

void IfsFx::applyNoDraw()
{
  if (!enabled)
  {
    return;
  }

  fxImpl->applyNoDraw();
}

void IfsFx::apply(PixelBuffer& prevBuff, PixelBuffer& currentBuff)
{
  if (!enabled)
  {
    return;
  }

  fxImpl->updateIfs(prevBuff, currentBuff);
}

IfsFx::ColorMode IfsFx::getColorMode() const
{
  return fxImpl->getColorMode();
}

void IfsFx::setColorMode(const ColorMode c)
{
  fxImpl->setColorMode(c);
}

void IfsFx::updateIncr()
{
  fxImpl->updateIncr();
}

void IfsFx::renew()
{
  fxImpl->renew();
}

template<class Archive>
void IfsFx::serialize(Archive& ar)
{
  ar(CEREAL_NVP(enabled), CEREAL_NVP(fxImpl));
}

// Need to explicitly instantiate template functions for serialization.
template void IfsFx::serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);
template void IfsFx::serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template void IfsFx::IfsImpl::save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
template void IfsFx::IfsImpl::load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template<class Archive>
void IfsFx::IfsImpl::save(Archive& ar) const
{
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(fractal), CEREAL_NVP(draw), CEREAL_NVP(colorizer),
     CEREAL_NVP(allowOverexposed), CEREAL_NVP(countSinceOverexposed), CEREAL_NVP(ifs_incr),
     CEREAL_NVP(decay_ifs), CEREAL_NVP(recay_ifs), CEREAL_NVP(updateData));
}

template<class Archive>
void IfsFx::IfsImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(fractal), CEREAL_NVP(draw), CEREAL_NVP(colorizer),
     CEREAL_NVP(allowOverexposed), CEREAL_NVP(countSinceOverexposed), CEREAL_NVP(ifs_incr),
     CEREAL_NVP(decay_ifs), CEREAL_NVP(recay_ifs), CEREAL_NVP(updateData));
}

bool IfsFx::IfsImpl::operator==(const IfsImpl& i) const
{
  if (goomInfo == nullptr && i.goomInfo != nullptr)
  {
    return false;
  }
  if (goomInfo != nullptr && i.goomInfo == nullptr)
  {
    return false;
  }

  return ((goomInfo == nullptr && i.goomInfo == nullptr) || (*goomInfo == *i.goomInfo)) &&
         *fractal == *i.fractal && draw == i.draw && colorizer == i.colorizer &&
         allowOverexposed == i.allowOverexposed &&
         countSinceOverexposed == i.countSinceOverexposed && ifs_incr == i.ifs_incr &&
         decay_ifs == i.decay_ifs && recay_ifs == i.recay_ifs && updateData == i.updateData;
}

IfsFx::IfsImpl::IfsImpl(std::shared_ptr<const PluginInfo> info) noexcept
  : goomInfo{std::move(info)},
    draw{goomInfo->getScreenInfo().width, goomInfo->getScreenInfo().height},
    fractal{std::make_unique<Fractal>(goomInfo, colorizer.getColorMaps())},
    blurrer{goomInfo->getScreenInfo().width, goomInfo->getScreenInfo().height, 3}
{
#ifndef NO_LOGGING
  Fractal* fractal = fractal.get();
  for (size_t i = 0; i < 5 * maxSimi; i++)
  {
    Similitude cur = fractal->components[i];
    logDebug("simi[{}]: dbl_cx = {:.2}, dbl_cy = {:.2}, r = {:.2}, dbl_r2 = {:.2}, A = {:.2}, A2 = "
             "{:.2}.",
             i, cur.dbl_cx, cur.dbl_cy, cur.dbl_r1, cur.dbl_r2, cur.A1, cur.A2);
  }
#endif
}

IfsFx::IfsImpl::~IfsImpl() noexcept = default;

void IfsFx::IfsImpl::init()
{
  fractal->init();
  updateLowDensityThreshold();
}

void IfsFx::IfsImpl::setBuffSettings(const FXBuffSettings& settings)
{
  buffSettings = settings;
}

IfsFx::ColorMode IfsFx::IfsImpl::getColorMode() const
{
  return colorizer.getColorMode();
}

void IfsFx::IfsImpl::setColorMode(const IfsFx::ColorMode c)
{
  return colorizer.setForcedColorMode(c);
}

void IfsFx::IfsImpl::finish()
{
  stats.setlastIfsIncr(ifs_incr);
}

void IfsFx::IfsImpl::log(const StatsLogValueFunc& logVal) const
{
  stats.log(logVal);
}

void IfsFx::IfsImpl::renew()
{
  changeColormaps();
  colorizer.changeColorMode();
  updateAllowOverexposed();

  fractal->setSpeed(static_cast<uint32_t>(std::min(getRandInRange(1.1F, 10.0F), 5.1F) /
                                          (1.1F - goomInfo->getSoundInfo().getAcceleration())));
}

void IfsFx::IfsImpl::changeColormaps()
{
  colorizer.changeColorMaps();
  updateData.couleur = ColorMap::getRandomColor(colorizer.getColorMaps().getRandomColorMap());
}

void IfsFx::IfsImpl::applyNoDraw()
{
  updateDecayAndRecay();
  updateDecay();
}

void IfsFx::IfsImpl::updateIfs(PixelBuffer& prevBuff, PixelBuffer& currentBuff)
{
  stats.updateStart();

  updateDecayAndRecay();

  if (getIfsIncr() <= 0)
  {
    return;
  }

  // TODO: trouver meilleur soluce pour increment (mettre le code de gestion de l'ifs dans ce fichier)
  //       find the best solution for increment (put the management code of the ifs in this file)
  updateData.cycle++;
  if (updateData.cycle >= 80)
  {
    updateData.cycle = 0;
  }

  const std::vector<IfsPoint>& points = fractal->drawIfs();
  const uint32_t maxHitCount = fractal->getMaxHitCount();

  const int cycle10 = (updateData.cycle < 40) ? updateData.cycle / 10 : 7 - updateData.cycle / 10;
  const Pixel color = getRightShiftedChannels(updateData.couleur, cycle10);

  updatePixelBuffers(prevBuff, currentBuff, points, maxHitCount, color);

  updateData.justChanged--;

  updateData.col = getChannelArray(updateData.couleur);

  updateColors();

  updateData.couleur = getPixel(updateData.col);

  logDebug("updateData.col[ALPHA] = {:x}", updateData.col[ALPHA]);
  logDebug("updateData.col[BLEU] = {:x}", updateData.col[BLEU]);
  logDebug("updateData.col[VERT] = {:x}", updateData.col[VERT]);
  logDebug("updateData.col[ROUGE] = {:x}", updateData.col[ROUGE]);

  logDebug("updateData.v[ALPHA] = {:x}", updateData.v[ALPHA]);
  logDebug("updateData.v[BLEU] = {:x}", updateData.v[BLEU]);
  logDebug("updateData.v[VERT] = {:x}", updateData.v[VERT]);
  logDebug("updateData.v[ROUGE] = {:x}", updateData.v[ROUGE]);

  logDebug("updateData.mode = {:x}", updateData.mode);

  stats.updateEnd();
}

void IfsFx::IfsImpl::updateIncr()
{
  if (ifs_incr <= 0)
  {
    recay_ifs = 5;
    ifs_incr = 11;
    stats.updateIfsIncr(ifs_incr);
    renew();
  }
}

void IfsFx::IfsImpl::updateDecay()
{
  if ((ifs_incr > 0) && (decay_ifs <= 0))
  {
    decay_ifs = 100;
  }
}

void IfsFx::IfsImpl::updateDecayAndRecay()
{
  decay_ifs--;
  if (decay_ifs > 0)
  {
    ifs_incr += 2;
  }
  if (decay_ifs == 0)
  {
    ifs_incr = 0;
  }

  if (recay_ifs)
  {
    ifs_incr -= 2;
    recay_ifs--;
    if ((recay_ifs == 0) && (ifs_incr <= 0))
    {
      ifs_incr = 1;
    }
  }

  stats.updateIfsIncr(ifs_incr);
}

inline int IfsFx::IfsImpl::getIfsIncr() const
{
  return ifs_incr;
}

inline void IfsFx::IfsImpl::drawPixel(PixelBuffer& prevBuff,
                                      PixelBuffer& currentBuff,
                                      const IfsPoint& point,
                                      const Pixel& ifsColor,
                                      const float tMix)
{
  const float fx = point.x / static_cast<float>(goomInfo->getScreenInfo().width);
  const float fy = point.y / static_cast<float>(goomInfo->getScreenInfo().height);

  Pixel mixedColor = colorizer.getMixedColor(point.color, point.count, tMix, fx, fy);
  //  const std::vector<Pixel> colors{ColorMap::colorMix(ifsColor, mixedColor, 0.1), mixedColor};
  const std::vector<Pixel> colors{mixedColor, mixedColor};
  std::vector<PixelBuffer*> buffs{&currentBuff, &prevBuff};
  draw.setPixelRGB(buffs, point.x, point.y, colors);
}

void IfsFx::IfsImpl::updateAllowOverexposed()
{
  if (buffSettings.allowOverexposed)
  {
    return;
  }

  if (allowOverexposed)
  {
    if (countSinceOverexposed == 0)
    {
      allowOverexposed = false;
    }
    else
    {
      countSinceOverexposed--;
    }
  }
  else if (allowOverexposedEvent())
  {
    allowOverexposed = true;
    countSinceOverexposed = maxCountSinceOverexposed;
  }
}

void IfsFx::IfsImpl::updatePixelBuffers(PixelBuffer& prevBuff,
                                        PixelBuffer& currentBuff,
                                        const std::vector<IfsPoint>& points,
                                        const uint32_t maxHitCount,
                                        const Pixel& color)
{
  colorizer.setMaxHitCount(maxHitCount);
  bool doneColorChange = colorizer.getColorMode() != IfsFx::ColorMode::megaMapColorChange &&
                         colorizer.getColorMode() != IfsFx::ColorMode::megaMixColorChange;
  const size_t numPoints = points.size();
  const float tStep = numPoints == 1 ? 0.0F : (1.0F - 0.0F) / static_cast<float>(numPoints - 1);
  float t = -tStep;

  fractal->resetCurrentIFSFunc();

  std::vector<IfsPoint> lowDensityPoints{};
  for (size_t i = 0; i < numPoints; i += static_cast<size_t>(getIfsIncr()))
  {
    t += tStep;

    const uint32_t x = points[i].x & 0x7fffffff;
    const uint32_t y = points[i].y & 0x7fffffff;
    if ((x >= goomInfo->getScreenInfo().width) || (y >= goomInfo->getScreenInfo().height))
    {
      continue;
    }

    if (!doneColorChange && megaChangeColorMapEvent())
    {
      changeColormaps();
      doneColorChange = true;
    }

    drawPixel(prevBuff, currentBuff, points[i], color, t);

    if (points[i].count <= lowDensityCount)
    {
      lowDensityPoints.emplace_back(points[i]);
    }
  }

  blurrer.doBlur(lowDensityPoints, prevBuff);
}

void IfsFx::IfsImpl::updateLowDensityThreshold()
{
  lowDensityCount = getRandInRange(minDensityCount, maxDensityCount);

  uint32_t blurWidth;
  constexpr uint32_t numWidths = 3;
  constexpr uint32_t widthRange = (maxDensityCount - minDensityCount) / numWidths;
  if (lowDensityCount <= minDensityCount + widthRange)
  {
    blurWidth = 7;
  }
  else if (lowDensityCount <= minDensityCount + 2 * widthRange)
  {
    blurWidth = 5;
  }
  else
  {
    blurWidth = 3;
  }
  blurrer.setWidth(blurWidth);
}

void IfsFx::IfsImpl::updateColors()
{
  if (updateData.mode == ModType::MOD_MER)
  {
    updateColorsModeMer();
  }
  else if (updateData.mode == ModType::MOD_MERVER)
  {
    updateColorsModeMerver();
  }
  else if (updateData.mode == ModType::MOD_FEU)
  {
    updateColorsModeFeu();
  }
}

void IfsFx::IfsImpl::updateColorsModeMer()
{
  updateData.col[BLEU] += updateData.v[BLEU];
  if (updateData.col[BLEU] > channel_limits<int32_t>::max())
  {
    updateData.col[BLEU] = channel_limits<int32_t>::max();
    updateData.v[BLEU] = -getRandInRange(1, 5);
  }
  if (updateData.col[BLEU] < 32)
  {
    updateData.col[BLEU] = 32;
    updateData.v[BLEU] = getRandInRange(1, 5);
  }

  updateData.col[VERT] += updateData.v[VERT];
  if (updateData.col[VERT] > 200)
  {
    updateData.col[VERT] = 200;
    updateData.v[VERT] = -getRandInRange(2, 5);
  }
  if (updateData.col[VERT] > updateData.col[BLEU])
  {
    updateData.col[VERT] = updateData.col[BLEU];
    updateData.v[VERT] = updateData.v[BLEU];
  }
  if (updateData.col[VERT] < 32)
  {
    updateData.col[VERT] = 32;
    updateData.v[VERT] = getRandInRange(2, 5);
  }

  updateData.col[ROUGE] += updateData.v[ROUGE];
  if (updateData.col[ROUGE] > 64)
  {
    updateData.col[ROUGE] = 64;
    updateData.v[ROUGE] = -getRandInRange(1, 5);
  }
  if (updateData.col[ROUGE] < 0)
  {
    updateData.col[ROUGE] = 0;
    updateData.v[ROUGE] = getRandInRange(1, 5);
  }

  updateData.col[ALPHA] += updateData.v[ALPHA];
  if (updateData.col[ALPHA] > 0)
  {
    updateData.col[ALPHA] = 0;
    updateData.v[ALPHA] = -getRandInRange(1, 5);
  }
  if (updateData.col[ALPHA] < 0)
  {
    updateData.col[ALPHA] = 0;
    updateData.v[ALPHA] = getRandInRange(1, 5);
  }

  if (((updateData.col[VERT] > 32) && (updateData.col[ROUGE] < updateData.col[VERT] + 40) &&
       (updateData.col[VERT] < updateData.col[ROUGE] + 20) && (updateData.col[BLEU] < 64) &&
       probabilityOfMInN(1, 20)) &&
      (updateData.justChanged < 0))
  {
    updateData.mode = probabilityOfMInN(2, 3) ? ModType::MOD_FEU : ModType::MOD_MERVER;
    updateData.justChanged = maxCountBeforeNextUpdate;
    renew();
  }
}

void IfsFx::IfsImpl::updateColorsModeMerver()
{
  updateData.col[BLEU] += updateData.v[BLEU];
  if (updateData.col[BLEU] > 128)
  {
    updateData.col[BLEU] = 128;
    updateData.v[BLEU] = -getRandInRange(1, 5);
  }
  if (updateData.col[BLEU] < 16)
  {
    updateData.col[BLEU] = 16;
    updateData.v[BLEU] = getRandInRange(1, 5);
  }

  updateData.col[VERT] += updateData.v[VERT];
  if (updateData.col[VERT] > 200)
  {
    updateData.col[VERT] = 200;
    updateData.v[VERT] = -getRandInRange(2, 5);
  }
  if (updateData.col[VERT] > updateData.col[ALPHA])
  {
    updateData.col[VERT] = updateData.col[ALPHA];
    updateData.v[VERT] = updateData.v[ALPHA];
  }
  if (updateData.col[VERT] < 32)
  {
    updateData.col[VERT] = 32;
    updateData.v[VERT] = getRandInRange(2, 5);
  }

  updateData.col[ROUGE] += updateData.v[ROUGE];
  if (updateData.col[ROUGE] > 128)
  {
    updateData.col[ROUGE] = 128;
    updateData.v[ROUGE] = -getRandInRange(1, 5);
  }
  if (updateData.col[ROUGE] < 0)
  {
    updateData.col[ROUGE] = 0;
    updateData.v[ROUGE] = getRandInRange(1, 5);
  }

  updateData.col[ALPHA] += updateData.v[ALPHA];
  if (updateData.col[ALPHA] > channel_limits<int32_t>::max())
  {
    updateData.col[ALPHA] = channel_limits<int32_t>::max();
    updateData.v[ALPHA] = -getRandInRange(1, 5);
  }
  if (updateData.col[ALPHA] < 0)
  {
    updateData.col[ALPHA] = 0;
    updateData.v[ALPHA] = getRandInRange(1, 5);
  }

  if (((updateData.col[VERT] > 32) && (updateData.col[ROUGE] < updateData.col[VERT] + 40) &&
       (updateData.col[VERT] < updateData.col[ROUGE] + 20) && (updateData.col[BLEU] < 64) &&
       probabilityOfMInN(1, 20)) &&
      (updateData.justChanged < 0))
  {
    updateData.mode = probabilityOfMInN(2, 3) ? ModType::MOD_FEU : ModType::MOD_MER;
    updateData.justChanged = maxCountBeforeNextUpdate;
    renew();
  }
}

void IfsFx::IfsImpl::updateColorsModeFeu()
{
  updateData.col[BLEU] += updateData.v[BLEU];
  if (updateData.col[BLEU] > 64)
  {
    updateData.col[BLEU] = 64;
    updateData.v[BLEU] = -getRandInRange(1, 5);
  }
  if (updateData.col[BLEU] < 0)
  {
    updateData.col[BLEU] = 0;
    updateData.v[BLEU] = getRandInRange(1, 5);
  }

  updateData.col[VERT] += updateData.v[VERT];
  if (updateData.col[VERT] > 200)
  {
    updateData.col[VERT] = 200;
    updateData.v[VERT] = -getRandInRange(2, 5);
  }
  if (updateData.col[VERT] > updateData.col[ROUGE] + 20)
  {
    updateData.col[VERT] = updateData.col[ROUGE] + 20;
    updateData.v[VERT] = -getRandInRange(2, 5);
    updateData.v[ROUGE] = getRandInRange(1, 5);
    updateData.v[BLEU] = getRandInRange(1, 5);
  }
  if (updateData.col[VERT] < 0)
  {
    updateData.col[VERT] = 0;
    updateData.v[VERT] = getRandInRange(2, 5);
  }

  updateData.col[ROUGE] += updateData.v[ROUGE];
  if (updateData.col[ROUGE] > channel_limits<int32_t>::max())
  {
    updateData.col[ROUGE] = channel_limits<int32_t>::max();
    updateData.v[ROUGE] = -getRandInRange(1, 5);
  }
  if (updateData.col[ROUGE] > updateData.col[VERT] + 40)
  {
    updateData.col[ROUGE] = updateData.col[VERT] + 40;
    updateData.v[ROUGE] = -getRandInRange(1, 5);
  }
  if (updateData.col[ROUGE] < 0)
  {
    updateData.col[ROUGE] = 0;
    updateData.v[ROUGE] = getRandInRange(1, 5);
  }

  updateData.col[ALPHA] += updateData.v[ALPHA];
  if (updateData.col[ALPHA] > 0)
  {
    updateData.col[ALPHA] = 0;
    updateData.v[ALPHA] = -getRandInRange(1, 5);
  }
  if (updateData.col[ALPHA] < 0)
  {
    updateData.col[ALPHA] = 0;
    updateData.v[ALPHA] = getRandInRange(1, 5);
  }

  if (((updateData.col[ROUGE] < 64) && (updateData.col[VERT] > 32) &&
       (updateData.col[VERT] < updateData.col[BLEU]) && (updateData.col[BLEU] > 32) &&
       probabilityOfMInN(1, 20)) &&
      (updateData.justChanged < 0))
  {
    updateData.mode = probabilityOfMInN(1, 2) ? ModType::MOD_MER : ModType::MOD_MERVER;
    updateData.justChanged = maxCountBeforeNextUpdate;
    renew();
  }
}

} // namespace goom
