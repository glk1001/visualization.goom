#ifndef _CONVOLVE_FX_H
#define _CONVOLVE_FX_H

#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <istream>
#include <memory>
#include <ostream>
#include <string>

namespace goom
{

struct ConvolveData;
struct PluginInfo;

class ConvolveFx : public VisualFx
{
public:
  ConvolveFx() = delete;
  explicit ConvolveFx(PluginInfo*);
  ~ConvolveFx() noexcept;
  ConvolveFx(const ConvolveFx&) = delete;
  ConvolveFx& operator=(const ConvolveFx&) = delete;

  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void apply(Pixel*, Pixel*) override{};
  void convolve(const Pixel* currentBuff, uint32_t* outputBuff);

  std::string getFxName() const override;
  void saveState(std::ostream&) override;
  void loadState(std::istream&) override;

  void log(const StatsLogValueFunc& logVal) const override;
  void finish() override;

private:
  PluginInfo* const goomInfo;
  bool enabled = true;
  std::unique_ptr<ConvolveData> fxData;
};

} // namespace goom
#endif
