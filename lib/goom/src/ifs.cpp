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

#include "goom.h"
#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_testing.h"
#include "goom_tools.h"
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"

#include <cmath>
#include <cstdint>

struct IFSPoint
{
  int32_t x;
  int32_t y;
};

#define LRAND() ((long)(goom_random(goomInfo->gRandom) & 0x7fffffff))
#define NRAND(n) ((int)(LRAND() % (n)))

#if RAND_MAX < 0x10000
#define MAXRAND (((float)(RAND_MAX < 16) + ((float)RAND_MAX) + 1.0f) / 127.0f)
#else
#define MAXRAND (2147483648.0 / 127.0) /* unsigned 1<<31 / 127.0 (cf goom_tools) as a float */
#endif

/*****************************************************/

typedef float DBL;
typedef int F_PT;
/* typedef float               F_PT; */

/*****************************************************/

constexpr int FIX = 12;
constexpr int unit = 1 << FIX;
#define DBL_To_F_PT(x) (F_PT)((DBL)(unit) * (x))
// Following inline is different to above #define???!!
//static inline F_PT DBL_To_F_PT(const DBL x) { return (F_PT)((DBL)unit * x); }
static inline F_PT div_by_unit(const F_PT x)
{
  return x >> FIX;
}
static inline F_PT div_by_2units(const F_PT x)
{
  return x >> (FIX + 1);
}

constexpr size_t MAX_SIMI = 6;

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
  Similitude components[5 * MAX_SIMI];
  uint32_t depth;
  uint32_t col;
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

  Fractal* root;
  Fractal* curF;

  /* Used by the Trace recursive method */
  IFSPoint* buff;
  int curPt;
  bool initialized;
};

/*****************************************************/

static DBL Gauss_Rand(PluginInfo* goomInfo, DBL c, DBL S, DBL A_mult_1_minus_exp_neg_S)
{
  DBL y = (DBL)LRAND() / MAXRAND;
  y = A_mult_1_minus_exp_neg_S * (1.0 - exp(-y * y * S));
  if (NRAND(2))
  {
    return (c + y);
  }
  return (c - y);
}

static DBL Half_Gauss_Rand(PluginInfo* goomInfo, DBL c, DBL S, DBL A_mult_1_minus_exp_neg_S)
{
  DBL y = (DBL)LRAND() / MAXRAND;
  y = A_mult_1_minus_exp_neg_S * (1.0 - exp(-y * y * S));
  return (c + y);
}

static DBL get_1_minus_exp_neg_S(DBL S)
{
  return 1.0 - exp(-S);
}

static void Random_Simis(PluginInfo* goomInfo, Fractal* fractal, Similitude* cur, int i)
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
    cur->c_x = Gauss_Rand(goomInfo, 0.0, 4.0, c_AS_factor);
    cur->c_y = Gauss_Rand(goomInfo, 0.0, 4.0, c_AS_factor);
    cur->r = Gauss_Rand(goomInfo, fractal->rMean, 3.0, r_AS_factor);
    cur->r2 = Half_Gauss_Rand(goomInfo, 0.0, 2.0, r2_AS_factor);
    cur->A = Gauss_Rand(goomInfo, 0.0, 4.0, A_AS_factor) * (M_PI / 180.0);
    cur->A2 = Gauss_Rand(goomInfo, 0.0, 4.0, A2_AS_factor) * (M_PI / 180.0);
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

static void free_ifs_buffers(Fractal* fractal)
{
  if (fractal->buffer1 != NULL)
  {
    (void)free((void*)fractal->buffer1);
    fractal->buffer1 = (IFSPoint*)NULL;
  }
  if (fractal->buffer2 != NULL)
  {
    (void)free((void*)fractal->buffer2);
    fractal->buffer2 = (IFSPoint*)NULL;
  }
}

static void free_ifs(Fractal* fractal)
{
  free_ifs_buffers(fractal);
}

/***************************************************************/

