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

struct IFSPoint
{
  uint32_t x = 0;
  uint32_t y = 0;
};

using Dbl = float;
using Flt = int;

constexpr int fix = 12;

inline Flt DBL_To_F_PT(const Dbl x)
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

  IFSPoint* buffer1;
  IFSPoint* buffer2;

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
  return Pixel{.val = ColorMap::colorMix(nextColor.val, prevMixerMap->getColor(t), tTransition)};
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
      const uint32_t mixColor = getNextMixerMapColor(tmix).val;
      return Pixel{.val = ColorMap::colorMix(mixColor, baseColor.val, tBetweenColors)};
    }
    case ColorMode::reverseMixColors:
    {
      const uint32_t mixColor = getNextMixerMapColor(tmix).val;
      return Pixel{.val = ColorMap::colorMix(baseColor.val, mixColor, tBetweenColors)};
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

  PluginParam enabled_bp;
  PluginParameters params;

  GoomDraw draw;
  Colorizer colorizer;
  void drawPixel(Pixel* prevBuff,
                 Pixel* currentBuff,
                 const uint32_t x,
                 const uint32_t y,
                 const Pixel& ifsColor,
                 const float tmix);
  bool useOldStyleDrawPixel = false;
  FXBuffSettings buffSettings = defaultFXBuffSettings;
  bool allowOverexposed = true;
  uint32_t countSinceOverexposed = 0;
  static constexpr uint32_t maxCountSinceOverexposed = 100;
  void updateAllowOverexposed();

  std::unique_ptr<Fractal> root = nullptr;
  Fractal* curF = nullptr;

  // Used by the Trace recursive method
  IFSPoint* buff = nullptr;
  size_t curPt = 0;
  bool initialized = false;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(colorizer, draw, useOldStyleDrawPixel, buffSettings, countSinceOverexposed, initialized,
       root);
  };
};

IfsData::IfsData(const uint32_t screenWidth, uint32_t screenHeight)
  : draw{screenWidth, screenHeight}, colorizer{}
{
}

static std::string getFxName(VisualFX*)
{
  return "IFS";
}

static void saveState(VisualFX* _this, std::ostream& f)
{
  const IfsData* data = static_cast<IfsData*>(_this->fx_data);
  cereal::JSONOutputArchive archiveOut(f);
  archiveOut(*data);
}

static void loadState(VisualFX* _this, std::istream& f)
{
  IfsData* data = static_cast<IfsData*>(_this->fx_data);
  cereal::JSONInputArchive archive_in(f);
  archive_in(*data);
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

static Dbl gaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S)
{
  const Dbl x = getRandInRange(0.0f, 1.0f);
  const Dbl y = A_mult_1_minus_exp_neg_S * (1.0 - exp(-x * x * S));
  return probabilityOfMInN(1, 2) ? c + y : c - y;
}

static Dbl halfGaussRand(const Dbl c, const Dbl S, const Dbl A_mult_1_minus_exp_neg_S)
{
  const Dbl x = getRandInRange(0.0f, 1.0f);
  const Dbl y = A_mult_1_minus_exp_neg_S * (1.0 - exp(-x * x * S));
  return c + y;
}

constexpr Dbl get_1_minus_exp_neg_S(const Dbl S)
{
  return 1.0 - std::exp(-S);
}

static void randomSimis(const Fractal* fractal, Similitude* cur, uint32_t i)
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

static void deleteIfsBuffers(Fractal* fractal)
{
  if (fractal->buffer1)
  {
    delete[] fractal->buffer1;
    fractal->buffer1 = nullptr;
  }
  if (fractal->buffer2)
  {
    delete[] fractal->buffer2;
    fractal->buffer2 = nullptr;
  }
}

