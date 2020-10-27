#ifndef _TENTACLES_FX_H
#define _TENTACLES_FX_H

#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>
#include <string>

namespace goom
{

struct PluginInfo;
class TentaclesWrapper;

class TentaclesFx : public VisualFx
{
public:
  TentaclesFx() = delete;
  explicit TentaclesFx(PluginInfo*);
  ~TentaclesFx() noexcept = default;
  TentaclesFx(const TentaclesFx&) = delete;
  TentaclesFx& operator=(const TentaclesFx&) = delete;

  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void applyNoDraw() override;
  void apply(Pixel* prevBuff, Pixel* currentBuff) override;

  std::string getFxName() const override;
  void saveState(std::ostream&) override;
  void loadState(std::istream&) override;

  void log(const StatsLogValueFunc& logVal) const override;
  void finish() override;

private:
  PluginInfo* const goomInfo;
  bool enabled = true;
  std::unique_ptr<TentaclesWrapper> tentacles;
};

} // namespace goom
#endif
