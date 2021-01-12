#ifndef VISUALIZATION_GOOM_LINES_FX_H
#define VISUALIZATION_GOOM_LINES_FX_H

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
          LineType srceLineType,
          float srceParam,
          const Pixel& srceColor,
          LineType destLineType,
          float destParam,
          const Pixel& destColor) noexcept;
  ~LinesFx() noexcept;
  LinesFx(const LinesFx&) = delete;
  LinesFx(const LinesFx&&) = delete;
  auto operator=(const LinesFx&) -> LinesFx& = delete;
  auto operator=(const LinesFx&&) -> LinesFx& = delete;

  Pixel getRandomLineColor();

  [[nodiscard]] auto getPower() const -> float;
  void setPower(float val);

  void switchLines(LineType newLineType, float newParam, float newAmplitude, const Pixel& newColor);

  void drawLines(const std::vector<int16_t>& soundData,
                 PixelBuffer& prevBuff,
                 PixelBuffer& currentBuff);

  [[nodiscard]] auto getFxName() const -> std::string;

  auto operator==(const LinesFx& l) const -> bool;

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
