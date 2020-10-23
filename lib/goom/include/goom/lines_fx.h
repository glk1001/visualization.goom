#ifndef _LINES_FX_H
#define _LINES_FX_H

/*
 *  lines.h
 *  Goom
 *  Copyright (c) 2000-2003 iOS-software. All rights reserved.
 */

#include "goom_config.h"
#include "goom_draw.h"
#include "goom_graphic.h"
#include "goomutils/colormap.h"

#include <cstddef>
#include <cstdint>

namespace goom
{

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
class GMLine
{
public:
  // construit un effet de line (une ligne horitontale pour commencer)
  GMLine(PluginInfo* goomInfo,
         const uint16_t rx,
         const uint16_t ry,
         const LineType IDsrc,
         const float paramS,
         const uint32_t srcColor,
         const LineType IDdest,
         const float paramD,
         const uint32_t destColor);

  uint32_t getRandomLineColor();

  void setResolution(const uint32_t rx, const uint32_t ry);
  void switchGoomLines(const LineType dest,
                       const float param,
                       const float amplitude,
                       const uint32_t color);
  void drawGoomLines(const int16_t data[AUDIO_SAMPLE_LEN], Pixel* prevBuff, Pixel* currentBuff);

  float power = 0;
  float powinc = 0;

private:
  PluginInfo* goomInfo;
  GoomDraw draw;
  utils::ColorMaps colorMaps{};

  uint16_t screenX;
  uint16_t screenY;

  const size_t nbPoints = AUDIO_SAMPLE_LEN;
  GMUnitPointer points[AUDIO_SAMPLE_LEN];
  GMUnitPointer points2[AUDIO_SAMPLE_LEN];

  LineType IDdest;
  float param;
  float amplitudeF = 1;
  float amplitude = 1;

  // pour l'instant je stocke la couleur a terme, on stockera le mode couleur et l'on animera
  uint32_t color;
  uint32_t color2;

  void goomLinesMove();
  static void generateLine(
      const LineType id, const float param, const uint32_t rx, const uint32_t ry, GMUnitPointer* l);
};

uint32_t getBlackLineColor();
uint32_t getGreenLineColor();
uint32_t getRedLineColor();

} // namespace goom
#endif
