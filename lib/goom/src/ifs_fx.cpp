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

#include "colorutils.h"
#include "goom_config.h"
#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_testing.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
// #undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"

#include <array>

#undef NDEBUG
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

inline bool changeCurrentColorMapEvent()
{
  return probabilityOfMInN(1, 10);
}

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

constexpr size_t maxSimi = 6;

#define MAX_DEPTH_2 10
#define MAX_DEPTH_3 6
#define MAX_DEPTH_4 4
#define MAX_DEPTH_5 2

struct Similitude
{
  Dbl c_x;
  Dbl c_y;
  Dbl r;
  Dbl r2;
  Dbl A;
  Dbl A2;
  Flt Ct;
  Flt St;
  Flt Ct2;
  Flt St2;
  Flt Cx;
  Flt Cy;
  Flt R;
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
    ar(c_x, c_y, r, r2, A, A2, Ct, St, Ct2, St2, Cx, Cy, R, R2);
  };
};

struct Fractal
{
  uint32_t numSimi = 0;
  std::array<Similitude, 5 * maxSimi> components{};
  uint32_t depth = 0;
  uint32_t count = 0;
  uint32_t speed = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t lx = 0;
  uint32_t ly = 0;
  Dbl rMean = 0;
  Dbl drMean = 0;
  Dbl dr2Mean = 0;
  uint32_t curPt = 0;
  uint32_t maxPt = 0;

  std::vector<IfsPoint> buffer1{};
  std::vector<IfsPoint> buffer2{};

  bool operator==(const Fractal& f) const
  {
    return numSimi == f.numSimi && components == f.components && depth == f.depth &&
           count == f.count && speed == f.speed && width == f.width && height == f.height &&
           lx == f.lx && ly == f.ly && rMean == f.rMean && drMean == f.drMean &&
           dr2Mean == f.dr2Mean && curPt == f.curPt && maxPt == f.maxPt;
  }

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(numSimi, components, depth, count, speed, width, height, lx, ly, rMean, drMean, dr2Mean,
       curPt, maxPt);
  };
};

class Colorizer
{
public:
  enum class ColorMode
  {
    mapColors,
    mixColors,
    reverseMixColors,
    megaMapColorChange,
    megaMixColorChange,
    singleColors,
  };

  Colorizer() noexcept;
  Colorizer(const Colorizer&) = delete;
  Colorizer& operator=(const Colorizer&) = delete;

  const ColorMaps& getColorMaps() const;

  ColorMode getColorMode() const;
  void changeColorMode();

  void changeColorMaps();

  Pixel getMixedColor(const Pixel& baseColor, const float tmix);

  bool operator==(const Colorizer& c) const
  {
    return countSinceColorMapChange == c.countSinceColorMapChange &&
           colorMapChangeCompleted == c.colorMapChangeCompleted && colorMode == c.colorMode &&
           tBetweenColors == c.tBetweenColors;
  }

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(countSinceColorMapChange, colorMapChangeCompleted, colorMode, tBetweenColors);

    auto mixerMapName = mixerMap->getMapName();
    ar(cereal::make_nvp("mixerMap", mixerMapName));
    auto prevMixerMapName = prevMixerMap->getMapName();
    ar(cereal::make_nvp("prevMixerMap", prevMixerMapName));
    mixerMap = &colorMaps.getColorMap(mixerMapName);
    prevMixerMap = &colorMaps.getColorMap(prevMixerMapName);
  };

private:
  const ColorMaps colorMaps;
  ColorMapGroup currentColorMapGroup;
  const ColorMap* mixerMap;
  const ColorMap* prevMixerMap;
  mutable uint32_t countSinceColorMapChange = 0;
  static constexpr uint32_t minColorMapChangeCompleted = 100;
  static constexpr uint32_t maxColorMapChangeCompleted = 10000;
  uint32_t colorMapChangeCompleted = minColorMapChangeCompleted;

  ColorMode colorMode;
  float tBetweenColors; // in [0, 1]
  static ColorMode getNextColorMode();
  Pixel getNextMixerMapColor(const float t) const;
};