static void init_ifs(PluginInfo* goomInfo, IfsData* data)
{
  const uint32_t width = goomInfo->screen.width;
  const uint32_t height = goomInfo->screen.height;

  data->enabled_bp = secure_b_param("Enabled", 1);

  if (data->root == NULL)
  {
    data->root = (Fractal*)malloc(sizeof(Fractal));
    if (data->root == NULL)
      return;
    data->root->buffer1 = (IFSPoint*)NULL;
    data->root->buffer2 = (IFSPoint*)NULL;
  }
  Fractal* fractal = data->root;

  free_ifs_buffers(fractal);

  const int num_centres = (NRAND(4)) + 2; /* Number of centers */
  switch (num_centres)
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

  fractal->numSimi = num_centres;
  fractal->maxPt = fractal->numSimi - 1;
  for (uint32_t i = 0; i <= fractal->depth + 2; i++)
  {
    fractal->maxPt *= fractal->numSimi;
  }

  if ((fractal->buffer1 = (IFSPoint*)calloc((size_t)fractal->maxPt, sizeof(IFSPoint))) == NULL)
  {
    free_ifs(fractal);
    return;
  }
  if ((fractal->buffer2 = (IFSPoint*)calloc((size_t)fractal->maxPt, sizeof(IFSPoint))) == NULL)
  {
    free_ifs(fractal);
    return;
  }

  fractal->speed = 6;
  fractal->width = width; /* modif by JeKo */
  fractal->height = height; /* modif by JeKo */
  fractal->curPt = 0;
  fractal->count = 0;
  fractal->lx = (fractal->width - 1) / 2;
  fractal->ly = (fractal->height - 1) / 2;
  fractal->col = pcg32_rand() % (width * height); /* modif by JeKo */

  Random_Simis(goomInfo, fractal, fractal->components, 5 * MAX_SIMI);

  for (size_t i = 0; i < 5 * MAX_SIMI; i++)
  {
    Similitude cur = fractal->components[i];
    logDebug("simi[{}]: c_x = {:.2}, c_y = {:.2}, r = {:.2}, r2 = {:.2}, A = {:.2}, A2 = {:.2}.", i,
             cur.c_x, cur.c_y, cur.r, cur.r2, cur.A, cur.A2);
  }
}

/***************************************************************/

static inline void Transform(Similitude* Simi, F_PT xo, F_PT yo, F_PT* x, F_PT* y)
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

/***************************************************************/

static void Trace(Fractal* F, F_PT xo, F_PT yo, IfsData* data)
{
  Similitude* Cur = data->curF->components;
  //	logDebug("data->Cur_F->numSimi = {}, xo = {}, yo = {}", data->Cur_F->numSimi, xo, yo);
  for (int i = data->curF->numSimi; i; --i, Cur++)
  {
    F_PT x, y;
    Transform(Cur, xo, yo, &x, &y);

    data->buff->x = F->lx + div_by_2units(x * F->lx);
    data->buff->y = F->ly - div_by_2units(y * F->ly);
    data->buff++;

    data->curPt++;

    if (F->depth && ((x - xo) >> 4) && ((y - yo) >> 4))
    {
      F->depth--;
      Trace(F, x, y, data);
      F->depth++;
    }
  }
}

static void Draw_Fractal(IfsData* data)
{
  Fractal* F = data->root;
  int i;
  Similitude* Cur;
  for (Cur = F->components, i = F->numSimi; i; --i, Cur++)
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
  data->curF = F;
  data->buff = F->buffer2;
  int j;
  for (Cur = F->components, i = F->numSimi; i; --i, Cur++)
  {
    const F_PT xo = Cur->Cx;
    const F_PT yo = Cur->Cy;
    logDebug("F->numSimi = {}, xo = {}, yo = {}", F->numSimi, xo, yo);
    Similitude* Simi;
    for (Simi = F->components, j = F->numSimi; j; --j, Simi++)
    {
      if (Simi == Cur)
      {
        continue;
      }
      F_PT x;
      F_PT y;
      Transform(Simi, xo, yo, &x, &y);
      Trace(F, x, y, data);
    }
  }

  /* Erase previous */
  F->curPt = data->curPt;
  data->buff = F->buffer1;
  F->buffer1 = F->buffer2;
  F->buffer2 = data->buff;
}

