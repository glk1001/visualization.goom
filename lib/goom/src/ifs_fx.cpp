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
#include <vector>

CEREAL_REGISTER_TYPE(goom::IfsFx);
CEREAL_REGISTER_POLYMORPHIC_RELATION(goom::VisualFx, goom::IfsFx);

namespace goom
{

using namespace goom::utils;

inline bool megaChangeColorMapEvent()
{
  return probabilityOfMInN(5, 10);
}

inline bool allowOverexposedEvent()
{
  return probabilityOfMInN(1, 50);
}

struct IfsPoint
{
  uint32_t x = 0;
  uint32_t y = 0;

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
  Dbl rMean;
  Dbl dr1Mean;
  Dbl dr2Mean;
};
// clang-format off
static const std::vector<CentreType> centreList = {
  { .depth = 10, .rMean = 0.7, .dr1Mean = 0.3, .dr2Mean = 0.4 },
  { .depth =  6, .rMean = 0.6, .dr1Mean = 0.4, .dr2Mean = 0.3 },
  { .depth =  4, .rMean = 0.5, .dr1Mean = 0.4, .dr2Mean = 0.3 },
  { .depth =  2, .rMean = 0.5, .dr1Mean = 0.4, .dr2Mean = 0.3 },
};
// clang-format on

struct Similitude
{
  Dbl c_x;
  Dbl c_y;
  Dbl r1;
  Dbl r2;
  Dbl A1;
  Dbl A2;
  Flt Ct1;
  Flt St1;
  Flt Ct2;
  Flt St2;
  Flt Cx;
  Flt Cy;
  Flt R1;
  Flt R2;

  bool operator==(const Similitude& s) const = default;
  /**
  bool operator==(const Similitude& s) const
  {
    return c_x == s.c_x && c_y == s.c_y && r == s.r && r2 == s.r2 && A == s.A && A2 == s.A2 &&
           Ct == s.Ct && St == s.St && Ct2 == s.Ct2 && St2 == s.St2 && Cx == s.Cx && Cy == s.Cy &&
           R == s.R && R2 == s.R2;
  }
  **/

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(c_x, c_y, r1, r2, A1, A2, Ct1, St1, Ct2, St2, Cx, Cy, R1, R2);
  };
};

class Fractal
{
public:
  Fractal() noexcept = default;
  explicit Fractal(const std::shared_ptr<const PluginInfo>&) noexcept;
  Fractal(const Fractal&) noexcept = delete;
  Fractal& operator=(const Fractal&) = delete;

  void init();

  uint32_t getSpeed() const { return speed; }
  void setSpeed(const uint32_t val) { speed = val; }

  uint32_t getCurPt() const { return curPt; }

  const std::vector<IfsPoint>& drawIfs();

  bool operator==(const Fractal&) const;

  template<class Archive>
  void serialize(Archive&);

private:
  static constexpr size_t maxSimi = 6;
  static constexpr size_t maxCountTimesSpeed = 1000;

  uint32_t lx = 0;
  uint32_t ly = 0;
  uint32_t numSimi = 0;
  uint32_t depth = 0;
  uint32_t count = 0;
  uint32_t speed = 6;
  Dbl rMean = 0;
  Dbl dr1Mean = 0;
  Dbl dr2Mean = 0;

  std::array<Similitude, 5 * maxSimi> components{};
  uint32_t maxPt = 0;
  uint32_t curPt = 0;

  std::vector<IfsPoint> buffer1{};
  std::vector<IfsPoint> buffer2{};
  IfsPoint* buff = nullptr;

  Flt getLx() const { return static_cast<Flt>(lx); }
  Flt getLy() const { return static_cast<Flt>(ly); }
  void drawFractal();
  void randomSimis(Similitude*, const size_t num);
  void trace(const FltPoint& po);
  static FltPoint transform(const Similitude&, const FltPoint& po);
  static Dbl gaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S);
  static Dbl halfGaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S);
  static constexpr Dbl get_1_minus_exp_neg_S(const Dbl S);
};

