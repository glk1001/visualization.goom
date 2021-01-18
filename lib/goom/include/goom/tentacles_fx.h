#ifndef VISUALIZATION_GOOM_TENTACLES_FX_H
#define VISUALIZATION_GOOM_TENTACLES_FX_H

#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <memory>
#include <string>

namespace goom
{

class PluginInfo;
class PixelBuffer;

class TentaclesFx : public IVisualFx
{
public:
  TentaclesFx() noexcept;
  explicit TentaclesFx(const std::shared_ptr<const PluginInfo>&) noexcept;
  ~TentaclesFx() noexcept override;
  TentaclesFx(const TentaclesFx&) = delete;
  TentaclesFx(const TentaclesFx&&) = delete;
  auto operator=(const TentaclesFx&) -> TentaclesFx& = delete;
  auto operator=(const TentaclesFx&&) -> TentaclesFx& = delete;

  [[nodiscard]] std::string GetFxName() const override;
  void SetBuffSettings(const FXBuffSettings&) override;

  void FreshStart();

  void Start() override;

  void ApplyNoDraw() override;
  void Apply(PixelBuffer&) override;
  void Apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff) override;

  void Log(const StatsLogValueFunc&) const override;
  void Finish() override;

  auto operator==(const TentaclesFx&) const -> bool;

private:
  bool m_enabled = true;
  class TentaclesImpl;
  std::unique_ptr<TentaclesImpl> m_fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive& ar);
};

} // namespace goom
#endif