static IFSPoint* draw_ifs(PluginInfo* goomInfo, int* nbpt, IfsData* data)
{
  if (data->root == NULL)
  {
    return NULL;
  }
  Fractal* F = data->root;
  if (F->buffer1 == NULL)
  {
    return NULL;
  }

  const DBL u = (DBL)(F->count) * (DBL)(F->speed) / 1000.0;
  const DBL uu = u * u;
  const DBL v = 1.0 - u;
  const DBL vv = v * v;
  const DBL u0 = vv * v;
  const DBL u1 = 3.0 * vv * u;
  const DBL u2 = 3.0 * v * uu;
  const DBL u3 = u * uu;

  Similitude* S = F->components;
  Similitude* S1 = S + F->numSimi;
  Similitude* S2 = S1 + F->numSimi;
  Similitude* S3 = S2 + F->numSimi;
  Similitude* S4 = S3 + F->numSimi;

  for (int i = F->numSimi; i; --i, S++, S1++, S2++, S3++, S4++)
  {
    S->c_x = u0 * S1->c_x + u1 * S2->c_x + u2 * S3->c_x + u3 * S4->c_x;
    S->c_y = u0 * S1->c_y + u1 * S2->c_y + u2 * S3->c_y + u3 * S4->c_y;
    S->r = u0 * S1->r + u1 * S2->r + u2 * S3->r + u3 * S4->r;
    S->r2 = u0 * S1->r2 + u1 * S2->r2 + u2 * S3->r2 + u3 * S4->r2;
    S->A = u0 * S1->A + u1 * S2->A + u2 * S3->A + u3 * S4->A;
    S->A2 = u0 * S1->A2 + u1 * S2->A2 + u2 * S3->A2 + u3 * S4->A2;
  }

  Draw_Fractal(data);

  if (F->count >= 1000 / F->speed)
  {
    S = F->components;
    S1 = S + F->numSimi;
    S2 = S1 + F->numSimi;
    S3 = S2 + F->numSimi;
    S4 = S3 + F->numSimi;

    for (int i = F->numSimi; i; --i, S++, S1++, S2++, S3++, S4++)
    {
      S2->c_x = 2.0 * S4->c_x - S3->c_x;
      S2->c_y = 2.0 * S4->c_y - S3->c_y;
      S2->r = 2.0 * S4->r - S3->r;
      S2->r2 = 2.0 * S4->r2 - S3->r2;
      S2->A = 2.0 * S4->A - S3->A;
      S2->A2 = 2.0 * S4->A2 - S3->A2;

      *S1 = *S4;
    }

    Random_Simis(goomInfo, F, F->components + 3 * F->numSimi, F->numSimi);
    Random_Simis(goomInfo, F, F->components + 4 * F->numSimi, F->numSimi);

    F->count = 0;
  }
  else
  {
    F->count++;
  }

  F->col++;
  (*nbpt) = data->curPt;

  return F->buffer2;
}

/***************************************************************/

static void release_ifs(IfsData* data)
{
  if (data->root != NULL)
  {
    free_ifs(data->root);
    (void)free((void*)data->root);
    data->root = (Fractal*)NULL;
  }
}

#define RAND() (int)goom_random(goomInfo->gRandom)
#define MOD_MER 0
#define MOD_FEU 1
#define MOD_MERVER 2

static struct IfsUpdate
{
  int justChanged;
  int couleur;
  int v[4];
  int col[4];
  int mode;
  int cycle;
} ifs_update_data = {0, (int)0xc0c0c0c0, {2, 4, 3, 2}, {2, 4, 3, 2}, MOD_MERVER, 0};

