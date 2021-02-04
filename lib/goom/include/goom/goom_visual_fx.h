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

namespace GOOM
{

class PixelBuffer;

struct FXBuffSettings
{
  float buffIntensity = 0.5;
  bool allowOverexposed = true;
#if __cplusplus <= 201402L
  auto operator==(const FXBuffSettings&) const -> bool { return false; };
#else
  auto operator==(const FXBuffSettings&) const -> bool = default;
#endif
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
  IVisualFx(const IVisualFx&) noexcept = delete;
  IVisualFx(IVisualFx&&) noexcept = delete;
  auto operator=(const IVisualFx&) -> IVisualFx& = delete;
  auto operator=(IVisualFx&&) -> IVisualFx& = delete;

  [[nodiscard]] virtual auto GetFxName() const -> std::string = 0;
  virtual void SetBuffSettings(const FXBuffSettings& settings) = 0;

  virtual void Start() = 0;

  virtual void Log([[maybe_unused]] const StatsLogValueFunc& l) const = 0;
  virtual void Finish() = 0;
};

} // namespace GOOM
#endif