Colorizer::Colorizer() noexcept
  : colorMaps{},
    currentColorMapGroup{colorMaps.getRandomGroup()},
    mixerMap{&colorMaps.getRandomColorMap(currentColorMapGroup)},
    prevMixerMap{mixerMap},
    colorMode{ColorMode::mapColors},
    tBetweenColors{0.5}
{
}

inline const ColorMaps& Colorizer::getColorMaps() const
{
  return colorMaps;
}

inline Colorizer::ColorMode Colorizer::getColorMode() const
{
  return colorMode;
}

void Colorizer::changeColorMode()
{
  colorMode = getNextColorMode();
}

Colorizer::ColorMode Colorizer::getNextColorMode()
{
  // clang-format off
  static const Weights<Colorizer::ColorMode> colorModeWeights{{
    { Colorizer::ColorMode::mapColors,           15 },
    { Colorizer::ColorMode::megaMapColorChange, 100 },
    { Colorizer::ColorMode::mixColors,           10 },
    { Colorizer::ColorMode::megaMixColorChange,  10 },
    { Colorizer::ColorMode::reverseMixColors,    10 },
    { Colorizer::ColorMode::singleColors,         5 },
  }};
  // clang-format on

  return colorModeWeights.getRandomWeighted();
}

void Colorizer::changeColorMaps()
{
  if (changeCurrentColorMapEvent())
  {
    currentColorMapGroup = colorMaps.getRandomGroup();
  }
  prevMixerMap = mixerMap;
  mixerMap = &colorMaps.getRandomColorMap(currentColorMapGroup);
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
    case ColorMode::mapColors:
    case ColorMode::megaMapColorChange:
    {
      const float tBright = static_cast<float>(getLuma(baseColor)) / channel_limits<float>::max();
      return getNextMixerMapColor(tBright);
    }
    case ColorMode::mixColors:
    case ColorMode::megaMixColorChange:
    {
      const Pixel mixColor = getNextMixerMapColor(tmix);
      return ColorMap::colorMix(mixColor, baseColor, tBetweenColors);
    }
    case ColorMode::reverseMixColors:
    {
      const Pixel mixColor = getNextMixerMapColor(tmix);
      return ColorMap::colorMix(baseColor, mixColor, tBetweenColors);
    }
    case ColorMode::singleColors:
    {
      return baseColor;
    }
    default:
      throw std::logic_error("Unknown ColorMode");
  }
}

#define MOD_MER 0
#define MOD_FEU 1
#define MOD_MERVER 2

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
  int mode = MOD_MERVER;
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

  void applyNoDraw();
  void updateIfs(Pixel* prevBuff, Pixel* currentBuff);
  void setBuffSettings(const FXBuffSettings&);
  void renew();
  void updateIncr();

  bool operator==(const IfsImpl&) const;

private:
  std::shared_ptr<const PluginInfo> goomInfo{};

  GoomDraw draw{};
  Colorizer colorizer{};
  bool useOldStyleDrawPixel = false;
  FXBuffSettings buffSettings{};

  bool allowOverexposed = true;
  uint32_t countSinceOverexposed = 0;
  static constexpr uint32_t maxCountSinceOverexposed = 100;
  void updateAllowOverexposed();

  std::unique_ptr<Fractal> root = nullptr;
  Fractal* curF = nullptr;

  // Used by the Trace recursive method
  IfsPoint* buff = nullptr;
  size_t curPt = 0;
  void trace(Fractal*, const Flt xo, const Flt yo);
  static void transform(Similitude*, Flt xo, Flt yo, Flt* x, Flt* y);

  bool initialized = false;
  int ifs_incr = 1; // dessiner l'ifs (0 = non: > = increment)
  int decay_ifs = 0; // disparition de l'ifs
  int recay_ifs = 0; // dedisparition de l'ifs
  void updateDecay();
  void updateDecayAndRecay();
  static Dbl gaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S);
  static Dbl halfGaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S);
  static constexpr Dbl get_1_minus_exp_neg_S(const Dbl S);
  const std::vector<IfsPoint>& drawIfs();
  void drawFractal();

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
  static void randomSimis(const Fractal*, Similitude* cur, uint32_t i);
  static constexpr int fix = 12;
  static Flt dbl_to_flt(const Dbl);
  static Flt div_by_unit(const Flt);
  static Flt div_by_2units(const Flt);

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
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(root), CEREAL_NVP(draw), CEREAL_NVP(colorizer),
     CEREAL_NVP(useOldStyleDrawPixel), CEREAL_NVP(allowOverexposed),
     CEREAL_NVP(countSinceOverexposed), CEREAL_NVP(curPt), CEREAL_NVP(ifs_incr),
     CEREAL_NVP(decay_ifs), CEREAL_NVP(recay_ifs), CEREAL_NVP(updateData));
}