static void ifs_update(
    PluginInfo* goomInfo, Pixel* data, Pixel* back, int increment, IfsData* fx_data)
{
  logDebug("increment = {}", increment);

  const int couleursl = ifs_update_data.couleur;
  const int width = goomInfo->screen.width;
  const int height = goomInfo->screen.height;

  ifs_update_data.cycle++;
  if (ifs_update_data.cycle >= 80)
  {
    ifs_update_data.cycle = 0;
  }

  int cycle10;
  if (ifs_update_data.cycle < 40)
  {
    cycle10 = ifs_update_data.cycle / 10;
  }
  else
  {
    cycle10 = 7 - ifs_update_data.cycle / 10;
  }

  {
    unsigned char* tmp = (unsigned char*)&couleursl;

    for (int i = 0; i < 4; i++)
    {
      *tmp = (*tmp) >> cycle10;
      tmp++;
    }
  }

  int nbpt = 0;
  IFSPoint* points = draw_ifs(goomInfo, &nbpt, fx_data);
  nbpt--;
  logDebug("nbpt = {}", nbpt);

  for (int i = 0; i < nbpt; i += increment)
  {
    const int x = (int)points[i].x & 0x7fffffff;
    const int y = (int)points[i].y & 0x7fffffff;

    if ((x < width) && (y < height))
    {
      const int pos = x + (int)(y * width);
      int tra = 0, i = 0;
      unsigned char* bra = (unsigned char*)&back[pos];
      unsigned char* dra = (unsigned char*)&data[pos];
      unsigned char* cra = (unsigned char*)&couleursl;

      for (; i < 4; i++)
      {
        tra = *cra;
        tra += *bra;
        if (tra > 255)
          tra = 255;
        *dra = tra;
        ++dra;
        ++cra;
        ++bra;
      }
    }
  }

  ifs_update_data.justChanged--;
  logDebug("ifs_update_data.justChanged = {}", ifs_update_data.justChanged);

  ifs_update_data.col[ALPHA] = ifs_update_data.couleur >> (ALPHA * 8) & 0xff;
  ifs_update_data.col[BLEU] = ifs_update_data.couleur >> (BLEU * 8) & 0xff;
  ifs_update_data.col[VERT] = ifs_update_data.couleur >> (VERT * 8) & 0xff;
  ifs_update_data.col[ROUGE] = ifs_update_data.couleur >> (ROUGE * 8) & 0xff;

  logDebug("ifs_update_data.col[ALPHA] = {}", ifs_update_data.col[ALPHA]);
  logDebug("ifs_update_data.col[BLEU] = {}", ifs_update_data.col[BLEU]);
  logDebug("ifs_update_data.col[VERT] = {}", ifs_update_data.col[VERT]);
  logDebug("ifs_update_data.col[ROUGE] = {}", ifs_update_data.col[ROUGE]);

  logDebug("ifs_update_data.v[ALPHA] = {}", ifs_update_data.v[ALPHA]);
  logDebug("ifs_update_data.v[BLEU] = {}", ifs_update_data.v[BLEU]);
  logDebug("ifs_update_data.v[VERT] = {}", ifs_update_data.v[VERT]);
  logDebug("ifs_update_data.v[ROUGE] = {}", ifs_update_data.v[ROUGE]);

  logDebug("ifs_update_data.mode = {}", ifs_update_data.mode);

  if (ifs_update_data.mode == MOD_MER)
  {
    ifs_update_data.col[BLEU] += ifs_update_data.v[BLEU];
    if (ifs_update_data.col[BLEU] > 255)
    {
      ifs_update_data.col[BLEU] = 255;
      ifs_update_data.v[BLEU] = -(RAND() % 4) - 1;
      logDebug("ifs_update_data.v[BLEU] = {}", ifs_update_data.v[BLEU]);
    }
    if (ifs_update_data.col[BLEU] < 32)
    {
      ifs_update_data.col[BLEU] = 32;
      ifs_update_data.v[BLEU] = (RAND() % 4) + 1;
      logDebug("ifs_update_data.v[BLEU] = {}", ifs_update_data.v[BLEU]);
    }

    ifs_update_data.col[VERT] += ifs_update_data.v[VERT];
    if (ifs_update_data.col[VERT] > 200)
    {
      ifs_update_data.col[VERT] = 200;
      ifs_update_data.v[VERT] = -(RAND() % 3) - 2;
      logDebug("ifs_update_data.v[VERT] = {}", ifs_update_data.v[VERT]);
    }
    if (ifs_update_data.col[VERT] > ifs_update_data.col[BLEU])
    {
      ifs_update_data.col[VERT] = ifs_update_data.col[BLEU];
      ifs_update_data.v[VERT] = ifs_update_data.v[BLEU];
    }
    if (ifs_update_data.col[VERT] < 32)
    {
      ifs_update_data.col[VERT] = 32;
      ifs_update_data.v[VERT] = (RAND() % 3) + 2;
      logDebug("ifs_update_data.v[VERT] = {}", ifs_update_data.v[VERT]);
    }

    ifs_update_data.col[ROUGE] += ifs_update_data.v[ROUGE];
    if (ifs_update_data.col[ROUGE] > 64)
    {
      ifs_update_data.col[ROUGE] = 64;
      ifs_update_data.v[ROUGE] = -(RAND() % 4) - 1;
      logDebug("ifs_update_data.v[ROUGE] = {}", ifs_update_data.v[ROUGE]);
    }
    if (ifs_update_data.col[ROUGE] < 0)
    {
      ifs_update_data.col[ROUGE] = 0;
      ifs_update_data.v[ROUGE] = (RAND() % 4) + 1;
      logDebug("ifs_update_data.v[ROUGE] = {}", ifs_update_data.v[ROUGE]);
    }

    ifs_update_data.col[ALPHA] += ifs_update_data.v[ALPHA];
    if (ifs_update_data.col[ALPHA] > 0)
    {
      ifs_update_data.col[ALPHA] = 0;
      ifs_update_data.v[ALPHA] = -(RAND() % 4) - 1;
      logDebug("ifs_update_data.v[ALPHA] = {}", ifs_update_data.v[ALPHA]);
    }
    if (ifs_update_data.col[ALPHA] < 0)
    {
      ifs_update_data.col[ALPHA] = 0;
      ifs_update_data.v[ALPHA] = (RAND() % 4) + 1;
      logDebug("ifs_update_data.v[ALPHA] = {}", ifs_update_data.v[ALPHA]);
    }

    if (((ifs_update_data.col[VERT] > 32) &&
         (ifs_update_data.col[ROUGE] < ifs_update_data.col[VERT] + 40) &&
         (ifs_update_data.col[VERT] < ifs_update_data.col[ROUGE] + 20) &&
         (ifs_update_data.col[BLEU] < 64) && (RAND() % 20 == 0)) &&
        (ifs_update_data.justChanged < 0))
    {
      ifs_update_data.mode = RAND() % 3 ? MOD_FEU : MOD_MERVER;
      logDebug("ifs_update_data.mode = {}", ifs_update_data.mode);
      ifs_update_data.justChanged = 250;
    }
  }
  else if (ifs_update_data.mode == MOD_MERVER)
  {
    ifs_update_data.col[BLEU] += ifs_update_data.v[BLEU];
    if (ifs_update_data.col[BLEU] > 128)
    {
      ifs_update_data.col[BLEU] = 128;
      ifs_update_data.v[BLEU] = -(RAND() % 4) - 1;
      logDebug("ifs_update_data.v[BLEU] = {}", ifs_update_data.v[BLEU]);
    }
    if (ifs_update_data.col[BLEU] < 16)
    {
      ifs_update_data.col[BLEU] = 16;
      ifs_update_data.v[BLEU] = (RAND() % 4) + 1;
      logDebug("ifs_update_data.v[BLEU] = {}", ifs_update_data.v[BLEU]);
    }

    ifs_update_data.col[VERT] += ifs_update_data.v[VERT];
    if (ifs_update_data.col[VERT] > 200)
    {
      ifs_update_data.col[VERT] = 200;
      ifs_update_data.v[VERT] = -(RAND() % 3) - 2;
      logDebug("ifs_update_data.v[VERT] = {}", ifs_update_data.v[VERT]);
    }
    if (ifs_update_data.col[VERT] > ifs_update_data.col[ALPHA])
    {
      ifs_update_data.col[VERT] = ifs_update_data.col[ALPHA];
      ifs_update_data.v[VERT] = ifs_update_data.v[ALPHA];
    }
    if (ifs_update_data.col[VERT] < 32)
    {
      ifs_update_data.col[VERT] = 32;
      ifs_update_data.v[VERT] = (RAND() % 3) + 2;
      logDebug("ifs_update_data.v[VERT] = {}", ifs_update_data.v[VERT]);
    }

    ifs_update_data.col[ROUGE] += ifs_update_data.v[ROUGE];
    if (ifs_update_data.col[ROUGE] > 128)
    {
      ifs_update_data.col[ROUGE] = 128;
      ifs_update_data.v[ROUGE] = -(RAND() % 4) - 1;
      logDebug("ifs_update_data.v[ROUGE] = {}", ifs_update_data.v[ROUGE]);
    }
    if (ifs_update_data.col[ROUGE] < 0)
    {
      ifs_update_data.col[ROUGE] = 0;
      ifs_update_data.v[ROUGE] = (RAND() % 4) + 1;
      logDebug("ifs_update_data.v[ROUGE] = {}", ifs_update_data.v[ROUGE]);
    }

    ifs_update_data.col[ALPHA] += ifs_update_data.v[ALPHA];
    if (ifs_update_data.col[ALPHA] > 255)
    {
      ifs_update_data.col[ALPHA] = 255;
      ifs_update_data.v[ALPHA] = -(RAND() % 4) - 1;
      logDebug("ifs_update_data.v[ALPHA] = {}", ifs_update_data.v[ALPHA]);
    }
    if (ifs_update_data.col[ALPHA] < 0)
    {
      ifs_update_data.col[ALPHA] = 0;
      ifs_update_data.v[ALPHA] = (RAND() % 4) + 1;
      logDebug("ifs_update_data.v[ALPHA] = {}", ifs_update_data.v[ALPHA]);
    }

    if (((ifs_update_data.col[VERT] > 32) &&
         (ifs_update_data.col[ROUGE] < ifs_update_data.col[VERT] + 40) &&
         (ifs_update_data.col[VERT] < ifs_update_data.col[ROUGE] + 20) &&
         (ifs_update_data.col[BLEU] < 64) && (RAND() % 20 == 0)) &&
        (ifs_update_data.justChanged < 0))
    {
      ifs_update_data.mode = RAND() % 3 ? MOD_FEU : MOD_MER;
      logDebug("ifs_update_data.mode = {}", ifs_update_data.mode);
      ifs_update_data.justChanged = 250;
    }
  }
  else if (ifs_update_data.mode == MOD_FEU)
  {
    ifs_update_data.col[BLEU] += ifs_update_data.v[BLEU];
    if (ifs_update_data.col[BLEU] > 64)
    {
      ifs_update_data.col[BLEU] = 64;
      ifs_update_data.v[BLEU] = -(RAND() % 4) - 1;
      logDebug("ifs_update_data.v[BLEU] = {}", ifs_update_data.v[BLEU]);
    }
    if (ifs_update_data.col[BLEU] < 0)
    {
      ifs_update_data.col[BLEU] = 0;
      ifs_update_data.v[BLEU] = (RAND() % 4) + 1;
      logDebug("ifs_update_data.v[BLEU] = {}", ifs_update_data.v[BLEU]);
    }

    ifs_update_data.col[VERT] += ifs_update_data.v[VERT];
    if (ifs_update_data.col[VERT] > 200)
    {
      ifs_update_data.col[VERT] = 200;
      ifs_update_data.v[VERT] = -(RAND() % 3) - 2;
      logDebug("ifs_update_data.v[VERT] = {}", ifs_update_data.v[VERT]);
    }
    if (ifs_update_data.col[VERT] > ifs_update_data.col[ROUGE] + 20)
    {
      ifs_update_data.col[VERT] = ifs_update_data.col[ROUGE] + 20;
      ifs_update_data.v[VERT] = -(RAND() % 3) - 2;
      ifs_update_data.v[ROUGE] = (RAND() % 4) + 1;
      ifs_update_data.v[BLEU] = (RAND() % 4) + 1;
      logDebug("ifs_update_data.v[VERT] = {}", ifs_update_data.v[VERT]);
    }
    if (ifs_update_data.col[VERT] < 0)
    {
      ifs_update_data.col[VERT] = 0;
      ifs_update_data.v[VERT] = (RAND() % 3) + 2;
      logDebug("ifs_update_data.v[VERT] = {}", ifs_update_data.v[VERT]);
    }

    ifs_update_data.col[ROUGE] += ifs_update_data.v[ROUGE];
    if (ifs_update_data.col[ROUGE] > 255)
    {
      ifs_update_data.col[ROUGE] = 255;
      ifs_update_data.v[ROUGE] = -(RAND() % 4) - 1;
      logDebug("ifs_update_data.v[ROUGE] = {}", ifs_update_data.v[ROUGE]);
    }
    if (ifs_update_data.col[ROUGE] > ifs_update_data.col[VERT] + 40)
    {
      ifs_update_data.col[ROUGE] = ifs_update_data.col[VERT] + 40;
      ifs_update_data.v[ROUGE] = -(RAND() % 4) - 1;
      logDebug("ifs_update_data.v[ROUGE] = {}", ifs_update_data.v[ROUGE]);
    }
    if (ifs_update_data.col[ROUGE] < 0)
    {
      ifs_update_data.col[ROUGE] = 0;
      ifs_update_data.v[ROUGE] = (RAND() % 4) + 1;
      logDebug("ifs_update_data.v[ROUGE] = {}", ifs_update_data.v[ROUGE]);
    }

    ifs_update_data.col[ALPHA] += ifs_update_data.v[ALPHA];
    if (ifs_update_data.col[ALPHA] > 0)
    {
      ifs_update_data.col[ALPHA] = 0;
      ifs_update_data.v[ALPHA] = -(RAND() % 4) - 1;
      logDebug("ifs_update_data.v[ALPHA] = {}", ifs_update_data.v[ALPHA]);
    }
    if (ifs_update_data.col[ALPHA] < 0)
    {
      ifs_update_data.col[ALPHA] = 0;
      ifs_update_data.v[ALPHA] = (RAND() % 4) + 1;
      logDebug("ifs_update_data.v[ALPHA] = {}", ifs_update_data.v[ALPHA]);
    }

    if (((ifs_update_data.col[ROUGE] < 64) && (ifs_update_data.col[VERT] > 32) &&
         (ifs_update_data.col[VERT] < ifs_update_data.col[BLEU]) &&
         (ifs_update_data.col[BLEU] > 32) && (RAND() % 20 == 0)) &&
        (ifs_update_data.justChanged < 0))
    {
      ifs_update_data.mode = RAND() % 2 ? MOD_MER : MOD_MERVER;
      logDebug("ifs_update_data.mode = {}", ifs_update_data.mode);
      ifs_update_data.justChanged = 250;
    }
  }

  ifs_update_data.couleur =
      (ifs_update_data.col[ALPHA] << (ALPHA * 8)) | (ifs_update_data.col[BLEU] << (BLEU * 8)) |
      (ifs_update_data.col[VERT] << (VERT * 8)) | (ifs_update_data.col[ROUGE] << (ROUGE * 8));

  logDebug("ifs_update_data.col[ALPHA] = {}", ifs_update_data.col[ALPHA]);
  logDebug("ifs_update_data.col[BLEU] = {}", ifs_update_data.col[BLEU]);
  logDebug("ifs_update_data.col[VERT] = {}", ifs_update_data.col[VERT]);
  logDebug("ifs_update_data.col[ROUGE] = {}", ifs_update_data.col[ROUGE]);

  logDebug("ifs_update_data.v[ALPHA] = {}", ifs_update_data.v[ALPHA]);
  logDebug("ifs_update_data.v[BLEU] = {}", ifs_update_data.v[BLEU]);
  logDebug("ifs_update_data.v[VERT] = {}", ifs_update_data.v[VERT]);
  logDebug("ifs_update_data.v[ROUGE] = {}", ifs_update_data.v[ROUGE]);

  logDebug("ifs_update_data.mode = {}", ifs_update_data.mode);
}

