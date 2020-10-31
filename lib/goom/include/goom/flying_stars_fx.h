#ifndef _FLYING_STARS_FX_H
#define _FLYING_STARS_FX_H

#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <istream>
#include <memory>
#include <ostream>
#include <string>

namespace goom
{

class PluginInfo;

class FlyingStarsFx : public VisualFx
{
public:
  FlyingStarsFx() noexcept = delete;
  explicit FlyingStarsFx(const PluginInfo*);
  ~FlyingStarsFx() noexcept = default;
  FlyingStarsFx(const FlyingStarsFx&) = delete;
  FlyingStarsFx& operator=(const FlyingStarsFx&) = delete;

  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void apply(Pixel* prevBuff, Pixel* currentBuff) override;

  std::string getFxName() const override;
  void saveState(std::ostream&) const override;
  void loadState(std::istream&) override;

  void log(const StatsLogValueFunc&) const override;
  void finish() override;

private:
  bool enabled = true;
  class FlyingStarsImpl;
  std::unique_ptr<FlyingStarsImpl> fxImpl;
};

} // namespace goom
#endif
