#ifndef _FLYING_STARS_FX_H
#define _FLYING_STARS_FX_H

#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <istream>
#include <memory>
#include <ostream>
#include <string>

namespace goom
{

struct PluginInfo;
struct FlyingStarsData;

class FlyingStarsFx : public VisualFx
{
public:
  FlyingStarsFx() = delete;
  explicit FlyingStarsFx(PluginInfo*);
  ~FlyingStarsFx() noexcept = default;

  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void apply(Pixel* prevBuff, Pixel* currentBuff) override;

  std::string getFxName() const override;
  void saveState(std::ostream&) override;
  void loadState(std::istream&) override;

  void log(const StatsLogValueFunc& logVal) const override;
  void finish() override;

private:
  PluginInfo* const goomInfo;
  bool enabled = true;
  std::unique_ptr<FlyingStarsData> fxData;
  void addABomb(
      const uint32_t mx, const uint32_t my, const float radius, float vage, const float gravity);
  uint32_t getLowColor(const size_t starNum, const float tmix);
  void soundEventOccured();
};

} // namespace goom
#endif
