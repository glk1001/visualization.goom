#ifndef _LINES_FX_H
#define _LINES_FX_H

/*
 *  lines.h
 *  Goom
 *  Copyright (c) 2000-2003 iOS-software. All rights reserved.
 */

#include "goom_config.h"
#include "goom_graphic.h"

#include <cereal/access.hpp>
#include <cstddef>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace goom
{

class PluginInfo;

Pixel getBlackLineColor();
Pixel getGreenLineColor();
Pixel getRedLineColor();

class LinesFx
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

  LinesFx() noexcept;

  // construit un effet de line (une ligne horitontale pour commencer)
  LinesFx(const std::shared_ptr<const PluginInfo>&,
          const LineType srceID,
          const float srceParam,
          const Pixel& srceColor,
          const LineType destID,
          const float destParam,
          const Pixel& destColor) noexcept;
  ~LinesFx() noexcept;
  LinesFx(const LinesFx&) = delete;
  LinesFx& operator=(const LinesFx&) = delete;

  Pixel getRandomLineColor();

  float getPower() const;
  void setPower(const float val);

  void switchGoomLines(const LineType newDestID,
                       const float newParam,
                       const float newAmplitude,
                       const Pixel& newColor);

  void drawGoomLines(const std::vector<int16_t>& soundData, Pixel* prevBuff, Pixel* currentBuff);

  std::string getFxName() const;
  void saveState(std::ostream&) const;
  void loadState(std::istream&);

  bool operator==(const LinesFx&) const;

private:
  bool enabled = true;
  class LinesImpl;
  std::unique_ptr<LinesImpl> fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace goom
#endif
