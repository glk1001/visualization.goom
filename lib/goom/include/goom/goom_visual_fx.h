#ifndef _VISUAL_FX_H
#define _VISUAL_FX_H

#include "goom_config.h"

#include <cstdint>
#include <string>

namespace GOOM
{

class PixelBuffer;

struct FXBuffSettings
{
  float buffIntensity = 0.5;
  bool allowOverexposed = true;
};

class IVisualFx
{
public:
  IVisualFx() = default;
  virtual ~IVisualFx() = default;
  IVisualFx(const IVisualFx&) noexcept = delete;
  IVisualFx(IVisualFx&&) noexcept = delete;
  auto operator=(const IVisualFx&) -> IVisualFx& = delete;
  auto operator=(IVisualFx&&) -> IVisualFx& = delete;

  [[nodiscard]] virtual auto GetResourcesDirectory() const -> const std::string& = 0;
  virtual void SetResourcesDirectory(const std::string& dirName) = 0;

  [[nodiscard]] virtual auto GetFxName() const -> std::string = 0;
  virtual void SetBuffSettings(const FXBuffSettings& settings) = 0;

  virtual void Start() = 0;

  virtual void Log([[maybe_unused]] const StatsLogValueFunc& l) const = 0;
  virtual void Finish() = 0;
};

} // namespace GOOM
#endif
