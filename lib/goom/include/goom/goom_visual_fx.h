#ifndef _VISUAL_FX_H
#define _VISUAL_FX_H

/**
 * File created on 2003-05-21 by Jeko.
 * (c)2003, JC Hoelt for iOS-software.
 *
 * LGPL Licence.
 * If you use this file on a visual program,
 *    please make my name being visible on it.
 */

#include "goom_config.h"

#include <cereal/archives/json.hpp>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

namespace goom
{

class PixelBuffer;

struct FXBuffSettings
{
  float buffIntensity = 0.5;
  bool allowOverexposed = true;
  bool operator==(const FXBuffSettings&) const = default;
  template<class Archive>
  void serialize(Archive&);
};

template<class Archive>
void FXBuffSettings::serialize(Archive& ar)
{
  ar(buffIntensity, allowOverexposed);
}

class IVisualFx
{
public:
  IVisualFx() = default;
  virtual ~IVisualFx() = default;
  IVisualFx(const IVisualFx&) = delete;
  IVisualFx(const IVisualFx&&) = delete;
  auto operator=(const IVisualFx&) -> IVisualFx& = delete;
  auto operator=(const IVisualFx&&) -> IVisualFx& = delete;

  [[nodiscard]] virtual std::string getFxName() const = 0;
  virtual void setBuffSettings(const FXBuffSettings&) = 0;

  virtual void start(){};

  virtual void applyNoDraw(){};
  virtual void apply(PixelBuffer& currentBuff) = 0;
  virtual void apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff) = 0;

  virtual void log(const StatsLogValueFunc&) const {};
  virtual void finish(){};
};

} // namespace goom
#endif
