#ifndef VISUALIZATION_GOOM_CONVOLVE_FX_H
#define VISUALIZATION_GOOM_CONVOLVE_FX_H

#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <memory>
#include <string>

namespace goom
{

namespace utils
{
class Parallel;
}

class PluginInfo;
class PixelBuffer;

class ConvolveFx : public IVisualFx
{
public:
  ConvolveFx() noexcept;
  explicit ConvolveFx(utils::Parallel&, const std::shared_ptr<const PluginInfo>&) noexcept;
  ~ConvolveFx() noexcept override;
  ConvolveFx(const ConvolveFx&) = delete;
  ConvolveFx(const ConvolveFx&&) = delete;
  auto operator=(const ConvolveFx&) -> ConvolveFx& = delete;
  auto operator=(const ConvolveFx&&) -> ConvolveFx& = delete;

  void Convolve(const PixelBuffer& currentBuff, PixelBuffer& outputBuff);

  [[nodiscard]] std::string getFxName() const override;
  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void apply(PixelBuffer& currentBuff) override;
  void apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff) override;

  void log(const StatsLogValueFunc&) const override;
  void finish() override;

  auto operator==(const ConvolveFx&) const -> bool;

private:
  bool m_enabled = true;
  class ConvolveImpl;
  std::unique_ptr<ConvolveImpl> m_fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace goom
#endif
