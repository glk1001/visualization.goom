#ifndef IFS_FX_H
#define IFS_FX_H

/*
 * File created 11 april 2002 by JeKo <jeko@free.fr>
 */

#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <memory>
#include <string>

namespace goom
{

class PluginInfo;
class PixelBuffer;

class IfsDancersFx : public IVisualFx
{
public:
  enum class ColorMode
  {
    _null = -1,
    mapColors,
    mixColors,
    reverseMixColors,
    megaMapColorChange,
    megaMixColorChange,
    singleColors,
    sineMixColors,
    sineMapColors,
  };

  IfsDancersFx() noexcept;
  explicit IfsDancersFx(const std::shared_ptr<const PluginInfo>&) noexcept;
  ~IfsDancersFx() noexcept override;
  IfsDancersFx(const IfsDancersFx&) = delete;
  IfsDancersFx& operator=(const IfsDancersFx&) = delete;

  void init();

  // If not colorMode is not set, or set to '_null', returns
  // random weighted color mode.
  [[nodiscard]] ColorMode getColorMode() const;
  void setColorMode(ColorMode);

  void renew();
  void updateIncr();

  [[nodiscard]] std::string getFxName() const override;
  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void applyNoDraw() override;
  void apply(PixelBuffer&) override;
  void apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff) override;

  void log(const StatsLogValueFunc&) const override;
  void finish() override;

  bool operator==(const IfsDancersFx&) const;

private:
  bool enabled = true;
  class IfsDancersFxImpl;
  std::unique_ptr<IfsDancersFxImpl> fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace goom
#endif
