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

namespace GOOM
{

class PluginInfo;

auto GetBlackLineColor() -> Pixel;
auto GetGreenLineColor() -> Pixel;
auto GetRedLineColor() -> Pixel;

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
  static constexpr size_t NUM_LINE_TYPES = static_cast<size_t>(LineType::_size);

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

  auto GetRandomLineColor() -> Pixel;

  [[nodiscard]] auto GetPower() const -> float;
  void SetPower(float val);

  void SwitchLines(LineType newLineType, float newParam, float newAmplitude, const Pixel& newColor);

  void DrawLines(const std::vector<int16_t>& soundData,
                 PixelBuffer& prevBuff,
                 PixelBuffer& currentBuff);

  [[nodiscard]] auto GetFxName() const -> std::string;

  auto operator==(const LinesFx& l) const -> bool;

private:
  bool m_enabled = true;
  class LinesImpl;
  std::unique_ptr<LinesImpl> m_fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace GOOM
#endif
