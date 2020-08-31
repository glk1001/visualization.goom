#ifndef _LINES_H
#define _LINES_H

/*
 *  lines.h
 *  Goom
 *  Copyright (c) 2000-2003 iOS-software. All rights reserved.
 */

#include "goom_config.h"
#include "goom_graphic.h"
#include "goomutils/colormap.h"

#include <cstddef>
#include <cstdint>

struct GMUnitPointer
{
  float x;
  float y;
  float angle;
};

struct PluginInfo;

// tableau de points
struct GMLine
{
  ColorMaps* colorMaps;

  GMUnitPointer* points;
  GMUnitPointer* points2;
  int IDdest;
  float param;
  float amplitudeF;
  float amplitude;

  size_t nbPoints;
  // pour l'instant je stocke la couleur a terme, on stockera le mode couleur et l'on animera
  uint32_t color;
  uint32_t color2;

  uint16_t screenX;
  uint16_t screenY;

  float power;
  float powinc;

  PluginInfo* goomInfo;
};

// les ID possibles...

#define GML_CIRCLE 0
// (param = radius)

#define GML_HLINE 1
// (param = y)

#define GML_VLINE 2
// (param = x)

// les modes couleur possible (si tu mets un autre c'est noir)
#define GML_BLEUBLANC 0
#define GML_RED 1
#define GML_ORANGE_V 2
#define GML_ORANGE_J 3
#define GML_VERT 4
#define GML_BLEU 5
#define GML_BLACK 6

// construit un effet de line (une ligne horitontale pour commencer)
GMLine* goom_lines_init(PluginInfo* goomInfo,
                        const uint32_t rx,
                        const uint32_t ry,
                        const int IDsrc,
                        const float paramS,
                        const int modeCoulSrc,
                        const int IDdest,
                        const float paramD,
                        const int modeCoulDest);

void goom_lines_switch_to(
    GMLine* gml, const int IDdest, const float param, const float amplitude, const int modeCoul);

void goom_lines_set_res(GMLine* gml, const uint32_t rx, const uint32_t ry);

void goom_lines_free(GMLine** gml);

void goom_lines_draw(PluginInfo* goomInfo,
                     GMLine* line,
                     const int16_t data[AUDIO_SAMPLE_LEN],
                     Pixel* p);

#endif
