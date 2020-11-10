#ifndef IFS_FX_H
#define IFS_FX_H

/*
 * File created 11 april 2002 by JeKo <jeko@free.fr>
 */

#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <memory>
#include <string>

namespace goom
{

class PluginInfo;

class IfsFx : public VisualFx
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

  IfsFx() noexcept;
  explicit IfsFx(const std::shared_ptr<const PluginInfo>&) noexcept;
  ~IfsFx() noexcept;
  IfsFx(const IfsFx&) = delete;
  IfsFx& operator=(const IfsFx&) = delete;

  // If not colorMode is not set, or set to '_null', returns
  // random weighted color mode.
  ColorMode getColorMode() const;
  void setColorMode(const ColorMode);

  void renew();
  void updateIncr();

  std::string getFxName() const override;
  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void applyNoDraw() override;
  void apply(Pixel* prevBuff, Pixel* currentBuff) override;

  void log(const StatsLogValueFunc&) const override;
  void finish() override;

  bool operator==(const IfsFx&) const;

private:
  bool enabled = true;
  class IfsImpl;
  std::unique_ptr<IfsImpl> fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace goom
#endif
