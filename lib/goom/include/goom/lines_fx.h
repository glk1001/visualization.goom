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
#include <istream>
#include <ostream>
#include <string>

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
  GMLine(const PluginInfo* goomInfo,
         const uint16_t rx,
         const uint16_t ry,
         const LineType IDsrc,
         const float paramS,
         const Pixel& srcColor,
         const LineType IDdest,
         const float paramD,
         const Pixel& destColor);
  GMLine(const GMLine&) = delete;
  GMLine& operator=(const GMLine&) = delete;

  Pixel getRandomLineColor();

  float getPower() const;
  void setPower(const float val);

  void setResolution(const uint32_t rx, const uint32_t ry);

  void switchGoomLines(const LineType dest,
                       const float param,
                       const float amplitude,
                       const Pixel& color);

  void drawGoomLines(const int16_t data[AUDIO_SAMPLE_LEN], Pixel* prevBuff, Pixel* currentBuff);

  std::string getFxName() const;
  void saveState(std::ostream&) const;
  void loadState(std::istream&);

private:
  const PluginInfo* const goomInfo;
  GoomDraw draw;
  utils::ColorMaps colorMaps{};

  uint16_t screenX;
  uint16_t screenY;

  const size_t nbPoints = AUDIO_SAMPLE_LEN;
  GMUnitPointer points[AUDIO_SAMPLE_LEN];
  GMUnitPointer points2[AUDIO_SAMPLE_LEN];

  float power = 0;
  float powinc = 0;

  LineType IDdest;
  float param;
  float amplitudeF = 1;
  float amplitude = 1;

  // pour l'instant je stocke la couleur a terme, on stockera le mode couleur et l'on animera
  Pixel color;
  Pixel color2;

  void goomLinesMove();
  static void generateLine(
      const LineType id, const float param, const uint32_t rx, const uint32_t ry, GMUnitPointer* l);
};

Pixel getBlackLineColor();
Pixel getGreenLineColor();
Pixel getRedLineColor();

} // namespace goom
#endif
