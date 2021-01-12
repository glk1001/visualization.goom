#ifndef VISUALIZATION_GOOM_FLYING_STARS_FX_H
#define VISUALIZATION_GOOM_FLYING_STARS_FX_H

#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <memory>
#include <string>

namespace goom
{

class PluginInfo;
class PixelBuffer;

class FlyingStarsFx : public IVisualFx
{
public:
  enum class ColorMode
  {
    _null = -1,
    mixColors,
    reverseMixColors,
    similarLowColors,
    sineMixColors,
  };

  FlyingStarsFx() noexcept;
  explicit FlyingStarsFx(const std::shared_ptr<const PluginInfo>&) noexcept;
  ~FlyingStarsFx() noexcept override;
  FlyingStarsFx(const FlyingStarsFx&) = delete;
  FlyingStarsFx(const FlyingStarsFx&&) = delete;
  auto operator=(const FlyingStarsFx&) -> FlyingStarsFx& = delete;
  auto operator=(const FlyingStarsFx&&) -> FlyingStarsFx& = delete;

  [[nodiscard]] auto getFxName() const -> std::string override;
  void setBuffSettings(const FXBuffSettings& settings) override;

  void start() override;

  void apply(PixelBuffer&) override;
  void apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff) override;

  void log(const StatsLogValueFunc&) const override;
  void finish() override;

  auto operator==(const FlyingStarsFx& f) const -> bool;

private:
  bool m_enabled = true;
  class FlyingStarsImpl;
  std::unique_ptr<FlyingStarsImpl> m_fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace goom
#endif
