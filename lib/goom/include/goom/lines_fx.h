#ifndef _LINES_FX_H
#define _LINES_FX_H

/*
 *  lines.h
 *  Goom
 *  Copyright (c) 2000-2003 iOS-software. All rights reserved.
 */

#include "goom_config.h"
#include "goom_graphic.h"

#include <cstddef>
#include <istream>
#include <memory>
#include <ostream>
#include <string>

namespace goom
{

class PluginInfo;

Pixel getBlackLineColor();
Pixel getGreenLineColor();
Pixel getRedLineColor();

class GoomLine
{
public:
  enum class LineType
  {
    circle = 0, // (param = radius)
    hline, // (param = y)
    vline, // (param = x)
    _size // must be last - gives number of enums
  };
  static constexpr size_t numLineTypes = static_cast<size_t>(LineType::_size);

  // construit un effet de line (une ligne horitontale pour commencer)
  GoomLine(const PluginInfo* goomInfo,
           const LineType srceID,
           const float srceParam,
           const Pixel& srceColor,
           const LineType destID,
           const float destParam,
           const Pixel& destColor);
  ~GoomLine() noexcept;
  GoomLine(const GoomLine&) = delete;
  GoomLine& operator=(const GoomLine&) = delete;

  Pixel getRandomLineColor();

  float getPower() const;
  void setPower(const float val);

  void switchGoomLines(const LineType newDestID,
                       const float newParam,
                       const float newAmplitude,
                       const Pixel& newColor);

  void drawGoomLines(const int16_t data[AUDIO_SAMPLE_LEN], Pixel* prevBuff, Pixel* currentBuff);

  std::string getFxName() const;
  void saveState(std::ostream&) const;
  void loadState(std::istream&);

private:
  bool enabled = true;
  class GoomLineImp;
  std::unique_ptr<GoomLineImp> lineImp;
};

} // namespace goom
#endif
