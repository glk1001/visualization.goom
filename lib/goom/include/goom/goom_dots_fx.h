#ifndef LIBS_GOOM_INCLUDE_GOOM_GOOM_DOTS_FX_H_
#define LIBS_GOOM_INCLUDE_GOOM_GOOM_DOTS_FX_H_

#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <istream>
#include <memory>
#include <ostream>
#include <string>

namespace goom
{

class PluginInfo;

class GoomDots : public VisualFx
{
public:
  GoomDots() noexcept = delete;
  explicit GoomDots(const PluginInfo*);
  ~GoomDots() noexcept;
  GoomDots(const GoomDots&) = delete;
  GoomDots& operator=(const GoomDots&) = delete;

  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void apply(Pixel* prevBuff, Pixel* currentBuff) override;

  std::string getFxName() const override;
  void saveState(std::ostream&) const override;
  void loadState(std::istream&) override;

  void finish() override;

private:
  bool enabled = true;
  class GoomDotsImpl;
  std::unique_ptr<GoomDotsImpl> fxImpl;
};

} // namespace goom

#endif /* LIBS_GOOM_INCLUDE_GOOM_GOOM_DOTS_FX_H_ */