static void initIfs(PluginInfo* goomInfo, IfsData* data)
{
  data->enabled_bp = secure_b_param("Enabled", 1);

  if (!data->root)
  {
    data->root = std::make_unique<Fractal>();
    if (!data->root)
    {
      return;
    }
    data->root->buffer1 = nullptr;
    data->root->buffer2 = nullptr;
  }

  Fractal* fractal = data->root.get();
  deleteIfsBuffers(fractal);

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

  fractal->buffer1 = new IFSPoint[fractal->maxPt]{};
  if (!fractal->buffer1)
  {
    deleteIfsBuffers(fractal);
    return;
  }
  fractal->buffer2 = new IFSPoint[fractal->maxPt]{};
  if (!fractal->buffer2)
  {
    deleteIfsBuffers(fractal);
    return;
  }

  fractal->speed = 6;
  fractal->width = goomInfo->screen.width; // modif by JeKo
  fractal->height = goomInfo->screen.height; // modif by JeKo
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

inline void transform(Similitude* Simi, Flt xo, Flt yo, Flt* x, Flt* y)
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

static void trace(Fractal* F, const Flt xo, const Flt yo, IfsData* data)
{
  Similitude* Cur = data->curF->components;
  //	logDebug("data->Cur_F->numSimi = {}, xo = {}, yo = {}", data->Cur_F->numSimi, xo, yo);
  for (int i = static_cast<int>(data->curF->numSimi); i; --i, Cur++)
  {
    Flt x, y;
    transform(Cur, xo, yo, &x, &y);

    data->buff->x =
        static_cast<uint32_t>(static_cast<Flt>(F->lx) + div_by_2units(x * static_cast<int>(F->lx)));
    data->buff->y =
        static_cast<uint32_t>(static_cast<Flt>(F->ly) - div_by_2units(y * static_cast<int>(F->ly)));
    data->buff++;

    data->curPt++;

    if (F->depth && ((x - xo) >> 4) && ((y - yo) >> 4))
    {
      F->depth--;
      trace(F, x, y, data);
      F->depth++;
    }
  }
}

static void drawFractal(IfsData* data)
{
  Fractal* fractal = data->root.get();
  int i;
  Similitude* Cur;
  for (Cur = fractal->components, i = static_cast<int>(fractal->numSimi); i; --i, Cur++)
  {
    Cur->Cx = DBL_To_F_PT(Cur->c_x);
    Cur->Cy = DBL_To_F_PT(Cur->c_y);

    Cur->Ct = DBL_To_F_PT(cos(Cur->A));
    Cur->St = DBL_To_F_PT(sin(Cur->A));
    Cur->Ct2 = DBL_To_F_PT(cos(Cur->A2));
    Cur->St2 = DBL_To_F_PT(sin(Cur->A2));

    Cur->R = DBL_To_F_PT(Cur->r);
    Cur->R2 = DBL_To_F_PT(Cur->r2);
  }

  data->curPt = 0;
  data->curF = fractal;
  data->buff = fractal->buffer2;
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
      transform(Simi, xo, yo, &x, &y);
      trace(fractal, x, y, data);
    }
  }

  /* Erase previous */
  fractal->curPt = data->curPt;
  data->buff = fractal->buffer1;
  fractal->buffer1 = fractal->buffer2;
  fractal->buffer2 = data->buff;
}

static IFSPoint* drawIfs(size_t* numPoints, IfsData* data)
{
  if (!data->root)
  {
    *numPoints = 0;
    return nullptr;
  }

  Fractal* fractal = data->root.get();
  if (!fractal->buffer1)
  {
    *numPoints = 0;
    return nullptr;
  }

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

  drawFractal(data);

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

    randomSimis(fractal, fractal->components + 3 * fractal->numSimi, fractal->numSimi);
    randomSimis(fractal, fractal->components + 4 * fractal->numSimi, fractal->numSimi);

    fractal->count = 0;
  }

  *numPoints = data->curPt;

  return fractal->buffer2;
}

static void releaseIfs(IfsData* data)
{
  if (data->root)
  {
    deleteIfsBuffers(data->root.get());
    data->root.release();
    data->root = nullptr;
  }
}

#define MOD_MER 0
#define MOD_FEU 1
#define MOD_MERVER 2

constexpr size_t numChannels = 4;
using Int32ChannelArray = std::array<int32_t, numChannels>;

struct IfsUpdateData
{
  Pixel couleur;
  Int32ChannelArray v;
  Int32ChannelArray col;
  int justChanged;
  int mode;
  int cycle;
};

// TODO Put this on IfsData ???????????????????????????????????????????????????????????????????
// clang-format off
static IfsUpdateData updData{
  .couleur = { .val = 0xc0c0c0c0 },
  .v = { 2, 4, 3, 2 },
  .col = { 2, 4, 3, 2 },
  .justChanged = 0,
  .mode = MOD_MERVER,
  .cycle = 0,
};
// clang-format on

static void changeColormaps(IfsData* fx_data)
{
  fx_data->colorizer.changeColorMaps();
  updData.couleur.val =
      ColorMap::getRandomColor(fx_data->colorizer.getColorMaps().getRandomColorMap());
}

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

