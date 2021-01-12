#ifndef VISUALIZATION_GOOM_GOOM_DOTS_FX_H
#define VISUALIZATION_GOOM_GOOM_DOTS_FX_H

#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <memory>
#include <string>

namespace goom
{

class PluginInfo;
class PixelBuffer;

class GoomDotsFx : public IVisualFx
{
public:
  GoomDotsFx() noexcept;
  explicit GoomDotsFx(const std::shared_ptr<const PluginInfo>&) noexcept;
  ~GoomDotsFx() noexcept override;
  GoomDotsFx(const GoomDotsFx&) = delete;
  GoomDotsFx(const GoomDotsFx&&) = delete;
  auto operator=(const GoomDotsFx&) -> GoomDotsFx& = delete;
  auto operator=(const GoomDotsFx&&) -> GoomDotsFx& = delete;

  [[nodiscard]] auto getFxName() const -> std::string override;
  void setBuffSettings(const FXBuffSettings& settings) override;

  void start() override;

  void apply(PixelBuffer& currentBuff) override;
  void apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff) override;

  void finish() override;

  auto operator==(const GoomDotsFx& d) const -> bool;

private:
  bool enabled = true;
  class GoomDotsImpl;
  std::unique_ptr<GoomDotsImpl> fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace goom

#endif /* VISUALIZATION_GOOM_GOOM_DOTS_FX_H */
