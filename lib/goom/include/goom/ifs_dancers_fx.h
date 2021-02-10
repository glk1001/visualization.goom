#ifndef VISUALIZATION_GOOM_IFS_DANCERS_FX_H
#define VISUALIZATION_GOOM_IFS_DANCERS_FX_H

#include "goom_visual_fx.h"

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

  IfsDancersFx() noexcept = delete;
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

private:
  bool m_enabled = true;
  class IfsDancersFxImpl;
  const std::unique_ptr<IfsDancersFxImpl> m_fxImpl;
};

} // namespace GOOM
#endif
