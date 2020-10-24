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
#include <cassert>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace goom
{

using namespace goom::utils;

inline bool changeCurrentColorMapEvent()
{
  return probabilityOfMInN(1, 10);
}

inline bool megaChangeColorMapEvent()
{
  return probabilityOfMInN(1, 10);
}

inline bool allowOverexposedEvent()
{
  return probabilityOfMInN(1, 50);
}

struct IfsPoint
{
  uint32_t x = 0;
  uint32_t y = 0;
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
};

struct Fractal
{
  uint32_t numSimi;
  Similitude components[5 * maxSimi];
  uint32_t depth;
  uint32_t count;
  uint32_t speed;
  uint32_t width;
  uint32_t height;
  uint32_t lx;
  uint32_t ly;
  Dbl rMean;
  Dbl drMean;
  Dbl dr2Mean;
  uint32_t curPt;
  uint32_t maxPt;

  std::vector<IfsPoint> buffer1;
  std::vector<IfsPoint> buffer2;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(numSimi,
       //components[5 * maxSimi];
       depth, count, speed, width, height, lx, ly, rMean, drMean, dr2Mean, curPt, maxPt);
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

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(countSinceColorMapChange, colorMapChangeCompleted, colorMapChangeCompleted, colorMode,
       tBetweenColors);

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
    { Colorizer::ColorMode::megaMapColorChange,  70 },
    { Colorizer::ColorMode::mixColors,           10 },
    { Colorizer::ColorMode::megaMixColorChange,  10 },
    { Colorizer::ColorMode::reverseMixColors,    10 },
    { Colorizer::ColorMode::singleColors,         1 },
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
  const Pixel nextColor = Pixel{.val = mixerMap->getColor(t)};
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

struct IfsData
{
  IfsData(const uint32_t screenWidth, uint32_t screenHeight);
  ~IfsData();

  bool enabled = true;

  GoomDraw draw;
  Colorizer colorizer;
  void drawPixel(Pixel* prevBuff,
                 Pixel* currentBuff,
                 const uint32_t x,
                 const uint32_t y,
                 const Pixel& ifsColor,
                 const float tmix);
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
  bool initialized = false;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(colorizer, draw, useOldStyleDrawPixel, buffSettings, countSinceOverexposed, initialized,
       root);
  };

  static void randomSimis(const Fractal*, Similitude* cur, uint32_t i);
  static constexpr int fix = 12;
  static Flt dbl_to_flt(const Dbl);
  static Flt div_by_unit(const Flt);
  static Flt div_by_2units(const Flt);
  void trace(Fractal*, const Flt xo, const Flt yo);
  static void transform(Similitude*, Flt xo, Flt yo, Flt* x, Flt* y);

private:
  static Dbl gaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S);
  static Dbl halfGaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S);
  static constexpr Dbl get_1_minus_exp_neg_S(const Dbl S);
};

IfsData::IfsData(const uint32_t screenWidth, uint32_t screenHeight)
  : draw{screenWidth, screenHeight}, colorizer{}
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
  fractal->width = screenWidth; // modif by JeKo
  fractal->height = screenHeight; // modif by JeKo
  fractal->curPt = 0;
  fractal->count = 0;
  fractal->lx = (fractal->width - 1) / 2;
  fractal->ly = (fractal->height - 1) / 2;

  randomSimis(fractal, fractal->components, 5 * maxSimi);

  for (size_t i = 0; i < 5 * maxSimi; i++)
  {
    Similitude cur = fractal->components[i];
    logDebug("simi[{}]: c_x = {:.2}, c_y = {:.2}, r = {:.2}, r2 = {:.2}, A = {:.2}, A2 = {:.2}.", i,
             cur.c_x, cur.c_y, cur.r, cur.r2, cur.A, cur.A2);
  }
}

IfsData::~IfsData()
{
}

Dbl IfsData::gaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S)
{
  const Dbl x = getRandInRange(0.0f, 1.0f);
  const Dbl y = A_mult_1_minus_exp_neg_S * (1.0 - exp(-x * x * S));
  return probabilityOfMInN(1, 2) ? c + y : c - y;
}

Dbl IfsData::halfGaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S)
{
  const Dbl x = getRandInRange(0.0f, 1.0f);
  const Dbl y = A_mult_1_minus_exp_neg_S * (1.0 - exp(-x * x * S));
  return c + y;
}

constexpr Dbl IfsData::get_1_minus_exp_neg_S(const Dbl S)
{
  return 1.0 - std::exp(-S);
}

