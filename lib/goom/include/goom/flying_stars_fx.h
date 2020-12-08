#ifndef _FLYING_STARS_FX_H
#define _FLYING_STARS_FX_H

#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <memory>
#include <string>

namespace goom
{

class PluginInfo;

class FlyingStarsFx : public VisualFx
{
public:
  FlyingStarsFx() noexcept;
  explicit FlyingStarsFx(const std::shared_ptr<const PluginInfo>&) noexcept;
  ~FlyingStarsFx() noexcept override;
  FlyingStarsFx(const FlyingStarsFx&) = delete;
  FlyingStarsFx& operator=(const FlyingStarsFx&) = delete;

  [[nodiscard]] std::string getFxName() const override;
  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void apply(PixelBuffer& prevBuff, PixelBuffer& currentBuff) override;

  void log(const StatsLogValueFunc&) const override;
  void finish() override;

  bool operator==(const FlyingStarsFx&) const;

private:
  bool enabled = true;
  class FlyingStarsImpl;
  std::unique_ptr<FlyingStarsImpl> fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace goom
#endif
