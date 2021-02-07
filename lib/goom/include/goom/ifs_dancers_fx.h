#ifndef VISUALIZATION_GOOM_IFS_DANCERS_FX_H
#define VISUALIZATION_GOOM_IFS_DANCERS_FX_H

/*
 * File created 11 april 2002 by JeKo <jeko@free.fr>
 */

#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <memory>
#include <string>

namespace GOOM
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
  IfsDancersFx(const IfsDancersFx&) noexcept = delete;
  IfsDancersFx(IfsDancersFx&&) noexcept = delete;
  auto operator=(const IfsDancersFx&) -> IfsDancersFx& = delete;
  auto operator=(IfsDancersFx&&) -> IfsDancersFx& = delete;

  [[nodiscard]] auto GetResourcesDirectory() const -> const std::string& override;
  void SetResourcesDirectory(const std::string& dirName) override;

  void Init();

  // If not colorMode is not set, or set to '_null', returns
  // random weighted color mode.
  [[nodiscard]] auto GetColorMode() const -> ColorMode;
  void SetColorMode(ColorMode c);

  void Renew();
  void UpdateIncr();

  [[nodiscard]] auto GetFxName() const -> std::string override;
  void SetBuffSettings(const FXBuffSettings& settings) override;

  void Start() override;

  void ApplyNoDraw();
  void Apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff);

  void Log(const StatsLogValueFunc& l) const override;
  void Finish() override;

  auto operator==(const IfsDancersFx& i) const -> bool;

private:
  bool m_enabled = true;
  class IfsDancersFxImpl;
  std::unique_ptr<IfsDancersFxImpl> m_fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace GOOM
#endif
