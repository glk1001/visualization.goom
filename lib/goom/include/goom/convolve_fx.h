#ifndef _CONVOLVE_FX_H
#define _CONVOLVE_FX_H

#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <memory>
#include <string>

namespace goom
{

class PluginInfo;

class ConvolveFx : public VisualFx
{
public:
  ConvolveFx() noexcept;
  explicit ConvolveFx(const std::shared_ptr<const PluginInfo>&) noexcept;
  ~ConvolveFx() noexcept;
  ConvolveFx(const ConvolveFx&) = delete;
  ConvolveFx& operator=(const ConvolveFx&) = delete;

  void convolve(const PixelBuffer& currentBuff, uint32_t* outputBuff);

  std::string getFxName() const override;
  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void apply(PixelBuffer&, PixelBuffer&) override{};

  void log(const StatsLogValueFunc&) const override;
  void finish() override;

  bool operator==(const ConvolveFx&) const;

private:
  bool enabled = true;
  class ConvolveImpl;
  std::unique_ptr<ConvolveImpl> fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace goom
#endif
