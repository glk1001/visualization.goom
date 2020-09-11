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

#include "ifs.h"

#include "colorutils.h"
#include "goom_config.h"
#include "goom_core.h"
#include "goom_graphic.h"
#include "goom_testing.h"
#include "goom_tools.h"
#include "goomutils/colormap.h"
#include "goomutils/logging_control.h"
// #undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"

#include <array>
#include <cmath>
#include <cstdint>

struct IFSPoint
{
  uint32_t x;
  uint32_t y;
};

inline uint32_t longRand(PluginInfo* goomInfo)
{
  return static_cast<long>(goomInfo->getRand() & 0x7fffffff);
}

inline int nRand(PluginInfo* goomInfo, const size_t n)
{
  return static_cast<int>(longRand(goomInfo) % n);
}

inline int rand(PluginInfo* goomInfo)
{
  return static_cast<int>(goomInfo->getRand());
}

#if RAND_MAX < 0x10000
#define MAXRAND ((static_cast<float>(RAND_MAX < 16) + static_cast<float>(RAND_MAX) + 1.0f) / 127.0f)
#else
#define MAXRAND (2147483648.0 / 127.0) /* unsigned 1<<31 / 127.0 (cf goom_tools) as a float */
#endif

/*****************************************************/

using DBL = float;
using F_PT = int;
/* typedef float               F_PT; */

/*****************************************************/

constexpr int fix = 12;
constexpr int unit = 1 << fix;
#define DBL_To_F_PT(x) (F_PT)((DBL)(unit) * (x))
// Following inline is different to above #define???!!
//static inline F_PT DBL_To_F_PT(const DBL x) { return (F_PT)((DBL)unit * x); }

inline F_PT div_by_unit(const F_PT x)
{
  return x >> fix;
}
inline F_PT div_by_2units(const F_PT x)
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
  DBL c_x;
  DBL c_y;
  DBL r;
  DBL r2;
  DBL A;
  DBL A2;
  F_PT Ct;
  F_PT St;
  F_PT Ct2;
  F_PT St2;
  F_PT Cx;
  F_PT Cy;
  F_PT R;
  F_PT R2;
};

struct Fractal
{
  int numSimi;
  Similitude components[5 * maxSimi];
  uint32_t depth;
  int count;
  int speed;
  uint16_t width;
  uint16_t height;
  uint16_t lx;
  uint16_t ly;
  DBL rMean;
  DBL drMean;
  DBL dr2Mean;
  int curPt;
  int maxPt;

  IFSPoint* buffer1;
  IFSPoint* buffer2;
};

struct IfsData
{
  PluginParam enabled_bp;
  PluginParameters params;

  const ColorMaps* colorMaps;

  Fractal* root;
  Fractal* curF;

  /* Used by the Trace recursive method */
  IFSPoint* buff;
  size_t curPt;
  bool initialized;
};

static DBL gaussRand(PluginInfo* goomInfo,
                     const DBL c,
                     const DBL S,
                     const DBL A_mult_1_minus_exp_neg_S)
{
  DBL y = static_cast<DBL>(longRand(goomInfo)) / MAXRAND;
  y = A_mult_1_minus_exp_neg_S * (1.0 - exp(-y * y * S));
  if (nRand(goomInfo, 2))
  {
    return (c + y);
  }
  return (c - y);
}

static DBL halfGaussRand(PluginInfo* goomInfo,
                         const DBL c,
                         const DBL S,
                         const DBL A_mult_1_minus_exp_neg_S)
{
  DBL y = static_cast<DBL>(longRand(goomInfo)) / MAXRAND;
  y = A_mult_1_minus_exp_neg_S * (1.0 - exp(-y * y * S));
  return (c + y);
}

inline DBL get_1_minus_exp_neg_S(const DBL S)
{
  return 1.0 - exp(-S);
}