static void updateColors(IfsUpdateData*);
static void updateColorsModeMer(IfsUpdateData*);
static void updateColorsModeMerver(IfsUpdateData*);
static void updateColorsModeFeu(IfsUpdateData*);

static void updatePixelBuffers(IfsData* fx_data,
                               PluginInfo* goomInfo,
                               Pixel* prevBuff,
                               Pixel* currentBuff,
                               const size_t numPoints,
                               const IFSPoint* points,
                               const int increment,
                               const Pixel& color)
{
  const float tStep = numPoints == 1 ? 0.0F : (1.0F - 0.0F) / static_cast<float>(numPoints - 1);
  float t = -tStep;
  bool doneColorChange =
      fx_data->colorizer.getColorMode() != Colorizer::ColorMode::megaMapColorChange &&
      fx_data->colorizer.getColorMode() != Colorizer::ColorMode::megaMixColorChange;
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
      changeColormaps(fx_data);
      doneColorChange = true;
    }

    fx_data->drawPixel(prevBuff, currentBuff, x, y, color, t);
  }
}

static void updateIfs(IfsData* fx_data, PluginInfo* goomInfo, Pixel* prevBuff, Pixel* currentBuff)
{
  // TODO: trouver meilleur soluce pour increment (mettre le code de gestion de l'ifs dans ce fichier)
  //       find the best solution for increment (put the management code of the ifs in this file)
  const int increment = goomInfo->update.ifs_incr;
  fx_data->useOldStyleDrawPixel = probabilityOfMInN(1, 4);

  logDebug("increment = {}", increment);

  updData.cycle++;
  if (updData.cycle >= 80)
  {
    updData.cycle = 0;
  }

  size_t numPoints;
  const IFSPoint* points = drawIfs(&numPoints, fx_data);
  numPoints--;

  const int cycle10 = (updData.cycle < 40) ? updData.cycle / 10 : 7 - updData.cycle / 10;
  const Pixel color = getRightShiftedChannels(updData.couleur, cycle10);

  updatePixelBuffers(fx_data, goomInfo, prevBuff, currentBuff, numPoints, points, increment, color);

  updData.justChanged--;

  updData.col = getChannelArray(updData.couleur);

  updateColors(&updData);

  updData.couleur = getPixel(updData.col);

  logDebug("updData.col[ALPHA] = {}", updData.col[ALPHA]);
  logDebug("updData.col[BLEU] = {}", updData.col[BLEU]);
  logDebug("updData.col[VERT] = {}", updData.col[VERT]);
  logDebug("updData.col[ROUGE] = {}", updData.col[ROUGE]);

  logDebug("updData.v[ALPHA] = {}", updData.v[ALPHA]);
  logDebug("updData.v[BLEU] = {}", updData.v[BLEU]);
  logDebug("updData.v[VERT] = {}", updData.v[VERT]);
  logDebug("updData.v[ROUGE] = {}", updData.v[ROUGE]);

  logDebug("updData.mode = {}", updData.mode);
}

static void updateColors(IfsUpdateData* upd)
{
  if (upd->mode == MOD_MER)
  {
    updateColorsModeMer(upd);
  }
  else if (upd->mode == MOD_MERVER)
  {
    updateColorsModeMerver(upd);
  }
  else if (upd->mode == MOD_FEU)
  {
    updateColorsModeFeu(upd);
  }
}

static void updateColorsModeMer(IfsUpdateData* upd)
{
  upd->col[BLEU] += upd->v[BLEU];
  if (upd->col[BLEU] > channel_limits<int32_t>::max())
  {
    upd->col[BLEU] = channel_limits<int32_t>::max();
    upd->v[BLEU] = -getRandInRange(1, 5);
  }
  if (upd->col[BLEU] < 32)
  {
    upd->col[BLEU] = 32;
    upd->v[BLEU] = getRandInRange(1, 5);
  }

  upd->col[VERT] += upd->v[VERT];
  if (upd->col[VERT] > 200)
  {
    upd->col[VERT] = 200;
    upd->v[VERT] = -getRandInRange(2, 5);
  }
  if (upd->col[VERT] > upd->col[BLEU])
  {
    upd->col[VERT] = upd->col[BLEU];
    upd->v[VERT] = upd->v[BLEU];
  }
  if (upd->col[VERT] < 32)
  {
    upd->col[VERT] = 32;
    upd->v[VERT] = getRandInRange(2, 5);
  }

  upd->col[ROUGE] += upd->v[ROUGE];
  if (upd->col[ROUGE] > 64)
  {
    upd->col[ROUGE] = 64;
    upd->v[ROUGE] = -getRandInRange(1, 5);
  }
  if (upd->col[ROUGE] < 0)
  {
    upd->col[ROUGE] = 0;
    upd->v[ROUGE] = getRandInRange(1, 5);
  }

  upd->col[ALPHA] += upd->v[ALPHA];
  if (upd->col[ALPHA] > 0)
  {
    upd->col[ALPHA] = 0;
    upd->v[ALPHA] = -getRandInRange(1, 5);
  }
  if (upd->col[ALPHA] < 0)
  {
    upd->col[ALPHA] = 0;
    upd->v[ALPHA] = getRandInRange(1, 5);
  }

  if (((upd->col[VERT] > 32) && (upd->col[ROUGE] < upd->col[VERT] + 40) &&
       (upd->col[VERT] < upd->col[ROUGE] + 20) && (upd->col[BLEU] < 64) &&
       probabilityOfMInN(1, 20)) &&
      (upd->justChanged < 0))
  {
    upd->mode = probabilityOfMInN(2, 3) ? MOD_FEU : MOD_MERVER;
    upd->justChanged = 250;
  }
}