Fractal::Fractal(const std::shared_ptr<const PluginInfo>& goomInfo) noexcept
  : lx{(static_cast<uint32_t>(goomInfo->getScreenInfo().width) - 1) / 2},
    ly{(static_cast<uint32_t>(goomInfo->getScreenInfo().height) - 1) / 2}
{
  init();
}

void Fractal::init()
{
  const size_t numCentres = 2 + getNRand(centreList.size());

  depth = centreList.at(numCentres - 2).depth;
  rMean = centreList[numCentres - 2].rMean;
  dr1Mean = centreList[numCentres - 2].dr1Mean;
  dr2Mean = centreList[numCentres - 2].dr2Mean;

  numSimi = numCentres;
  maxPt = numSimi - 1;
  for (uint32_t i = 0; i <= depth + 2; i++)
  {
    maxPt *= numSimi;
  }

  if (maxPt > 100)
  {
    logWarn("maxPt = {}, numSimi = {}, numCentres = {}, depth = {}",
        maxPt, numSimi, numCentres, depth);
  }
  buffer1.resize(maxPt);
  buffer2.resize(maxPt);

  randomSimis(components.data(), 5 * maxSimi);
}

bool Fractal::operator==(const Fractal& f) const
{
  return numSimi == f.numSimi && components == f.components && depth == f.depth &&
         count == f.count && speed == f.speed && lx == f.lx && ly == f.ly && rMean == f.rMean &&
         dr1Mean == f.dr1Mean && dr2Mean == f.dr2Mean && curPt == f.curPt && maxPt == f.maxPt;
}

template<class Archive>
void Fractal::serialize(Archive& ar)
{
  ar(numSimi, components, depth, count, speed, lx, ly, rMean, dr1Mean, dr2Mean, curPt, maxPt);
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
    s[i].c_x = u0 * s0[i].c_x + u1 * s1[i].c_x + u2 * s2[i].c_x + u3 * s3[i].c_x;
    s[i].c_y = u0 * s0[i].c_y + u1 * s1[i].c_y + u2 * s2[i].c_y + u3 * s3[i].c_y;

    s[i].r1 = u0 * s0[i].r1 + u1 * s1[i].r1 + u2 * s2[i].r1 + u3 * s3[i].r1;
    s[i].r2 = u0 * s0[i].r2 + u1 * s1[i].r2 + u2 * s2[i].r2 + u3 * s3[i].r2;

    s[i].A1 = u0 * s0[i].A1 + u1 * s1[i].A1 + u2 * s2[i].A1 + u3 * s3[i].A1;
    s[i].A2 = u0 * s0[i].A2 + u1 * s1[i].A2 + u2 * s2[i].A2 + u3 * s3[i].A2;
  }

  drawFractal();

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
      s1[i].c_x = (2.0 * s3[i].c_x) - s2[i].c_x;
      s1[i].c_y = (2.0 * s3[i].c_y) - s2[i].c_y;

      s1[i].r1 = (2.0 * s3[i].r1) - s2[i].r1;
      s1[i].r2 = (2.0 * s3[i].r2) - s2[i].r2;

      s1[i].A1 = (2.0 * s3[i].A1) - s2[i].A1;
      s1[i].A2 = (2.0 * s3[i].A2) - s2[i].A2;

      s0[i] = s3[i];
    }

    randomSimis(components.data() + 3 * numSimi, numSimi);
    randomSimis(components.data() + 4 * numSimi, numSimi);

    count = 0;
  }

  return buffer2;
}

void Fractal::drawFractal()
{
  for (size_t i = 0; i < numSimi; i++)
  {
    Similitude& simi = components[i];

    simi.Cx = dbl_to_flt(simi.c_x);
    simi.Cy = dbl_to_flt(simi.c_y);

    simi.Ct1 = dbl_to_flt(cos(simi.A1));
    simi.St1 = dbl_to_flt(sin(simi.A1));
    simi.Ct2 = dbl_to_flt(cos(simi.A2));
    simi.St2 = dbl_to_flt(sin(simi.A2));

    simi.R1 = dbl_to_flt(simi.r1);
    simi.R2 = dbl_to_flt(simi.r2);
  }

  curPt = 0;
  buff = buffer2.data();

  for (size_t i = 0; i < numSimi; i++)
  {
    const Similitude& cur = components[i];
    const FltPoint po{cur.Cx, cur.Cy};
    for (size_t j = 0; j < numSimi; j++)
    {
      if (i != j)
      {
        trace(transform(components[j], po));
      }
    }
  }

  // Erase previous
  buff = buffer1.data();
  std::swap(buffer1, buffer2);
}