static void randomSimis(PluginInfo* goomInfo, Fractal* fractal, Similitude* cur, int i)
{
  static DBL c_AS_factor;
  static DBL r_1_minus_exp_neg_S;
  static DBL r2_1_minus_exp_neg_S;
  static DBL A_AS_factor;
  static DBL A2_AS_factor;

  static bool doneInit = false;
  if (!doneInit)
  {
    c_AS_factor = 0.8 * get_1_minus_exp_neg_S(4.0);
    r_1_minus_exp_neg_S = get_1_minus_exp_neg_S(3.0);
    r2_1_minus_exp_neg_S = get_1_minus_exp_neg_S(2.0);
    A_AS_factor = 360.0 * get_1_minus_exp_neg_S(4.0);
    A2_AS_factor = A_AS_factor;
    doneInit = true;
  }
  const DBL r_AS_factor = fractal->drMean * r_1_minus_exp_neg_S;
  const DBL r2_AS_factor = fractal->dr2Mean * r2_1_minus_exp_neg_S;

  while (i--)
  {
    cur->c_x = gaussRand(goomInfo, 0.0, 4.0, c_AS_factor);
    cur->c_y = gaussRand(goomInfo, 0.0, 4.0, c_AS_factor);
    cur->r = gaussRand(goomInfo, fractal->rMean, 3.0, r_AS_factor);
    cur->r2 = halfGaussRand(goomInfo, 0.0, 2.0, r2_AS_factor);
    cur->A = gaussRand(goomInfo, 0.0, 4.0, A_AS_factor) * (M_PI / 180.0);
    cur->A2 = gaussRand(goomInfo, 0.0, 4.0, A2_AS_factor) * (M_PI / 180.0);
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

static void freeIfsBuffers(Fractal* fractal)
{
  if (fractal->buffer1)
  {
    free(fractal->buffer1);
    fractal->buffer1 = nullptr;
  }
  if (fractal->buffer2)
  {
    free(fractal->buffer2);
    fractal->buffer2 = nullptr;
  }
}

static void freeIfs(Fractal* fractal)
{
  freeIfsBuffers(fractal);
}

static void initIfs(PluginInfo* goomInfo, IfsData* data)
{
  data->enabled_bp = secure_b_param("Enabled", 1);

  if (!data->root)
  {
    data->root = (Fractal*)malloc(sizeof(Fractal));
    if (!data->root)
      return;
    data->root->buffer1 = nullptr;
    data->root->buffer2 = nullptr;
  }

  Fractal* fractal = data->root;
  freeIfsBuffers(fractal);

  const int numCentres = nRand(goomInfo, 4) + 2;
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

  if ((fractal->buffer1 = (IFSPoint*)calloc((size_t)fractal->maxPt, sizeof(IFSPoint))) == nullptr)
  {
    freeIfs(fractal);
    return;
  }
  if ((fractal->buffer2 = (IFSPoint*)calloc((size_t)fractal->maxPt, sizeof(IFSPoint))) == nullptr)
  {
    freeIfs(fractal);
    return;
  }

  fractal->speed = 6;
  fractal->width = goomInfo->screen.width; // modif by JeKo
  fractal->height = goomInfo->screen.height; // modif by JeKo
  fractal->curPt = 0;
  fractal->count = 0;
  fractal->lx = (fractal->width - 1) / 2;
  fractal->ly = (fractal->height - 1) / 2;

  randomSimis(goomInfo, fractal, fractal->components, 5 * maxSimi);

  for (size_t i = 0; i < 5 * maxSimi; i++)
  {
    Similitude cur = fractal->components[i];
    logDebug("simi[{}]: c_x = {:.2}, c_y = {:.2}, r = {:.2}, r2 = {:.2}, A = {:.2}, A2 = {:.2}.", i,
             cur.c_x, cur.c_y, cur.r, cur.r2, cur.A, cur.A2);
  }
}

static inline void transform(Similitude* Simi, F_PT xo, F_PT yo, F_PT* x, F_PT* y)
{
  xo = xo - Simi->Cx;
  xo = div_by_unit(xo * Simi->R);
  yo = yo - Simi->Cy;
  yo = div_by_unit(yo * Simi->R);

  F_PT xx = xo - Simi->Cx;
  xx = div_by_unit(xx * Simi->R2);
  F_PT yy = -yo - Simi->Cy;
  yy = div_by_unit(yy * Simi->R2);

  *x = div_by_unit(xo * Simi->Ct - yo * Simi->St + xx * Simi->Ct2 - yy * Simi->St2) + Simi->Cx;
  *y = div_by_unit(xo * Simi->St + yo * Simi->Ct + xx * Simi->St2 + yy * Simi->Ct2) + Simi->Cy;
}

static void trace(Fractal* F, const F_PT xo, const F_PT yo, IfsData* data)
{
  Similitude* Cur = data->curF->components;
  //	logDebug("data->Cur_F->numSimi = {}, xo = {}, yo = {}", data->Cur_F->numSimi, xo, yo);
  for (int i = data->curF->numSimi; i; --i, Cur++)
  {
    F_PT x, y;
    transform(Cur, xo, yo, &x, &y);

    data->buff->x = static_cast<uint32_t>(F->lx + div_by_2units(x * F->lx));
    data->buff->y = static_cast<uint32_t>(F->ly - div_by_2units(y * F->ly));
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
  Fractal* fractal = data->root;
  int i;
  Similitude* Cur;
  for (Cur = fractal->components, i = fractal->numSimi; i; --i, Cur++)
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
  for (Cur = fractal->components, i = fractal->numSimi; i; --i, Cur++)
  {
    const F_PT xo = Cur->Cx;
    const F_PT yo = Cur->Cy;
    logDebug("F->numSimi = {}, xo = {}, yo = {}", fractal->numSimi, xo, yo);
    Similitude* Simi;
    for (Simi = fractal->components, j = fractal->numSimi; j; --j, Simi++)
    {
      if (Simi == Cur)
      {
        continue;
      }
      F_PT x;
      F_PT y;
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

static IFSPoint* drawIfs(PluginInfo* goomInfo, size_t* numPoints, IfsData* data)
{
  if (data->root == nullptr)
  {
    *numPoints = 0;
    return nullptr;
  }

  Fractal* fractal = data->root;
  if (fractal->buffer1 == nullptr)
  {
    *numPoints = 0;
    return nullptr;
  }

  const DBL u = static_cast<DBL>(fractal->count) * static_cast<DBL>(fractal->speed) / 1000.0;
  const DBL uu = u * u;
  const DBL v = 1.0 - u;
  const DBL vv = v * v;
  const DBL u0 = vv * v;
  const DBL u1 = 3.0 * vv * u;
  const DBL u2 = 3.0 * v * uu;
  const DBL u3 = u * uu;

  Similitude* S = fractal->components;
  Similitude* S1 = S + fractal->numSimi;
  Similitude* S2 = S1 + fractal->numSimi;
  Similitude* S3 = S2 + fractal->numSimi;
  Similitude* S4 = S3 + fractal->numSimi;

  for (int i = fractal->numSimi; i; --i, S++, S1++, S2++, S3++, S4++)
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

    for (int i = fractal->numSimi; i; --i, S++, S1++, S2++, S3++, S4++)
    {
      S2->c_x = 2.0 * S4->c_x - S3->c_x;
      S2->c_y = 2.0 * S4->c_y - S3->c_y;
      S2->r = 2.0 * S4->r - S3->r;
      S2->r2 = 2.0 * S4->r2 - S3->r2;
      S2->A = 2.0 * S4->A - S3->A;
      S2->A2 = 2.0 * S4->A2 - S3->A2;

      *S1 = *S4;
    }

    randomSimis(goomInfo, fractal, fractal->components + 3 * fractal->numSimi, fractal->numSimi);
    randomSimis(goomInfo, fractal, fractal->components + 4 * fractal->numSimi, fractal->numSimi);

    fractal->count = 0;
  }

  *numPoints = data->curPt;

  return fractal->buffer2;
}

static void releaseIfs(IfsData* data)
{
  if (data->root)
  {
    freeIfs(data->root);
    free(data->root);
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

static void updateColors(PluginInfo*, IfsUpdateData*);
static void updateColorsModeMer(PluginInfo*, IfsUpdateData*);
static void updateColorsModeMerver(PluginInfo*, IfsUpdateData*);
static void updateColorsModeFeu(PluginInfo*, IfsUpdateData*);

static void updatePixelBuffers(PluginInfo* goomInfo,
                               Pixel* frontBuff,
                               Pixel* backBuff,
                               const size_t numPoints,
                               const IFSPoint* points,
                               const int increment,
                               const Pixel& color)
{
  const uint32_t width = goomInfo->screen.width;
  const uint32_t height = goomInfo->screen.height;

  for (size_t i = 0; i < numPoints; i += static_cast<size_t>(increment))
  {
    const uint32_t x = points[i].x & 0x7fffffff;
    const uint32_t y = points[i].y & 0x7fffffff;

    if ((x < width) && (y < height))
    {
      const uint32_t pos = x + (y * width);
      Pixel* const p = &frontBuff[pos];
      *p = getColorAdd(backBuff[pos], color);
    }
  }
}

static void updateIfs(
    PluginInfo* goomInfo, Pixel* frontBuff, Pixel* backBuff, int increment, IfsData* fx_data)
{
  logDebug("increment = {}", increment);

  updData.cycle++;
  if (updData.cycle >= 80)
  {
    updData.cycle = 0;
  }

  size_t numPoints;
  const IFSPoint* points = drawIfs(goomInfo, &numPoints, fx_data);
  numPoints--;

  const int cycle10 = (updData.cycle < 40) ? updData.cycle / 10 : 7 - updData.cycle / 10;
  const Pixel color = getRightShiftedChannels(updData.couleur, cycle10);

  updatePixelBuffers(goomInfo, frontBuff, backBuff, numPoints, points, increment, color);

  updData.justChanged--;

  updData.col = getChannelArray(updData.couleur);

  updateColors(goomInfo, &updData);

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

static void updateColors(PluginInfo* goomInfo, IfsUpdateData* upd)
{
  if (upd->mode == MOD_MER)
  {
    updateColorsModeMer(goomInfo, upd);
  }
  else if (upd->mode == MOD_MERVER)
  {
    updateColorsModeMerver(goomInfo, upd);
  }
  else if (upd->mode == MOD_FEU)
  {
    updateColorsModeFeu(goomInfo, upd);
  }
}

static void updateColorsModeMer(PluginInfo* goomInfo, IfsUpdateData* upd)
{
  upd->col[BLEU] += upd->v[BLEU];
  if (upd->col[BLEU] > 255)
  {
    upd->col[BLEU] = 255;
    upd->v[BLEU] = -(rand(goomInfo) % 4) - 1;
  }
  if (upd->col[BLEU] < 32)
  {
    upd->col[BLEU] = 32;
    upd->v[BLEU] = (rand(goomInfo) % 4) + 1;
  }

  upd->col[VERT] += upd->v[VERT];
  if (upd->col[VERT] > 200)
  {
    upd->col[VERT] = 200;
    upd->v[VERT] = -(rand(goomInfo) % 3) - 2;
  }
  if (upd->col[VERT] > upd->col[BLEU])
  {
    upd->col[VERT] = upd->col[BLEU];
    upd->v[VERT] = upd->v[BLEU];
  }
  if (upd->col[VERT] < 32)
  {
    upd->col[VERT] = 32;
    upd->v[VERT] = (rand(goomInfo) % 3) + 2;
  }

  upd->col[ROUGE] += upd->v[ROUGE];
  if (upd->col[ROUGE] > 64)
  {
    upd->col[ROUGE] = 64;
    upd->v[ROUGE] = -(rand(goomInfo) % 4) - 1;
  }
  if (upd->col[ROUGE] < 0)
  {
    upd->col[ROUGE] = 0;
    upd->v[ROUGE] = (rand(goomInfo) % 4) + 1;
  }

  upd->col[ALPHA] += upd->v[ALPHA];
  if (upd->col[ALPHA] > 0)
  {
    upd->col[ALPHA] = 0;
    upd->v[ALPHA] = -(rand(goomInfo) % 4) - 1;
  }
  if (upd->col[ALPHA] < 0)
  {
    upd->col[ALPHA] = 0;
    upd->v[ALPHA] = (rand(goomInfo) % 4) + 1;
  }

  if (((upd->col[VERT] > 32) && (upd->col[ROUGE] < upd->col[VERT] + 40) &&
       (upd->col[VERT] < upd->col[ROUGE] + 20) && (upd->col[BLEU] < 64) &&
       (rand(goomInfo) % 20 == 0)) &&
      (upd->justChanged < 0))
  {
    upd->mode = rand(goomInfo) % 3 ? MOD_FEU : MOD_MERVER;
    upd->justChanged = 250;
  }
}

static void updateColorsModeMerver(PluginInfo* goomInfo, IfsUpdateData* upd)
{
  upd->col[BLEU] += upd->v[BLEU];
  if (upd->col[BLEU] > 128)
  {
    upd->col[BLEU] = 128;
    upd->v[BLEU] = -(rand(goomInfo) % 4) - 1;
  }
  if (upd->col[BLEU] < 16)
  {
    upd->col[BLEU] = 16;
    upd->v[BLEU] = (rand(goomInfo) % 4) + 1;
  }

  upd->col[VERT] += upd->v[VERT];
  if (upd->col[VERT] > 200)
  {
    upd->col[VERT] = 200;
    upd->v[VERT] = -(rand(goomInfo) % 3) - 2;
  }
  if (upd->col[VERT] > upd->col[ALPHA])
  {
    upd->col[VERT] = upd->col[ALPHA];
    upd->v[VERT] = upd->v[ALPHA];
  }
  if (upd->col[VERT] < 32)
  {
    upd->col[VERT] = 32;
    upd->v[VERT] = (rand(goomInfo) % 3) + 2;
  }

  upd->col[ROUGE] += upd->v[ROUGE];
  if (upd->col[ROUGE] > 128)
  {
    upd->col[ROUGE] = 128;
    upd->v[ROUGE] = -(rand(goomInfo) % 4) - 1;
  }
  if (upd->col[ROUGE] < 0)
  {
    upd->col[ROUGE] = 0;
    upd->v[ROUGE] = (rand(goomInfo) % 4) + 1;
  }

  upd->col[ALPHA] += upd->v[ALPHA];
  if (upd->col[ALPHA] > 255)
  {
    upd->col[ALPHA] = 255;
    upd->v[ALPHA] = -(rand(goomInfo) % 4) - 1;
  }
  if (upd->col[ALPHA] < 0)
  {
    upd->col[ALPHA] = 0;
    upd->v[ALPHA] = (rand(goomInfo) % 4) + 1;
  }

  if (((upd->col[VERT] > 32) && (upd->col[ROUGE] < upd->col[VERT] + 40) &&
       (upd->col[VERT] < upd->col[ROUGE] + 20) && (upd->col[BLEU] < 64) &&
       (rand(goomInfo) % 20 == 0)) &&
      (upd->justChanged < 0))
  {
    upd->mode = rand(goomInfo) % 3 ? MOD_FEU : MOD_MER;
    upd->justChanged = 250;
  }
}

static void updateColorsModeFeu(PluginInfo* goomInfo, IfsUpdateData* upd)
{
  upd->col[BLEU] += upd->v[BLEU];
  if (upd->col[BLEU] > 64)
  {
    upd->col[BLEU] = 64;
    upd->v[BLEU] = -(rand(goomInfo) % 4) - 1;
  }
  if (upd->col[BLEU] < 0)
  {
    upd->col[BLEU] = 0;
    upd->v[BLEU] = (rand(goomInfo) % 4) + 1;
  }

  upd->col[VERT] += upd->v[VERT];
  if (upd->col[VERT] > 200)
  {
    upd->col[VERT] = 200;
    upd->v[VERT] = -(rand(goomInfo) % 3) - 2;
  }
  if (upd->col[VERT] > upd->col[ROUGE] + 20)
  {
    upd->col[VERT] = upd->col[ROUGE] + 20;
    upd->v[VERT] = -(rand(goomInfo) % 3) - 2;
    upd->v[ROUGE] = (rand(goomInfo) % 4) + 1;
    upd->v[BLEU] = (rand(goomInfo) % 4) + 1;
  }
  if (upd->col[VERT] < 0)
  {
    upd->col[VERT] = 0;
    upd->v[VERT] = (rand(goomInfo) % 3) + 2;
  }

  upd->col[ROUGE] += upd->v[ROUGE];
  if (upd->col[ROUGE] > 255)
  {
    upd->col[ROUGE] = 255;
    upd->v[ROUGE] = -(rand(goomInfo) % 4) - 1;
  }
  if (upd->col[ROUGE] > upd->col[VERT] + 40)
  {
    upd->col[ROUGE] = upd->col[VERT] + 40;
    upd->v[ROUGE] = -(rand(goomInfo) % 4) - 1;
  }
  if (upd->col[ROUGE] < 0)
  {
    upd->col[ROUGE] = 0;
    upd->v[ROUGE] = (rand(goomInfo) % 4) + 1;
  }

  upd->col[ALPHA] += upd->v[ALPHA];
  if (upd->col[ALPHA] > 0)
  {
    upd->col[ALPHA] = 0;
    upd->v[ALPHA] = -(rand(goomInfo) % 4) - 1;
  }
  if (upd->col[ALPHA] < 0)
  {
    upd->col[ALPHA] = 0;
    upd->v[ALPHA] = (rand(goomInfo) % 4) + 1;
  }

  if (((upd->col[ROUGE] < 64) && (upd->col[VERT] > 32) && (upd->col[VERT] < upd->col[BLEU]) &&
       (upd->col[BLEU] > 32) && (rand(goomInfo) % 20 == 0)) &&
      (upd->justChanged < 0))
  {
    upd->mode = rand(goomInfo) % 2 ? MOD_MER : MOD_MERVER;
    upd->justChanged = 250;
  }
}

static void ifs_vfx_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* goomInfo)
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
  updateIfs(goomInfo, dest, src, goomInfo->update.ifs_incr, data);

  /*TODO: trouver meilleur soluce pour increment (mettre le code de gestion de l'ifs dans ce fichier: ifs_vfx_apply) */
}

static const char* const vfxname = "Ifs";

static void ifs_vfx_save(VisualFX* _this, const PluginInfo*, const char* file)
{
  FILE* f = fopen(file, "w");

  save_int_setting(f, vfxname, "updData.justChanged", updData.justChanged);
  save_int_setting(f, vfxname, "updData.couleur", static_cast<int>(updData.couleur.val));
  save_int_setting(f, vfxname, "updData.v_0", updData.v[0]);
  save_int_setting(f, vfxname, "updData.v_1", updData.v[1]);
  save_int_setting(f, vfxname, "updData.v_2", updData.v[2]);
  save_int_setting(f, vfxname, "updData.v_3", updData.v[3]);
  save_int_setting(f, vfxname, "updData.col_0", updData.col[0]);
  save_int_setting(f, vfxname, "updData.col_1", updData.col[1]);
  save_int_setting(f, vfxname, "updData.col_2", updData.col[2]);
  save_int_setting(f, vfxname, "updData.col_3", updData.col[3]);
  save_int_setting(f, vfxname, "updData.mode", updData.mode);
  save_int_setting(f, vfxname, "updData.cycle", updData.cycle);

  IfsData* data = static_cast<IfsData*>(_this->fx_data);
  save_int_setting(f, vfxname, "data.Cur_Pt", data->curPt);
  save_int_setting(f, vfxname, "data.initalized", data->initialized);

  Fractal* fractal = data->root;
  save_int_setting(f, vfxname, "fractal.numSimi", fractal->numSimi);
  save_int_setting(f, vfxname, "fractal.depth", int(fractal->depth));
  save_int_setting(f, vfxname, "fractal.count", fractal->count);
  save_int_setting(f, vfxname, "fractal.speed", fractal->speed);
  save_int_setting(f, vfxname, "fractal.width", fractal->width);
  save_int_setting(f, vfxname, "fractal.height", fractal->height);
  save_int_setting(f, vfxname, "fractal.lx", fractal->lx);
  save_int_setting(f, vfxname, "fractal.ly", fractal->ly);
  save_float_setting(f, vfxname, "fractal.rMean", fractal->rMean);
  save_float_setting(f, vfxname, "fractal.drMean", fractal->drMean);
  save_float_setting(f, vfxname, "fractal.dr2Mean", fractal->dr2Mean);
  save_int_setting(f, vfxname, "fractal.curPt", fractal->curPt);
  save_int_setting(f, vfxname, "fractal.maxPt", fractal->maxPt);

  for (int i = 0; i < fractal->numSimi; i++)
  {
    const Similitude* simi = &(fractal->components[i]);
    save_indexed_float_setting(f, vfxname, "simi.c_x", i, simi->c_x);
    save_indexed_float_setting(f, vfxname, "simi.c_y", i, simi->c_y);
    save_indexed_float_setting(f, vfxname, "simi.r", i, simi->r);
    save_indexed_float_setting(f, vfxname, "simi.r2", i, simi->r2);
    save_indexed_float_setting(f, vfxname, "simi.A", i, simi->A);
    save_indexed_float_setting(f, vfxname, "simi.A2", i, simi->A2);
    save_indexed_int_setting(f, vfxname, "simi.Ct", i, simi->Ct);
    save_indexed_int_setting(f, vfxname, "simi.St", i, simi->St);
    save_indexed_int_setting(f, vfxname, "simi.Ct2", i, simi->Ct2);
    save_indexed_int_setting(f, vfxname, "simi.St2", i, simi->St2);
    save_indexed_int_setting(f, vfxname, "simi.Cx", i, simi->Cx);
    save_indexed_int_setting(f, vfxname, "simi.Cy", i, simi->Cy);
    save_indexed_int_setting(f, vfxname, "simi.R", i, simi->R);
    save_indexed_int_setting(f, vfxname, "simi.R2", i, simi->R2);
  }

  fclose(f);
}

static void ifs_vfx_restore(VisualFX* _this, PluginInfo*, const char* file)
{
  FILE* f = fopen(file, "r");
  if (f == nullptr)
  {
    exit(EXIT_FAILURE);
  }

  updData.justChanged = get_int_setting(f, vfxname, "updData.justChanged");
  updData.couleur.val = static_cast<uint32_t>(get_int_setting(f, vfxname, "updData.couleur"));
  updData.v[0] = get_int_setting(f, vfxname, "updData.v_0");
  updData.v[1] = get_int_setting(f, vfxname, "updData.v_1");
  updData.v[2] = get_int_setting(f, vfxname, "updData.v_2");
  updData.v[3] = get_int_setting(f, vfxname, "updData.v_3");
  updData.col[0] = get_int_setting(f, vfxname, "updData.col_0");
  updData.col[1] = get_int_setting(f, vfxname, "updData.col_1");
  updData.col[2] = get_int_setting(f, vfxname, "updData.col_2");
  updData.col[3] = get_int_setting(f, vfxname, "updData.col_3");
  updData.mode = get_int_setting(f, vfxname, "updData.mode");
  updData.cycle = get_int_setting(f, vfxname, "updData.cycle");

  IfsData* data = static_cast<IfsData*>(_this->fx_data);
  data->curPt = static_cast<size_t>(get_int_setting(f, vfxname, "data.Cur_Pt"));
  data->initialized = get_int_setting(f, vfxname, "data.initalized");
  data->initialized = true;

  Fractal* fractal = data->root;
  fractal->numSimi = get_int_setting(f, vfxname, "fractal.numSimi");
  fractal->depth = uint32_t(get_int_setting(f, vfxname, "fractal.depth"));
  fractal->count = get_int_setting(f, vfxname, "fractal.count");
  fractal->speed = get_int_setting(f, vfxname, "fractal.speed");
  fractal->width = get_int_setting(f, vfxname, "fractal.width");
  fractal->height = get_int_setting(f, vfxname, "fractal.height");
  fractal->lx = get_int_setting(f, vfxname, "fractal.lx");
  fractal->ly = get_int_setting(f, vfxname, "fractal.ly");
  fractal->rMean = get_float_setting(f, vfxname, "fractal.rMean");
  fractal->drMean = get_float_setting(f, vfxname, "fractal.drMean");
  fractal->dr2Mean = get_float_setting(f, vfxname, "fractal.dr2Mean");
  fractal->curPt = get_int_setting(f, vfxname, "fractal.curPt");
  fractal->maxPt = get_int_setting(f, vfxname, "fractal.maxPt");

  for (int i = 0; i < fractal->numSimi; i++)
  {
    Similitude* simi = &(fractal->components[i]);
    simi->c_x = get_indexed_float_setting(f, vfxname, "simi.c_x", i);
    simi->c_y = get_indexed_float_setting(f, vfxname, "simi.c_y", i);
    simi->r = get_indexed_float_setting(f, vfxname, "simi.r", i);
    simi->r2 = get_indexed_float_setting(f, vfxname, "simi.r2", i);
    simi->A = get_indexed_float_setting(f, vfxname, "simi.A", i);
    simi->A2 = get_indexed_float_setting(f, vfxname, "simi.A2", i);
    simi->Ct = get_indexed_int_setting(f, vfxname, "simi.Ct", i);
    simi->St = get_indexed_int_setting(f, vfxname, "simi.St", i);
    simi->Ct2 = get_indexed_int_setting(f, vfxname, "simi.Ct2", i);
    simi->St2 = get_indexed_int_setting(f, vfxname, "simi.St2", i);
    simi->Cx = get_indexed_int_setting(f, vfxname, "simi.Cx", i);
    simi->Cy = get_indexed_int_setting(f, vfxname, "simi.Cy", i);
    simi->R = get_indexed_int_setting(f, vfxname, "simi.R", i);
    simi->R2 = get_indexed_int_setting(f, vfxname, "simi.R2", i);
  }

  fclose(f);

  //    ifs_vfx_save(_this, info, "/tmp/vfx_save_after_restore.txt");
}

static void ifs_vfx_init(VisualFX* _this, PluginInfo* goomInfo)
{
  IfsData* data = (IfsData*)malloc(sizeof(IfsData));

  data->enabled_bp = secure_b_param("Enabled", 1);
  data->params = plugin_parameters("Ifs", 1);
  data->params.params[0] = &data->enabled_bp;

  data->colorMaps = new ColorMaps{};

  data->root = nullptr;
  data->initialized = false;

  _this->fx_data = data;
  _this->params = &data->params;

  initIfs(goomInfo, data);
  data->initialized = true;

  ifsRenew(_this);
}

static void ifs_vfx_free(VisualFX* _this)
{
  IfsData* data = static_cast<IfsData*>(_this->fx_data);
  releaseIfs(data);
  free(data);
}

VisualFX ifs_visualfx_create(void)
{
  VisualFX vfx;
  vfx.init = ifs_vfx_init;
  vfx.free = ifs_vfx_free;
  vfx.apply = ifs_vfx_apply;
  vfx.save = ifs_vfx_save;
  vfx.restore = ifs_vfx_restore;
  return vfx;
}

void ifsRenew(VisualFX* _this)
{
  IfsData* data = static_cast<IfsData*>(_this->fx_data);
  updData.couleur.val = ColorMap::getRandomColor(data->colorMaps->getRandomColorMap());
}