template<class Archive>
void IfsFx::IfsImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(root), CEREAL_NVP(draw), CEREAL_NVP(colorizer),
     CEREAL_NVP(useOldStyleDrawPixel), CEREAL_NVP(allowOverexposed),
     CEREAL_NVP(countSinceOverexposed), CEREAL_NVP(curPt), CEREAL_NVP(ifs_incr),
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
         *root == *i.root && draw == i.draw && colorizer == i.colorizer &&
         useOldStyleDrawPixel == i.useOldStyleDrawPixel && allowOverexposed == i.allowOverexposed &&
         countSinceOverexposed == i.countSinceOverexposed && curPt == i.curPt &&
         ifs_incr == i.ifs_incr && decay_ifs == i.decay_ifs && recay_ifs == i.recay_ifs &&
         updateData == i.updateData;
}

IfsFx::IfsImpl::IfsImpl(const std::shared_ptr<const PluginInfo>& info) noexcept
  : goomInfo{info}, draw{goomInfo->getScreenInfo().width, goomInfo->getScreenInfo().height}
{
  root = std::make_unique<Fractal>();

  Fractal* fractal = root.get();

  const uint32_t numCentres = getNRand(4) + 2;
  switch (numCentres)
  {
    case 3:
      fractal->depth = MAX_DEPTH_3;
      fractal->rMean = .6;
      fractal->drMean = .4;
      fractal->dr2Mean = .3;
      break;

    case 4:
      fractal->depth = MAX_DEPTH_4;
      fractal->rMean = .5;
      fractal->drMean = .4;
      fractal->dr2Mean = .3;
      break;

    case 5:
      fractal->depth = MAX_DEPTH_5;
      fractal->rMean = .5;
      fractal->drMean = .4;
      fractal->dr2Mean = .3;
      break;

    default:
    case 2:
      fractal->depth = MAX_DEPTH_2;
      fractal->rMean = .7;
      fractal->drMean = .3;
      fractal->dr2Mean = .4;
      break;
  }

  fractal->numSimi = numCentres;
  fractal->maxPt = fractal->numSimi - 1;
  for (uint32_t i = 0; i <= fractal->depth + 2; i++)
  {
    fractal->maxPt *= fractal->numSimi;
  }

  fractal->buffer1.resize(fractal->maxPt);
  fractal->buffer2.resize(fractal->maxPt);

  fractal->speed = 6;
  fractal->width = goomInfo->getScreenInfo().width; // modif by JeKo
  fractal->height = goomInfo->getScreenInfo().height; // modif by JeKo
  fractal->curPt = 0;
  fractal->count = 0;
  fractal->lx = (fractal->width - 1) / 2;
  fractal->ly = (fractal->height - 1) / 2;

  randomSimis(fractal, fractal->components.data(), 5 * maxSimi);

#ifndef NO_LOGGING
  for (size_t i = 0; i < 5 * maxSimi; i++)
  {
    Similitude cur = fractal->components[i];
    logDebug("simi[{}]: c_x = {:.2}, c_y = {:.2}, r = {:.2}, r2 = {:.2}, A = {:.2}, A2 = {:.2}.", i,
             cur.c_x, cur.c_y, cur.r, cur.r2, cur.A, cur.A2);
  }
#endif
}

IfsFx::IfsImpl::~IfsImpl() noexcept
{
}

void IfsFx::IfsImpl::setBuffSettings(const FXBuffSettings& settings)
{
  buffSettings = settings;
}