constexpr Dbl Fractal::get_1_minus_exp_neg_S(const Dbl S)
{
  return 1.0 - std::exp(-S);
}

Dbl Fractal::gaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S)
{
  const Dbl x = getRandInRange(0.0f, 1.0f);
  const Dbl y = A_mult_1_minus_exp_neg_S * (1.0 - exp(-x * x * S));
  return probabilityOfMInN(1, 2) ? c + y : c - y;
}

Dbl Fractal::halfGaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S)
{
  const Dbl x = getRandInRange(0.0f, 1.0f);
  const Dbl y = A_mult_1_minus_exp_neg_S * (1.0 - exp(-x * x * S));
  return c + y;
}

void Fractal::randomSimis(Similitude* simi, const size_t num)
{
  static const constinit Dbl c_AS_factor = 0.8f * get_1_minus_exp_neg_S(4.0);
  static const constinit Dbl r1_1_minus_exp_neg_S = get_1_minus_exp_neg_S(3.0);
  static const constinit Dbl r2_1_minus_exp_neg_S = get_1_minus_exp_neg_S(2.0);
  static const constinit Dbl A_AS_factor = 360.0F * get_1_minus_exp_neg_S(4.0);
  static const constinit Dbl A2_AS_factor = A_AS_factor;

  const Dbl r1_AS_factor = dr1Mean * r1_1_minus_exp_neg_S;
  const Dbl r2_AS_factor = dr2Mean * r2_1_minus_exp_neg_S;

  for (size_t i = 0; i < num; i++)
  {
    simi->c_x = gaussRand(0.0, 4.0, c_AS_factor);
    simi->c_y = gaussRand(0.0, 4.0, c_AS_factor);
    simi->r1 = gaussRand(rMean, 3.0, r1_AS_factor);
    simi->r2 = halfGaussRand(0.0, 2.0, r2_AS_factor);
    simi->A1 = gaussRand(0.0, 4.0, A_AS_factor) * (m_pi / 180.0);
    simi->A2 = gaussRand(0.0, 4.0, A2_AS_factor) * (m_pi / 180.0);
    simi->Ct1 = 0;
    simi->St1 = 0;
    simi->Ct2 = 0;
    simi->St2 = 0;
    simi->Cx = 0;
    simi->Cy = 0;
    simi->R1 = 0;
    simi->R2 = 0;

    simi++;
  }
}

inline FltPoint Fractal::transform(const Similitude& simi, const FltPoint& po)
{
  const Flt xo = div_by_unit((po.x - simi.Cx) * simi.R1);
  const Flt yo = div_by_unit((po.y - simi.Cy) * simi.R1);

  const Flt xx = div_by_unit((xo - simi.Cx) * simi.R2);
  // NOTE: changed '-yo - simi->Cy' to 'yo - simi->Cy' for symmetry
  //   reasons. Not sure it made any difference that's why I kept it.
  const Flt yy = div_by_unit((yo - simi.Cy) * simi.R2);

  return {
      div_by_unit(xo * simi.Ct1 - yo * simi.St1 + xx * simi.Ct2 - yy * simi.St2) + simi.Cx,
      div_by_unit(xo * simi.St1 + yo * simi.Ct1 + xx * simi.St2 + yy * simi.Ct2) + simi.Cy,
  };
}

