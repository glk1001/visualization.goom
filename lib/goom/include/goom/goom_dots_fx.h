#ifndef LIBS_GOOM_INCLUDE_GOOM_GOOM_DOTS_FX_H_
#define LIBS_GOOM_INCLUDE_GOOM_GOOM_DOTS_FX_H_

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

class GoomDotsFx : public VisualFx
{
public:
  GoomDotsFx() noexcept;
  explicit GoomDotsFx(const std::shared_ptr<const PluginInfo>&) noexcept;
  ~GoomDotsFx() noexcept;
  GoomDotsFx(const GoomDotsFx&) = delete;
  GoomDotsFx& operator=(const GoomDotsFx&) = delete;

  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void apply(Pixel* prevBuff, Pixel* currentBuff) override;

  std::string getFxName() const override;
  void saveState(std::ostream&) const override;
  void loadState(std::istream&) override;

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