void IfsFx::IfsImpl::renew()
{
  changeColormaps();
  colorizer.changeColorMode();
  updateAllowOverexposed();

  root->speed = static_cast<uint32_t>(getRandInRange(1.11F, 5.1F) /
                                      (1.1F - goomInfo->getSoundInfo().getAcceleration()));
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

  const std::vector<IfsPoint>& points = drawIfs();
  const size_t numPoints = curPt - 1;

  const int cycle10 = (updateData.cycle < 40) ? updateData.cycle / 10 : 7 - updateData.cycle / 10;
  const Pixel color = getRightShiftedChannels(updateData.couleur, cycle10);

  updatePixelBuffers(prevBuff, currentBuff, numPoints, points, color);

  updateData.justChanged--;

  updateData.col = getChannelArray(updateData.couleur);

  updateColors();

  updateData.couleur = getPixel(updateData.col);

  logDebug("updateData.col[ALPHA] = {}", updateData.col[ALPHA]);
  logDebug("updateData.col[BLEU] = {}", updateData.col[BLEU]);
  logDebug("updateData.col[VERT] = {}", updateData.col[VERT]);
  logDebug("updateData.col[ROUGE] = {}", updateData.col[ROUGE]);

  logDebug("updateData.v[ALPHA] = {}", updateData.v[ALPHA]);
  logDebug("updateData.v[BLEU] = {}", updateData.v[BLEU]);
  logDebug("updateData.v[VERT] = {}", updateData.v[VERT]);
  logDebug("updateData.v[ROUGE] = {}", updateData.v[ROUGE]);

  logDebug("updateData.mode = {}", updateData.mode);
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

Dbl IfsFx::IfsImpl::gaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S)
{
  const Dbl x = getRandInRange(0.0f, 1.0f);
  const Dbl y = A_mult_1_minus_exp_neg_S * (1.0 - exp(-x * x * S));
  return probabilityOfMInN(1, 2) ? c + y : c - y;
}

Dbl IfsFx::IfsImpl::halfGaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S)
{
  const Dbl x = getRandInRange(0.0f, 1.0f);
  const Dbl y = A_mult_1_minus_exp_neg_S * (1.0 - exp(-x * x * S));
  return c + y;
}

constexpr Dbl IfsFx::IfsImpl::get_1_minus_exp_neg_S(const Dbl S)
{
  return 1.0 - std::exp(-S);
}

