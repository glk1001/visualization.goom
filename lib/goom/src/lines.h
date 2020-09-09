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

// les ID possibles...
enum class LineType
{
  circle = 0, // (param = radius)
  hline, // (param = y)
  vline, // (param = x)
  _size // must be last - gives number of enums
};
constexpr size_t numLineTypes = static_cast<size_t>(LineType::_size);

// tableau de points
struct GMLine
{
  ColorMaps* colorMaps;

  GMUnitPointer* points;
  GMUnitPointer* points2;
  LineType IDdest;
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

// construit un effet de line (une ligne horitontale pour commencer)
GMLine* goomLinesInit(PluginInfo* goomInfo,
                      const uint32_t rx,
                      const uint32_t ry,
                      const LineType IDsrc,
                      const float paramS,
                      const uint32_t srcColor,
                      const LineType IDdest,
                      const float paramD,
                      const uint32_t destColor);

void goomLinesFree(GMLine**);

void goomLinesSetResolution(GMLine*, const uint32_t rx, const uint32_t ry);

void switchGoomLines(
    GMLine*, const LineType dest, const float param, const float amplitude, const uint32_t color);

uint32_t getBlackLineColor();
uint32_t getGreenLineColor();
uint32_t getRedLineColor();
uint32_t getRandomLineColor(PluginInfo*);

void drawGoomLines(PluginInfo*, GMLine*, const int16_t data[AUDIO_SAMPLE_LEN], Pixel*);

#endif
