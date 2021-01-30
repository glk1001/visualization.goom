#ifndef VISUALIZATION_GOOM_CONVOLVE_FX_H
#define VISUALIZATION_GOOM_CONVOLVE_FX_H

#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <memory>
#include <string>

namespace GOOM
{

namespace UTILS
{
class Parallel;
} // namespace UTILS

class PluginInfo;
class PixelBuffer;

class ConvolveFx : public IVisualFx
{
public:
  ConvolveFx() noexcept;
  ConvolveFx(UTILS::Parallel&, const std::shared_ptr<const PluginInfo>&) noexcept;
  ~ConvolveFx() noexcept override;
  ConvolveFx(const ConvolveFx&) noexcept = delete;
  ConvolveFx(ConvolveFx&&) noexcept = delete;
  auto operator=(const ConvolveFx&) -> ConvolveFx& = delete;
  auto operator=(ConvolveFx&&) -> ConvolveFx& = delete;

  void Convolve(const PixelBuffer& currentBuff, PixelBuffer& outputBuff);

  [[nodiscard]] auto GetFxName() const -> std::string override;
  void SetBuffSettings(const FXBuffSettings& settings) override;

  void Start() override;

  void Log(const StatsLogValueFunc& l) const override;
  void Finish() override;

  auto operator==(const ConvolveFx&) const -> bool;

private:
  bool m_enabled = true;
  class ConvolveImpl;
  std::unique_ptr<ConvolveImpl> m_fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive& ar);
};

} // namespace GOOM
#endif
