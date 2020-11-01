#ifndef _TENTACLES_FX_H
#define _TENTACLES_FX_H

#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <cereal/access.hpp>
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
  TentaclesFx() noexcept;
  explicit TentaclesFx(const std::shared_ptr<const PluginInfo>&) noexcept;
  ~TentaclesFx() noexcept;
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

  bool operator==(const TentaclesFx&) const;

private:
  bool enabled = true;
  class TentaclesImpl;
  std::unique_ptr<TentaclesImpl> fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace goom
#endif