void Fractal::trace(const FltPoint& po)
{
  for (size_t i = 0; i < numSimi; i++)
  {
    const FltPoint p = transform(components[i], po);

    buff->x = static_cast<uint32_t>(getLx() + div_by_2units(p.x * getLx()));
    buff->y = static_cast<uint32_t>(getLy() - div_by_2units(p.y * getLy()));
    buff++;

    curPt++;

    if (depth && ((p.x - po.x) >> 4) && ((p.y - po.y) >> 4))
    {
      depth--;
      trace(p);
      depth++;
    }
  }
}

class Colorizer
{
public:
  Colorizer() noexcept;
  Colorizer(const Colorizer&) = delete;
  Colorizer& operator=(const Colorizer&) = delete;

  const ColorMaps& getColorMaps() const;

  IfsFx::ColorMode getColorMode() const;
  void setForcedColorMode(const IfsFx::ColorMode);
  void changeColorMode();

  void changeColorMaps();

  Pixel getMixedColor(const Pixel& baseColor, const float tmix);

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
  const ColorMap* mixerMap;
  const ColorMap* prevMixerMap;
  mutable uint32_t countSinceColorMapChange = 0;
  static constexpr uint32_t minColorMapChangeCompleted = 100;
  static constexpr uint32_t maxColorMapChangeCompleted = 10000;
  uint32_t colorMapChangeCompleted = minColorMapChangeCompleted;

  IfsFx::ColorMode colorMode = IfsFx::ColorMode::mapColors;
  IfsFx::ColorMode forcedColorMode = IfsFx::ColorMode::_null;
  float tBetweenColors = 0.5; // in [0, 1]
  static IfsFx::ColorMode getNextColorMode();
  Pixel getNextMixerMapColor(const float t) const;
};

Colorizer::Colorizer() noexcept : mixerMap{&colorMaps.getRandomColorMap()}, prevMixerMap{mixerMap}
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

  auto mixerMapName = mixerMap->getMapName();
  ar(cereal::make_nvp("mixerMap", mixerMapName));
  auto prevMixerMapName = prevMixerMap->getMapName();
  ar(cereal::make_nvp("prevMixerMap", prevMixerMapName));
  mixerMap = &colorMaps.getColorMap(mixerMapName);
  prevMixerMap = &colorMaps.getColorMap(prevMixerMapName);
};

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
    { IfsFx::ColorMode::megaMapColorChange, 100 },
    { IfsFx::ColorMode::mixColors,           10 },
    { IfsFx::ColorMode::megaMixColorChange,  10 },
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
  prevMixerMap = mixerMap;
  mixerMap = &colorMaps.getRandomColorMap();
  //  logInfo("prevMixerMap = {}", enumToString(prevMixerMap->getMapName()));
  //  logInfo("mixerMap = {}", enumToString(mixerMap->getMapName()));
  colorMapChangeCompleted = getRandInRange(minColorMapChangeCompleted, maxColorMapChangeCompleted);
  tBetweenColors = getRandInRange(0.2F, 0.8F);
  countSinceColorMapChange = colorMapChangeCompleted;
}

inline Pixel Colorizer::getNextMixerMapColor(const float t) const
{
  const Pixel nextColor = mixerMap->getColor(t);
  if (countSinceColorMapChange == 0)
  {
    return nextColor;
  }
  const float tTransition =
      static_cast<float>(countSinceColorMapChange) / static_cast<float>(colorMapChangeCompleted);
  countSinceColorMapChange--;
  return ColorMap::colorMix(nextColor, prevMixerMap->getColor(t), tTransition);
}