/** VISUAL_FX WRAPPER FOR IFS */

static void ifs_vfx_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* goomInfo)
{
  IfsData* data = (IfsData*)_this->fx_data;
  if (!data->initialized)
  {
    data->initialized = true;
    init_ifs(goomInfo, data);
  }
  if (!BVAL(data->enabled_bp))
  {
    return;
  }
  ifs_update(goomInfo, dest, src, goomInfo->update.ifs_incr, data);

  /*TODO: trouver meilleur soluce pour increment (mettre le code de gestion de l'ifs dans ce fichier: ifs_vfx_apply) */
}

static const char* const vfxname = "Ifs";

static void ifs_vfx_save(VisualFX* _this, const PluginInfo*, const char* file)
{
  FILE* f = fopen(file, "w");

  save_int_setting(f, vfxname, "ifs_update_data.justChanged", ifs_update_data.justChanged);
  save_int_setting(f, vfxname, "ifs_update_data.couleur", ifs_update_data.couleur);
  save_int_setting(f, vfxname, "ifs_update_data.v_0", ifs_update_data.v[0]);
  save_int_setting(f, vfxname, "ifs_update_data.v_1", ifs_update_data.v[1]);
  save_int_setting(f, vfxname, "ifs_update_data.v_2", ifs_update_data.v[2]);
  save_int_setting(f, vfxname, "ifs_update_data.v_3", ifs_update_data.v[3]);
  save_int_setting(f, vfxname, "ifs_update_data.col_0", ifs_update_data.col[0]);
  save_int_setting(f, vfxname, "ifs_update_data.col_1", ifs_update_data.col[1]);
  save_int_setting(f, vfxname, "ifs_update_data.col_2", ifs_update_data.col[2]);
  save_int_setting(f, vfxname, "ifs_update_data.col_3", ifs_update_data.col[3]);
  save_int_setting(f, vfxname, "ifs_update_data.mode", ifs_update_data.mode);
  save_int_setting(f, vfxname, "ifs_update_data.cycle", ifs_update_data.cycle);

  IfsData* data = (IfsData*)_this->fx_data;
  save_int_setting(f, vfxname, "data.Cur_Pt", data->curPt);
  save_int_setting(f, vfxname, "data.initalized", data->initialized);

  Fractal* fractal = data->root;
  save_int_setting(f, vfxname, "fractal.numSimi", fractal->numSimi);
  save_int_setting(f, vfxname, "fractal.depth", int(fractal->depth));
  save_int_setting(f, vfxname, "fractal.col", int(fractal->col));
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
  if (f == NULL)
  {
    exit(EXIT_FAILURE);
  }

  ifs_update_data.justChanged = get_int_setting(f, vfxname, "ifs_update_data.justChanged");
  ifs_update_data.couleur = get_int_setting(f, vfxname, "ifs_update_data.couleur");
  ifs_update_data.v[0] = get_int_setting(f, vfxname, "ifs_update_data.v_0");
  ifs_update_data.v[1] = get_int_setting(f, vfxname, "ifs_update_data.v_1");
  ifs_update_data.v[2] = get_int_setting(f, vfxname, "ifs_update_data.v_2");
  ifs_update_data.v[3] = get_int_setting(f, vfxname, "ifs_update_data.v_3");
  ifs_update_data.col[0] = get_int_setting(f, vfxname, "ifs_update_data.col_0");
  ifs_update_data.col[1] = get_int_setting(f, vfxname, "ifs_update_data.col_1");
  ifs_update_data.col[2] = get_int_setting(f, vfxname, "ifs_update_data.col_2");
  ifs_update_data.col[3] = get_int_setting(f, vfxname, "ifs_update_data.col_3");
  ifs_update_data.mode = get_int_setting(f, vfxname, "ifs_update_data.mode");
  ifs_update_data.cycle = get_int_setting(f, vfxname, "ifs_update_data.cycle");

  IfsData* data = (IfsData*)_this->fx_data;
  data->curPt = get_int_setting(f, vfxname, "data.Cur_Pt");
  data->initialized = get_int_setting(f, vfxname, "data.initalized");
  data->initialized = true;

  Fractal* fractal = data->root;
  fractal->numSimi = get_int_setting(f, vfxname, "fractal.numSimi");
  fractal->depth = uint32_t(get_int_setting(f, vfxname, "fractal.depth"));
  fractal->col = uint32_t(get_int_setting(f, vfxname, "fractal.col"));
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

static void ifs_vfx_init(VisualFX* _this, PluginInfo* info)
{
  IfsData* data = (IfsData*)malloc(sizeof(IfsData));

  data->enabled_bp = secure_b_param("Enabled", 1);
  data->params = plugin_parameters("Ifs", 1);
  data->params.params[0] = &data->enabled_bp;

  data->root = (Fractal*)NULL;
  data->initialized = false;

  _this->fx_data = data;
  _this->params = &data->params;

  init_ifs(info, data);
  data->initialized = true;
}

static void ifs_vfx_free(VisualFX* _this)
{
  IfsData* data = (IfsData*)_this->fx_data;
  release_ifs(data);
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
