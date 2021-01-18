#ifndef VISUALIZATION_GOOM_FLYING_STARS_FX_H
#define VISUALIZATION_GOOM_FLYING_STARS_FX_H

#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <memory>
#include <string>

namespace GOOM
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

  [[nodiscard]] auto GetFxName() const -> std::string override;
  void SetBuffSettings(const FXBuffSettings& settings) override;

  void Start() override;

  void Apply(PixelBuffer& currentBuff) override;
  void Apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff) override;

  void Log(const StatsLogValueFunc& l) const override;
  void Finish() override;

  auto operator==(const FlyingStarsFx& f) const -> bool;

private:
  bool m_enabled = true;
  class FlyingStarsImpl;
  std::unique_ptr<FlyingStarsImpl> m_fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace GOOM
#endif
