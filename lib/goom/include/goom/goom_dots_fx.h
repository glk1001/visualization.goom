#ifndef VISUALIZATION_GOOM_GOOM_DOTS_FX_H
#define VISUALIZATION_GOOM_GOOM_DOTS_FX_H

#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <memory>
#include <string>

namespace GOOM
{

class PluginInfo;
class PixelBuffer;

class GoomDotsFx : public IVisualFx
{
public:
  GoomDotsFx() noexcept;
  explicit GoomDotsFx(const std::shared_ptr<const PluginInfo>&) noexcept;
  ~GoomDotsFx() noexcept override;
  GoomDotsFx(const GoomDotsFx&) noexcept = delete;
  GoomDotsFx(GoomDotsFx&&) noexcept = delete;
  auto operator=(const GoomDotsFx&) -> GoomDotsFx& = delete;
  auto operator=(GoomDotsFx&&) -> GoomDotsFx& = delete;

  [[nodiscard]] auto GetFxName() const -> std::string override;
  void SetBuffSettings(const FXBuffSettings& settings) override;

  void Start() override;

  void Apply(PixelBuffer& currentBuff) override;
  void Apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff) override;

  void Finish() override;

  auto operator==(const GoomDotsFx& d) const -> bool;

private:
  bool m_enabled = true;
  class GoomDotsImpl;
  std::unique_ptr<GoomDotsImpl> m_fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive& ar);
};

} // namespace GOOM

#endif /* VISUALIZATION_GOOM_GOOM_DOTS_FX_H */
