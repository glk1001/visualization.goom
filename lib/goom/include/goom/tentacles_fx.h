#ifndef _TENTACLES_FX_H
#define _TENTACLES_FX_H

#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <istream>
#include <memory>
#include <ostream>
#include <string>

namespace goom
{

class PluginInfo;

class TentaclesFx : public VisualFx
{
public:
  TentaclesFx() noexcept = delete;
  explicit TentaclesFx(const PluginInfo*);
  ~TentaclesFx() noexcept = default;
  TentaclesFx(const TentaclesFx&) = delete;
  TentaclesFx& operator=(const TentaclesFx&) = delete;

  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void applyNoDraw() override;
  void apply(Pixel* prevBuff, Pixel* currentBuff) override;

  std::string getFxName() const override;
  void saveState(std::ostream&) const override;
  void loadState(std::istream&) override;

  void log(const StatsLogValueFunc&) const override;
  void finish() override;

private:
  bool enabled = true;
  class TentaclesImpl;
  std::unique_ptr<TentaclesImpl> fxImpl;
};

} // namespace goom
#endif