void IfsFx::IfsImpl::randomSimis(const Fractal* fractal, Similitude* cur, uint32_t i)
{
  static const constinit Dbl c_AS_factor = 0.8f * get_1_minus_exp_neg_S(4.0);
  static const constinit Dbl r_1_minus_exp_neg_S = get_1_minus_exp_neg_S(3.0);
  static const constinit Dbl r2_1_minus_exp_neg_S = get_1_minus_exp_neg_S(2.0);
  static const constinit Dbl A_AS_factor = 360.0F * get_1_minus_exp_neg_S(4.0);
  static const constinit Dbl A2_AS_factor = A_AS_factor;

  const Dbl r_AS_factor = fractal->drMean * r_1_minus_exp_neg_S;
  const Dbl r2_AS_factor = fractal->dr2Mean * r2_1_minus_exp_neg_S;

  while (i--)
  {
    cur->c_x = gaussRand(0.0, 4.0, c_AS_factor);
    cur->c_y = gaussRand(0.0, 4.0, c_AS_factor);
    cur->r = gaussRand(fractal->rMean, 3.0, r_AS_factor);
    cur->r2 = halfGaussRand(0.0, 2.0, r2_AS_factor);
    cur->A = gaussRand(0.0, 4.0, A_AS_factor) * (m_pi / 180.0);
    cur->A2 = gaussRand(0.0, 4.0, A2_AS_factor) * (m_pi / 180.0);
    cur->Ct = 0;
    cur->St = 0;
    cur->Ct2 = 0;
    cur->St2 = 0;
    cur->Cx = 0;
    cur->Cy = 0;
    cur->R = 0;
    cur->R2 = 0;
    cur++;
  }
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

inline void IfsFx::IfsImpl::transform(Similitude* Simi, Flt xo, Flt yo, Flt* x, Flt* y)
{
  xo = xo - Simi->Cx;
  xo = div_by_unit(xo * Simi->R);
  yo = yo - Simi->Cy;
  yo = div_by_unit(yo * Simi->R);

  Flt xx = xo - Simi->Cx;
  xx = div_by_unit(xx * Simi->R2);
  Flt yy = -yo - Simi->Cy;
  yy = div_by_unit(yy * Simi->R2);

  *x = div_by_unit(xo * Simi->Ct - yo * Simi->St + xx * Simi->Ct2 - yy * Simi->St2) + Simi->Cx;
  *y = div_by_unit(xo * Simi->St + yo * Simi->Ct + xx * Simi->St2 + yy * Simi->Ct2) + Simi->Cy;
}

void IfsFx::IfsImpl::trace(Fractal* F, const Flt xo, const Flt yo)
{
  Similitude* Cur = curF->components.data();
  //  logDebug("data->Cur_F->numSimi = {}, xo = {}, yo = {}", data->Cur_F->numSimi, xo, yo);
  for (int i = static_cast<int>(curF->numSimi); i; --i, Cur++)
  {
    Flt x, y;
    transform(Cur, xo, yo, &x, &y);

    buff->x =
        static_cast<uint32_t>(static_cast<Flt>(F->lx) + div_by_2units(x * static_cast<int>(F->lx)));
    buff->y =
        static_cast<uint32_t>(static_cast<Flt>(F->ly) - div_by_2units(y * static_cast<int>(F->ly)));
    buff++;

    curPt++;

    if (F->depth && ((x - xo) >> 4) && ((y - yo) >> 4))
    {
      F->depth--;
      trace(F, x, y);
      F->depth++;
    }
  }
}

inline Flt IfsFx::IfsImpl::dbl_to_flt(const Dbl x)
{
  constexpr int unit = 1 << fix;
  return static_cast<Flt>(static_cast<Dbl>(unit) * x);
}

inline Flt IfsFx::IfsImpl::div_by_unit(const Flt x)
{
  return x >> fix;
}

inline Flt IfsFx::IfsImpl::div_by_2units(const Flt x)
{
  return x >> (fix + 1);
}

const std::vector<IfsPoint>& IfsFx::IfsImpl::drawIfs()
{
  Fractal* fractal = root.get();

  const Dbl u = static_cast<Dbl>(fractal->count) * static_cast<Dbl>(fractal->speed) / 1000.0;
  const Dbl uu = u * u;
  const Dbl v = 1.0 - u;
  const Dbl vv = v * v;
  const Dbl u0 = vv * v;
  const Dbl u1 = 3.0 * vv * u;
  const Dbl u2 = 3.0 * v * uu;
  const Dbl u3 = u * uu;

  Similitude* S = fractal->components.data();
  Similitude* S1 = S + fractal->numSimi;
  Similitude* S2 = S1 + fractal->numSimi;
  Similitude* S3 = S2 + fractal->numSimi;
  Similitude* S4 = S3 + fractal->numSimi;

  for (int i = static_cast<int>(fractal->numSimi); i; --i, S++, S1++, S2++, S3++, S4++)
  {
    S->c_x = u0 * S1->c_x + u1 * S2->c_x + u2 * S3->c_x + u3 * S4->c_x;
    S->c_y = u0 * S1->c_y + u1 * S2->c_y + u2 * S3->c_y + u3 * S4->c_y;
    S->r = u0 * S1->r + u1 * S2->r + u2 * S3->r + u3 * S4->r;
    S->r2 = u0 * S1->r2 + u1 * S2->r2 + u2 * S3->r2 + u3 * S4->r2;
    S->A = u0 * S1->A + u1 * S2->A + u2 * S3->A + u3 * S4->A;
    S->A2 = u0 * S1->A2 + u1 * S2->A2 + u2 * S3->A2 + u3 * S4->A2;
  }

  drawFractal();

  if (fractal->count < 1000 / fractal->speed)
  {
    fractal->count++;
  }
  else
  {
    S = fractal->components.data();
    S1 = S + fractal->numSimi;
    S2 = S1 + fractal->numSimi;
    S3 = S2 + fractal->numSimi;
    S4 = S3 + fractal->numSimi;

    for (int i = static_cast<int>(fractal->numSimi); i; --i, S++, S1++, S2++, S3++, S4++)
    {
      S2->c_x = 2.0 * S4->c_x - S3->c_x;
      S2->c_y = 2.0 * S4->c_y - S3->c_y;
      S2->r = 2.0 * S4->r - S3->r;
      S2->r2 = 2.0 * S4->r2 - S3->r2;
      S2->A = 2.0 * S4->A - S3->A;
      S2->A2 = 2.0 * S4->A2 - S3->A2;

      *S1 = *S4;
    }

    IfsImpl::randomSimis(fractal, fractal->components.data() + 3 * fractal->numSimi,
                         fractal->numSimi);
    IfsImpl::randomSimis(fractal, fractal->components.data() + 4 * fractal->numSimi,
                         fractal->numSimi);

    fractal->count = 0;
  }

  return fractal->buffer2;
}

void IfsFx::IfsImpl::drawFractal()
{
  Fractal* fractal = root.get();
  int i;
  Similitude* Cur;
  for (Cur = fractal->components.data(), i = static_cast<int>(fractal->numSimi); i; --i, Cur++)
  {
    Cur->Cx = IfsImpl::dbl_to_flt(Cur->c_x);
    Cur->Cy = IfsImpl::dbl_to_flt(Cur->c_y);

    Cur->Ct = IfsImpl::dbl_to_flt(cos(Cur->A));
    Cur->St = IfsImpl::dbl_to_flt(sin(Cur->A));
    Cur->Ct2 = IfsImpl::dbl_to_flt(cos(Cur->A2));
    Cur->St2 = IfsImpl::dbl_to_flt(sin(Cur->A2));

    Cur->R = IfsImpl::dbl_to_flt(Cur->r);
    Cur->R2 = IfsImpl::dbl_to_flt(Cur->r2);
  }

  curPt = 0;
  curF = fractal;
  buff = fractal->buffer2.data();
  int j;
  for (Cur = fractal->components.data(), i = static_cast<int>(fractal->numSimi); i; --i, Cur++)
  {
    const Flt xo = Cur->Cx;
    const Flt yo = Cur->Cy;
    logDebug("F->numSimi = {}, xo = {}, yo = {}", fractal->numSimi, xo, yo);
    Similitude* Simi;
    for (Simi = fractal->components.data(), j = static_cast<int>(fractal->numSimi); j; --j, Simi++)
    {
      if (Simi == Cur)
      {
        continue;
      }
      Flt x;
      Flt y;
      IfsImpl::transform(Simi, xo, yo, &x, &y);
      trace(fractal, x, y);
    }
  }

  // Erase previous
  fractal->curPt = curPt;
  buff = fractal->buffer1.data();
  std::swap(fractal->buffer1, fractal->buffer2);
}

void IfsFx::IfsImpl::updatePixelBuffers(Pixel* prevBuff,
                                        Pixel* currentBuff,
                                        const size_t numPoints,
                                        const std::vector<IfsPoint>& points,
                                        const Pixel& color)
{
  bool doneColorChange = colorizer.getColorMode() != Colorizer::ColorMode::megaMapColorChange &&
                         colorizer.getColorMode() != Colorizer::ColorMode::megaMixColorChange;
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
  if (updateData.mode == MOD_MER)
  {
    updateColorsModeMer();
  }
  else if (updateData.mode == MOD_MERVER)
  {
    updateColorsModeMerver();
  }
  else if (updateData.mode == MOD_FEU)
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
    updateData.mode = probabilityOfMInN(2, 3) ? MOD_FEU : MOD_MERVER;
    updateData.justChanged = 250;
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
    updateData.mode = probabilityOfMInN(2, 3) ? MOD_FEU : MOD_MER;
    updateData.justChanged = 250;
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
    updateData.mode = probabilityOfMInN(1, 2) ? MOD_MER : MOD_MERVER;
    updateData.justChanged = 250;
  }
}

} // namespace goom