inline Pixel Colorizer::getMixedColor(const Pixel& baseColor, const float tmix)
{
  switch (colorMode)
  {
    case IfsFx::ColorMode::mapColors:
    case IfsFx::ColorMode::megaMapColorChange:
    {
      const float tBright = static_cast<float>(getLuma(baseColor)) / channel_limits<float>::max();
      return getNextMixerMapColor(tBright);
    }
    case IfsFx::ColorMode::mixColors:
    case IfsFx::ColorMode::megaMixColorChange:
    {
      const Pixel mixColor = getNextMixerMapColor(tmix);
      return ColorMap::colorMix(mixColor, baseColor, tBetweenColors);
    }
    case IfsFx::ColorMode::reverseMixColors:
    {
      const Pixel mixColor = getNextMixerMapColor(tmix);
      return ColorMap::colorMix(baseColor, mixColor, tBetweenColors);
    }
    case IfsFx::ColorMode::singleColors:
    {
      return baseColor;
    }
    case IfsFx::ColorMode::sineMixColors:
    case IfsFx::ColorMode::sineMapColors:
    {
      static float freq = 20;
      static const float xStep = 0.1;
      static float x = 0;

      const Pixel mixColor = getNextMixerMapColor(0.5 * (1.0F + std::sin(freq * x)));
      x += xStep;
      if (colorMode == IfsFx::ColorMode::sineMapColors)
      {
        //logInfo("Returning color {:x}", mixColor.rgba());
        return mixColor;
      };
      return ColorMap::colorMix(baseColor, mixColor, tBetweenColors);
    }
    default:
      throw std::logic_error("Unknown ColorMode");
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
  IfsImpl() noexcept {}
  explicit IfsImpl(const std::shared_ptr<const PluginInfo>&) noexcept;
  ~IfsImpl() noexcept;
  IfsImpl(const IfsImpl&) = delete;
  IfsImpl& operator=(const IfsImpl&) = delete;

  void init();

  void applyNoDraw();
  void updateIfs(Pixel* prevBuff, Pixel* currentBuff);
  void setBuffSettings(const FXBuffSettings&);
  IfsFx::ColorMode getColorMode() const;
  void setColorMode(const IfsFx::ColorMode);
  void renew();
  void updateIncr();

  bool operator==(const IfsImpl&) const;

private:
  static constexpr int maxCountBeforeNextUpdate = 500;

  std::shared_ptr<const PluginInfo> goomInfo{};

  GoomDraw draw{};
  Colorizer colorizer{};
  bool useOldStyleDrawPixel = false;
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

  void changeColormaps();
  void updatePixelBuffers(Pixel* prevBuff,
                          Pixel* currentBuff,
                          const size_t numPoints,
                          const std::vector<IfsPoint>& points,
                          const Pixel& color);
  void drawPixel(Pixel* prevBuff,
                 Pixel* currentBuff,
                 const uint32_t x,
                 const uint32_t y,
                 const Pixel& ifsColor,
                 const float tmix);
  void updateColors();
  void updateColorsModeMer();
  void updateColorsModeMerver();
  void updateColorsModeFeu();
  int getIfsIncr() const;

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

IfsFx::~IfsFx() noexcept
{
}

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
}

void IfsFx::log(const StatsLogValueFunc&) const
{
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

void IfsFx::apply(Pixel* prevBuff, Pixel* currentBuff)
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
     CEREAL_NVP(useOldStyleDrawPixel), CEREAL_NVP(allowOverexposed),
     CEREAL_NVP(countSinceOverexposed), CEREAL_NVP(ifs_incr), CEREAL_NVP(decay_ifs),
     CEREAL_NVP(recay_ifs), CEREAL_NVP(updateData));
}

template<class Archive>
void IfsFx::IfsImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(fractal), CEREAL_NVP(draw), CEREAL_NVP(colorizer),
     CEREAL_NVP(useOldStyleDrawPixel), CEREAL_NVP(allowOverexposed),
     CEREAL_NVP(countSinceOverexposed), CEREAL_NVP(ifs_incr), CEREAL_NVP(decay_ifs),
     CEREAL_NVP(recay_ifs), CEREAL_NVP(updateData));
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
         useOldStyleDrawPixel == i.useOldStyleDrawPixel && allowOverexposed == i.allowOverexposed &&
         countSinceOverexposed == i.countSinceOverexposed && ifs_incr == i.ifs_incr &&
         decay_ifs == i.decay_ifs && recay_ifs == i.recay_ifs && updateData == i.updateData;
}

