#ifndef LIBS_GOOM_INCLUDE_GOOM_GOOM_DOTS_FX_H_
#define LIBS_GOOM_INCLUDE_GOOM_GOOM_DOTS_FX_H_

#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <memory>
#include <string>

namespace goom
{

class PluginInfo;

class GoomDotsFx : public VisualFx
{
public:
  GoomDotsFx() noexcept;
  explicit GoomDotsFx(const std::shared_ptr<const PluginInfo>&) noexcept;
  ~GoomDotsFx() noexcept override;
  GoomDotsFx(const GoomDotsFx&) = delete;
  GoomDotsFx& operator=(const GoomDotsFx&) = delete;

  [[nodiscard]] std::string getFxName() const override;
  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void apply(PixelBuffer& prevBuff, PixelBuffer& currentBuff) override;

  void finish() override;

  bool operator==(const GoomDotsFx&) const;

private:
  bool enabled = true;
  class GoomDotsImpl;
  std::unique_ptr<GoomDotsImpl> fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace goom

#endif /* LIBS_GOOM_INCLUDE_GOOM_GOOM_DOTS_FX_H_ */