static void updateColorsModeMerver(IfsUpdateData* upd)
{
  upd->col[BLEU] += upd->v[BLEU];
  if (upd->col[BLEU] > 128)
  {
    upd->col[BLEU] = 128;
    upd->v[BLEU] = -getRandInRange(1, 5);
  }
  if (upd->col[BLEU] < 16)
  {
    upd->col[BLEU] = 16;
    upd->v[BLEU] = getRandInRange(1, 5);
  }

  upd->col[VERT] += upd->v[VERT];
  if (upd->col[VERT] > 200)
  {
    upd->col[VERT] = 200;
    upd->v[VERT] = -getRandInRange(2, 5);
  }
  if (upd->col[VERT] > upd->col[ALPHA])
  {
    upd->col[VERT] = upd->col[ALPHA];
    upd->v[VERT] = upd->v[ALPHA];
  }
  if (upd->col[VERT] < 32)
  {
    upd->col[VERT] = 32;
    upd->v[VERT] = getRandInRange(2, 5);
  }

  upd->col[ROUGE] += upd->v[ROUGE];
  if (upd->col[ROUGE] > 128)
  {
    upd->col[ROUGE] = 128;
    upd->v[ROUGE] = -getRandInRange(1, 5);
  }
  if (upd->col[ROUGE] < 0)
  {
    upd->col[ROUGE] = 0;
    upd->v[ROUGE] = getRandInRange(1, 5);
  }

  upd->col[ALPHA] += upd->v[ALPHA];
  if (upd->col[ALPHA] > channel_limits<int32_t>::max())
  {
    upd->col[ALPHA] = channel_limits<int32_t>::max();
    upd->v[ALPHA] = -getRandInRange(1, 5);
  }
  if (upd->col[ALPHA] < 0)
  {
    upd->col[ALPHA] = 0;
    upd->v[ALPHA] = getRandInRange(1, 5);
  }

  if (((upd->col[VERT] > 32) && (upd->col[ROUGE] < upd->col[VERT] + 40) &&
       (upd->col[VERT] < upd->col[ROUGE] + 20) && (upd->col[BLEU] < 64) &&
       probabilityOfMInN(1, 20)) &&
      (upd->justChanged < 0))
  {
    upd->mode = probabilityOfMInN(2, 3) ? MOD_FEU : MOD_MER;
    upd->justChanged = 250;
  }
}