IfsFx::IfsImpl::IfsImpl(const std::shared_ptr<const PluginInfo>& info) noexcept
  : goomInfo{info},
    draw{goomInfo->getScreenInfo().width, goomInfo->getScreenInfo().height},
    fractal{std::make_unique<Fractal>(goomInfo)}
{
#ifndef NO_LOGGING
  Fractal* fractal = fractal.get();
  for (size_t i = 0; i < 5 * maxSimi; i++)
  {
    Similitude cur = fractal->components[i];
    logDebug("simi[{}]: c_x = {:.2}, c_y = {:.2}, r = {:.2}, r2 = {:.2}, A = {:.2}, A2 = {:.2}.", i,
             cur.c_x, cur.c_y, cur.r1, cur.r2, cur.A1, cur.A2);
  }
#endif
}

IfsFx::IfsImpl::~IfsImpl() noexcept
{
}

void IfsFx::IfsImpl::init()
{
  fractal->init();
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

void IfsFx::IfsImpl::renew()
{
  changeColormaps();
  colorizer.changeColorMode();
  updateAllowOverexposed();

  fractal->setSpeed(static_cast<uint32_t>(std::min(getRandInRange(1.11F, 20.0F), 10.0F) /
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

void IfsFx::IfsImpl::updateIfs(Pixel* prevBuff, Pixel* currentBuff)
{
  updateDecayAndRecay();

  if (getIfsIncr() <= 0)
  {
    return;
  }

  // TODO: trouver meilleur soluce pour increment (mettre le code de gestion de l'ifs dans ce fichier)
  //       find the best solution for increment (put the management code of the ifs in this file)
  useOldStyleDrawPixel = probabilityOfMInN(1, 50);

  updateData.cycle++;
  if (updateData.cycle >= 80)
  {
    updateData.cycle = 0;
  }

  const std::vector<IfsPoint>& points = fractal->drawIfs();
  const size_t numPoints = fractal->getCurPt() - 1;

  const int cycle10 = (updateData.cycle < 40) ? updateData.cycle / 10 : 7 - updateData.cycle / 10;
  const Pixel color = getRightShiftedChannels(updateData.couleur, cycle10);

  updatePixelBuffers(prevBuff, currentBuff, numPoints, points, color);

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
}

void IfsFx::IfsImpl::updateIncr()
{
  if (ifs_incr <= 0)
  {
    recay_ifs = 5;
    ifs_incr = 11;
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
}

inline int IfsFx::IfsImpl::getIfsIncr() const
{
  return ifs_incr;
}

inline void IfsFx::IfsImpl::drawPixel(Pixel* prevBuff,
                                      Pixel* currentBuff,
                                      const uint32_t x,
                                      const uint32_t y,
                                      const Pixel& ifsColor,
                                      const float tmix)
{
  if (useOldStyleDrawPixel)
  {
    const Pixel prevBuffColor = draw.getPixelRGB(prevBuff, x, y);
    const Pixel mixedColor = colorizer.getMixedColor(ifsColor, tmix);
    const Pixel finalColor = getColorAdd(prevBuffColor, mixedColor, allowOverexposed);
    draw.setPixelRGBNoBlend(currentBuff, x, y, finalColor);
  }
  else
  {
    // TODO buff right way around ??????????????????????????????????????????????????????????????
    std::vector<Pixel*> buffs{currentBuff, prevBuff};
    const std::vector<Pixel> colors{colorizer.getMixedColor(ifsColor, tmix), ifsColor};
    draw.setPixelRGB(buffs, x, y, colors);
  }
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

void IfsFx::IfsImpl::updatePixelBuffers(Pixel* prevBuff,
                                        Pixel* currentBuff,
                                        const size_t numPoints,
                                        const std::vector<IfsPoint>& points,
                                        const Pixel& color)
{
  bool doneColorChange = colorizer.getColorMode() != IfsFx::ColorMode::megaMapColorChange &&
                         colorizer.getColorMode() != IfsFx::ColorMode::megaMixColorChange;
  const float tStep = numPoints == 1 ? 0.0F : (1.0F - 0.0F) / static_cast<float>(numPoints - 1);
  float t = -tStep;

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

    drawPixel(prevBuff, currentBuff, x, y, color, t);
  }
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
  }
}

} // namespace goom