void IfsData::randomSimis(const Fractal* fractal, Similitude* cur, uint32_t i)
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

inline void IfsData::drawPixel(Pixel* prevBuff,
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

void IfsData::updateAllowOverexposed()
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

inline void IfsData::transform(Similitude* Simi, Flt xo, Flt yo, Flt* x, Flt* y)
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

void IfsData::trace(Fractal* F, const Flt xo, const Flt yo)
{
  Similitude* Cur = curF->components;
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

inline Flt IfsData::dbl_to_flt(const Dbl x)
{
  constexpr int unit = 1 << fix;
  return static_cast<Flt>(static_cast<Dbl>(unit) * x);
}

inline Flt IfsData::div_by_unit(const Flt x)
{
  return x >> fix;
}

inline Flt IfsData::div_by_2units(const Flt x)
{
  return x >> (fix + 1);
}


constexpr size_t numChannels = 4;
using Int32ChannelArray = std::array<int32_t, numChannels>;

#define MOD_MER 0
#define MOD_FEU 1
#define MOD_MERVER 2

struct IfsFx::IfsUpdateData
{
  Pixel couleur;
  Int32ChannelArray v;
  Int32ChannelArray col;
  int justChanged;
  int mode;
  int cycle;
};

inline Pixel getPixel(const Int32ChannelArray& col)
{
  Pixel p;
  for (size_t i = 0; i < numChannels; i++)
  {
    p.cop[i] = static_cast<uint8_t>(col[i]);
  }
  return p;
}

inline Int32ChannelArray getChannelArray(const Pixel& p)
{
  Int32ChannelArray a;
  for (size_t i = 0; i < numChannels; i++)
  {
    a[i] = p.cop[i];
  }
  return a;
}


IfsFx::IfsFx(PluginInfo* info)
  : goomInfo{info},
    fxData{new IfsData{goomInfo->screen.width, goomInfo->screen.height}},
    // clang-format off
    updateData{new IfsUpdateData{
      .couleur = { .val = 0xc0c0c0c0 },
      .v = { 2, 4, 3, 2 },
      .col = { 2, 4, 3, 2 },
      .justChanged = 0,
      .mode = MOD_MERVER,
      .cycle = 0,
    }}
// clang-format on
{
}

IfsFx::~IfsFx() noexcept
{
}

void IfsFx::setBuffSettings(const FXBuffSettings& settings)
{
  fxData->buffSettings = settings;
}

void IfsFx::start()
{
}

void IfsFx::finish()
{
  std::ofstream f("/tmp/ifs.json");
  saveState(f);
  f << std::endl;
  f.close();
}

void IfsFx::log(const StatsLogValueFunc&) const
{
}

std::string IfsFx::getFxName() const
{
  return "IFS FX";
}

void IfsFx::saveState(std::ostream& f)
{
  cereal::JSONOutputArchive archiveOut(f);
  archiveOut(*fxData);
}

void IfsFx::loadState(std::istream& f)
{
  cereal::JSONInputArchive archive_in(f);
  archive_in(*fxData);
}

void IfsFx::apply(Pixel* prevBuff, Pixel* currentBuff)
{
  if (!fxData->enabled)
  {
    return;
  }

  updateIfs(prevBuff, currentBuff);
}

void IfsFx::renew()
{
  changeColormaps();
  fxData->colorizer.changeColorMode();
  fxData->updateAllowOverexposed();

  fxData->root->speed = static_cast<uint32_t>(getRandInRange(1.11F, 5.1F) /
                                              (1.1F - goomInfo->getSoundInfo().getAcceleration()));
}

void IfsFx::changeColormaps()
{
  fxData->colorizer.changeColorMaps();
  updateData->couleur =
      ColorMap::getRandomColor(fxData->colorizer.getColorMaps().getRandomColorMap());
}

void IfsFx::updateIfs(Pixel* prevBuff, Pixel* currentBuff)
{
  // TODO: trouver meilleur soluce pour increment (mettre le code de gestion de l'ifs dans ce fichier)
  //       find the best solution for increment (put the management code of the ifs in this file)
  const int increment = goomInfo->update.ifs_incr;
  fxData->useOldStyleDrawPixel = probabilityOfMInN(1, 4);

  logDebug("increment = {}", increment);

  updateData->cycle++;
  if (updateData->cycle >= 80)
  {
    updateData->cycle = 0;
  }

  const std::vector<IfsPoint>& points = drawIfs();
  const size_t numPoints = fxData->curPt - 1;

  const int cycle10 =
      (updateData->cycle < 40) ? updateData->cycle / 10 : 7 - updateData->cycle / 10;
  const Pixel color = getRightShiftedChannels(updateData->couleur, cycle10);

  updatePixelBuffers(prevBuff, currentBuff, numPoints, points, increment, color);

  updateData->justChanged--;

  updateData->col = getChannelArray(updateData->couleur);

  updateColors();

  updateData->couleur = getPixel(updateData->col);

  logDebug("updateData.col[ALPHA] = {}", updateData->col[ALPHA]);
  logDebug("updateData.col[BLEU] = {}", updateData->col[BLEU]);
  logDebug("updateData.col[VERT] = {}", updateData->col[VERT]);
  logDebug("updateData.col[ROUGE] = {}", updateData->col[ROUGE]);

  logDebug("updateData.v[ALPHA] = {}", updateData->v[ALPHA]);
  logDebug("updateData.v[BLEU] = {}", updateData->v[BLEU]);
  logDebug("updateData.v[VERT] = {}", updateData->v[VERT]);
  logDebug("updateData.v[ROUGE] = {}", updateData->v[ROUGE]);

  logDebug("updateData.mode = {}", updateData->mode);
}

const std::vector<IfsPoint>& IfsFx::drawIfs()
{
  Fractal* fractal = fxData->root.get();

  const Dbl u = static_cast<Dbl>(fractal->count) * static_cast<Dbl>(fractal->speed) / 1000.0;
  const Dbl uu = u * u;
  const Dbl v = 1.0 - u;
  const Dbl vv = v * v;
  const Dbl u0 = vv * v;
  const Dbl u1 = 3.0 * vv * u;
  const Dbl u2 = 3.0 * v * uu;
  const Dbl u3 = u * uu;

  Similitude* S = fractal->components;
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
    S = fractal->components;
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

    IfsData::randomSimis(fractal, fractal->components + 3 * fractal->numSimi, fractal->numSimi);
    IfsData::randomSimis(fractal, fractal->components + 4 * fractal->numSimi, fractal->numSimi);

    fractal->count = 0;
  }

  return fractal->buffer2;
}

void IfsFx::drawFractal()
{
  Fractal* fractal = fxData->root.get();
  int i;
  Similitude* Cur;
  for (Cur = fractal->components, i = static_cast<int>(fractal->numSimi); i; --i, Cur++)
  {
    Cur->Cx = IfsData::dbl_to_flt(Cur->c_x);
    Cur->Cy = IfsData::dbl_to_flt(Cur->c_y);

    Cur->Ct = IfsData::dbl_to_flt(cos(Cur->A));
    Cur->St = IfsData::dbl_to_flt(sin(Cur->A));
    Cur->Ct2 = IfsData::dbl_to_flt(cos(Cur->A2));
    Cur->St2 = IfsData::dbl_to_flt(sin(Cur->A2));

    Cur->R = IfsData::dbl_to_flt(Cur->r);
    Cur->R2 = IfsData::dbl_to_flt(Cur->r2);
  }

  fxData->curPt = 0;
  fxData->curF = fractal;
  fxData->buff = fractal->buffer2.data();
  int j;
  for (Cur = fractal->components, i = static_cast<int>(fractal->numSimi); i; --i, Cur++)
  {
    const Flt xo = Cur->Cx;
    const Flt yo = Cur->Cy;
    logDebug("F->numSimi = {}, xo = {}, yo = {}", fractal->numSimi, xo, yo);
    Similitude* Simi;
    for (Simi = fractal->components, j = static_cast<int>(fractal->numSimi); j; --j, Simi++)
    {
      if (Simi == Cur)
      {
        continue;
      }
      Flt x;
      Flt y;
      IfsData::transform(Simi, xo, yo, &x, &y);
      fxData->trace(fractal, x, y);
    }
  }

  // Erase previous
  fractal->curPt = fxData->curPt;
  fxData->buff = fractal->buffer1.data();
  std::swap(fractal->buffer1, fractal->buffer2);
}

void IfsFx::updatePixelBuffers(Pixel* prevBuff,
                               Pixel* currentBuff,
                               const size_t numPoints,
                               const std::vector<IfsPoint>& points,
                               const int increment,
                               const Pixel& color)
{
  bool doneColorChange =
      fxData->colorizer.getColorMode() != Colorizer::ColorMode::megaMapColorChange &&
      fxData->colorizer.getColorMode() != Colorizer::ColorMode::megaMixColorChange;
  const float tStep = numPoints == 1 ? 0.0F : (1.0F - 0.0F) / static_cast<float>(numPoints - 1);
  float t = -tStep;

  for (size_t i = 0; i < numPoints; i += static_cast<size_t>(increment))
  {
    t += tStep;

    const uint32_t x = points[i].x & 0x7fffffff;
    const uint32_t y = points[i].y & 0x7fffffff;
    if ((x >= goomInfo->screen.width) || (y >= goomInfo->screen.height))
    {
      continue;
    }

    if (!doneColorChange && megaChangeColorMapEvent())
    {
      changeColormaps();
      doneColorChange = true;
    }

    fxData->drawPixel(prevBuff, currentBuff, x, y, color, t);
  }
}

void IfsFx::updateColors()
{
  if (updateData->mode == MOD_MER)
  {
    updateColorsModeMer();
  }
  else if (updateData->mode == MOD_MERVER)
  {
    updateColorsModeMerver();
  }
  else if (updateData->mode == MOD_FEU)
  {
    updateColorsModeFeu();
  }
}

void IfsFx::updateColorsModeMer()
{
  updateData->col[BLEU] += updateData->v[BLEU];
  if (updateData->col[BLEU] > channel_limits<int32_t>::max())
  {
    updateData->col[BLEU] = channel_limits<int32_t>::max();
    updateData->v[BLEU] = -getRandInRange(1, 5);
  }
  if (updateData->col[BLEU] < 32)
  {
    updateData->col[BLEU] = 32;
    updateData->v[BLEU] = getRandInRange(1, 5);
  }

  updateData->col[VERT] += updateData->v[VERT];
  if (updateData->col[VERT] > 200)
  {
    updateData->col[VERT] = 200;
    updateData->v[VERT] = -getRandInRange(2, 5);
  }
  if (updateData->col[VERT] > updateData->col[BLEU])
  {
    updateData->col[VERT] = updateData->col[BLEU];
    updateData->v[VERT] = updateData->v[BLEU];
  }
  if (updateData->col[VERT] < 32)
  {
    updateData->col[VERT] = 32;
    updateData->v[VERT] = getRandInRange(2, 5);
  }

  updateData->col[ROUGE] += updateData->v[ROUGE];
  if (updateData->col[ROUGE] > 64)
  {
    updateData->col[ROUGE] = 64;
    updateData->v[ROUGE] = -getRandInRange(1, 5);
  }
  if (updateData->col[ROUGE] < 0)
  {
    updateData->col[ROUGE] = 0;
    updateData->v[ROUGE] = getRandInRange(1, 5);
  }

  updateData->col[ALPHA] += updateData->v[ALPHA];
  if (updateData->col[ALPHA] > 0)
  {
    updateData->col[ALPHA] = 0;
    updateData->v[ALPHA] = -getRandInRange(1, 5);
  }
  if (updateData->col[ALPHA] < 0)
  {
    updateData->col[ALPHA] = 0;
    updateData->v[ALPHA] = getRandInRange(1, 5);
  }

  if (((updateData->col[VERT] > 32) && (updateData->col[ROUGE] < updateData->col[VERT] + 40) &&
       (updateData->col[VERT] < updateData->col[ROUGE] + 20) && (updateData->col[BLEU] < 64) &&
       probabilityOfMInN(1, 20)) &&
      (updateData->justChanged < 0))
  {
    updateData->mode = probabilityOfMInN(2, 3) ? MOD_FEU : MOD_MERVER;
    updateData->justChanged = 250;
  }
}

void IfsFx::updateColorsModeMerver()
{
  updateData->col[BLEU] += updateData->v[BLEU];
  if (updateData->col[BLEU] > 128)
  {
    updateData->col[BLEU] = 128;
    updateData->v[BLEU] = -getRandInRange(1, 5);
  }
  if (updateData->col[BLEU] < 16)
  {
    updateData->col[BLEU] = 16;
    updateData->v[BLEU] = getRandInRange(1, 5);
  }

  updateData->col[VERT] += updateData->v[VERT];
  if (updateData->col[VERT] > 200)
  {
    updateData->col[VERT] = 200;
    updateData->v[VERT] = -getRandInRange(2, 5);
  }
  if (updateData->col[VERT] > updateData->col[ALPHA])
  {
    updateData->col[VERT] = updateData->col[ALPHA];
    updateData->v[VERT] = updateData->v[ALPHA];
  }
  if (updateData->col[VERT] < 32)
  {
    updateData->col[VERT] = 32;
    updateData->v[VERT] = getRandInRange(2, 5);
  }

  updateData->col[ROUGE] += updateData->v[ROUGE];
  if (updateData->col[ROUGE] > 128)
  {
    updateData->col[ROUGE] = 128;
    updateData->v[ROUGE] = -getRandInRange(1, 5);
  }
  if (updateData->col[ROUGE] < 0)
  {
    updateData->col[ROUGE] = 0;
    updateData->v[ROUGE] = getRandInRange(1, 5);
  }

  updateData->col[ALPHA] += updateData->v[ALPHA];
  if (updateData->col[ALPHA] > channel_limits<int32_t>::max())
  {
    updateData->col[ALPHA] = channel_limits<int32_t>::max();
    updateData->v[ALPHA] = -getRandInRange(1, 5);
  }
  if (updateData->col[ALPHA] < 0)
  {
    updateData->col[ALPHA] = 0;
    updateData->v[ALPHA] = getRandInRange(1, 5);
  }

  if (((updateData->col[VERT] > 32) && (updateData->col[ROUGE] < updateData->col[VERT] + 40) &&
       (updateData->col[VERT] < updateData->col[ROUGE] + 20) && (updateData->col[BLEU] < 64) &&
       probabilityOfMInN(1, 20)) &&
      (updateData->justChanged < 0))
  {
    updateData->mode = probabilityOfMInN(2, 3) ? MOD_FEU : MOD_MER;
    updateData->justChanged = 250;
  }
}

void IfsFx::updateColorsModeFeu()
{
  updateData->col[BLEU] += updateData->v[BLEU];
  if (updateData->col[BLEU] > 64)
  {
    updateData->col[BLEU] = 64;
    updateData->v[BLEU] = -getRandInRange(1, 5);
  }
  if (updateData->col[BLEU] < 0)
  {
    updateData->col[BLEU] = 0;
    updateData->v[BLEU] = getRandInRange(1, 5);
  }

  updateData->col[VERT] += updateData->v[VERT];
  if (updateData->col[VERT] > 200)
  {
    updateData->col[VERT] = 200;
    updateData->v[VERT] = -getRandInRange(2, 5);
  }
  if (updateData->col[VERT] > updateData->col[ROUGE] + 20)
  {
    updateData->col[VERT] = updateData->col[ROUGE] + 20;
    updateData->v[VERT] = -getRandInRange(2, 5);
    updateData->v[ROUGE] = getRandInRange(1, 5);
    updateData->v[BLEU] = getRandInRange(1, 5);
  }
  if (updateData->col[VERT] < 0)
  {
    updateData->col[VERT] = 0;
    updateData->v[VERT] = getRandInRange(2, 5);
  }

  updateData->col[ROUGE] += updateData->v[ROUGE];
  if (updateData->col[ROUGE] > channel_limits<int32_t>::max())
  {
    updateData->col[ROUGE] = channel_limits<int32_t>::max();
    updateData->v[ROUGE] = -getRandInRange(1, 5);
  }
  if (updateData->col[ROUGE] > updateData->col[VERT] + 40)
  {
    updateData->col[ROUGE] = updateData->col[VERT] + 40;
    updateData->v[ROUGE] = -getRandInRange(1, 5);
  }
  if (updateData->col[ROUGE] < 0)
  {
    updateData->col[ROUGE] = 0;
    updateData->v[ROUGE] = getRandInRange(1, 5);
  }

  updateData->col[ALPHA] += updateData->v[ALPHA];
  if (updateData->col[ALPHA] > 0)
  {
    updateData->col[ALPHA] = 0;
    updateData->v[ALPHA] = -getRandInRange(1, 5);
  }
  if (updateData->col[ALPHA] < 0)
  {
    updateData->col[ALPHA] = 0;
    updateData->v[ALPHA] = getRandInRange(1, 5);
  }

  if (((updateData->col[ROUGE] < 64) && (updateData->col[VERT] > 32) &&
       (updateData->col[VERT] < updateData->col[BLEU]) && (updateData->col[BLEU] > 32) &&
       probabilityOfMInN(1, 20)) &&
      (updateData->justChanged < 0))
  {
    updateData->mode = probabilityOfMInN(1, 2) ? MOD_MER : MOD_MERVER;
    updateData->justChanged = 250;
  }
}

} // namespace goom