static void updateColorsModeFeu(IfsUpdateData* upd)
{
  upd->col[BLEU] += upd->v[BLEU];
  if (upd->col[BLEU] > 64)
  {
    upd->col[BLEU] = 64;
    upd->v[BLEU] = -getRandInRange(1, 5);
  }
  if (upd->col[BLEU] < 0)
  {
    upd->col[BLEU] = 0;
    upd->v[BLEU] = getRandInRange(1, 5);
  }

  upd->col[VERT] += upd->v[VERT];
  if (upd->col[VERT] > 200)
  {
    upd->col[VERT] = 200;
    upd->v[VERT] = -getRandInRange(2, 5);
  }
  if (upd->col[VERT] > upd->col[ROUGE] + 20)
  {
    upd->col[VERT] = upd->col[ROUGE] + 20;
    upd->v[VERT] = -getRandInRange(2, 5);
    upd->v[ROUGE] = getRandInRange(1, 5);
    upd->v[BLEU] = getRandInRange(1, 5);
  }
  if (upd->col[VERT] < 0)
  {
    upd->col[VERT] = 0;
    upd->v[VERT] = getRandInRange(2, 5);
  }

  upd->col[ROUGE] += upd->v[ROUGE];
  if (upd->col[ROUGE] > channel_limits<int32_t>::max())
  {
    upd->col[ROUGE] = channel_limits<int32_t>::max();
    upd->v[ROUGE] = -getRandInRange(1, 5);
  }
  if (upd->col[ROUGE] > upd->col[VERT] + 40)
  {
    upd->col[ROUGE] = upd->col[VERT] + 40;
    upd->v[ROUGE] = -getRandInRange(1, 5);
  }
  if (upd->col[ROUGE] < 0)
  {
    upd->col[ROUGE] = 0;
    upd->v[ROUGE] = getRandInRange(1, 5);
  }

  upd->col[ALPHA] += upd->v[ALPHA];
  if (upd->col[ALPHA] > 0)
  {
    upd->col[ALPHA] = 0;
    upd->v[ALPHA] = -getRandInRange(1, 5);
  }
  if (upd->col[ALPHA] < 0)
  {
    upd->col[ALPHA] = 0;
    upd->v[ALPHA] = getRandInRange(1, 5);
  }

  if (((upd->col[ROUGE] < 64) && (upd->col[VERT] > 32) && (upd->col[VERT] < upd->col[BLEU]) &&
       (upd->col[BLEU] > 32) && probabilityOfMInN(1, 20)) &&
      (upd->justChanged < 0))
  {
    upd->mode = probabilityOfMInN(1, 2) ? MOD_MER : MOD_MERVER;
    upd->justChanged = 250;
  }
}

static void ifs_vfx_apply(VisualFX* _this,
                          PluginInfo* goomInfo,
                          Pixel* prevBuff,
                          Pixel* currentBuff)
{
  IfsData* data = static_cast<IfsData*>(_this->fx_data);
  if (!data->initialized)
  {
    data->initialized = true;
    initIfs(goomInfo, data);
  }
  if (!BVAL(data->enabled_bp))
  {
    return;
  }
  updateIfs(data, goomInfo, prevBuff, currentBuff);
}

static const char* const vfxname = "Ifs";

static void ifs_vfx_save(VisualFX*, const PluginInfo*, const char*)
{
}

static void ifs_vfx_restore(VisualFX*, PluginInfo*, const char*)
{
}

static void ifs_vfx_init(VisualFX* _this, PluginInfo* goomInfo)
{
  IfsData* data = new IfsData{goomInfo->screen.width, goomInfo->screen.height};

  data->enabled_bp = secure_b_param("Enabled", 1);
  data->params = plugin_parameters("Ifs", 1);
  data->params.params[0] = &data->enabled_bp;

  data->root = nullptr;
  data->initialized = false;

  _this->fx_data = data;
  _this->params = &data->params;

  initIfs(goomInfo, data);
  data->initialized = true;
}

static void ifs_vfx_free(VisualFX* _this)
{
  std::ofstream f("/tmp/ifs.json");
  saveState(_this, f);
  f << std::endl;
  f.close();

  IfsData* data = static_cast<IfsData*>(_this->fx_data);

  releaseIfs(data);

  delete data;
}

static void ifs_vfx_setBuffSettings(VisualFX* _this, const FXBuffSettings& settings)
{
  IfsData* data = static_cast<IfsData*>(_this->fx_data);
  data->buffSettings = settings;
}

VisualFX ifs_visualfx_create(void)
{
  VisualFX fx;
  fx.init = ifs_vfx_init;
  fx.free = ifs_vfx_free;

  fx.setBuffSettings = ifs_vfx_setBuffSettings;
  fx.getFxName = getFxName;
  fx.saveState = saveState;
  fx.loadState = loadState;
  fx.apply = ifs_vfx_apply;

  fx.save = ifs_vfx_save;
  fx.restore = ifs_vfx_restore;

  return fx;
}

void ifsRenew(VisualFX* _this, const SoundInfo& soundInfo)
{
  IfsData* data = static_cast<IfsData*>(_this->fx_data);

  changeColormaps(data);
  data->colorizer.changeColorMode();
  data->updateAllowOverexposed();

  data->root->speed =
      static_cast<uint32_t>(getRandInRange(1.11F, 5.1F) / (1.1F - soundInfo.getAcceleration()));
}

} // namespace goom
