#ifndef VISUALIZATION_GOOM_FILTER_CONTROL_H
#define VISUALIZATION_GOOM_FILTER_CONTROL_H

#include "filters.h"
#include "goomutils/goomrand.h"

#include <memory>

namespace GOOM
{

class FilterControl
{
public:
  explicit FilterControl(const std::shared_ptr<const PluginInfo>& goomInfo) noexcept;
  virtual ~FilterControl() noexcept;

  auto GetHasChangedSinceMark() const -> bool;
  void ClearUnchangedMark();

  void SetReverseSetting(bool value);
  void ToggleReverseSetting();
  void SetVitesseSetting(int32_t value);
  void SetNoisifySetting(bool value);
  void SetNoiseFactorSetting(float value);
  void SetBlockyWavySetting(bool value);
  void SetHPlaneEffectSetting(int32_t value);
  void SetVPlaneEffectSetting(int32_t value);
  void SetRotateSetting(float value);
  void MultiplyRotateSetting(float factor);
  void ToggleRotateSetting();

  void ChangeMilieu();

  auto GetFilterSettings() const -> const ZoomFilterData&;
  void SetRandomFilterSettings();
  void SetRandomFilterSettings(ZoomFilterMode mode);
  void SetDefaultFilterSettings(ZoomFilterMode mode);

private:
  static const UTILS::Weights<ZoomFilterMode> WEIGHTED_FILTER_EVENTS;
  const std::shared_ptr<const PluginInfo> m_goomInfo;
  ZoomFilterData m_filterData{};
  class FilterEvents;
  std::unique_ptr<FilterEvents> m_filterEvents;
  bool m_hasChanged = false;

  auto GetNewRandomMode() const -> ZoomFilterMode;

  void SetDefaultSettings();

  void SetAmuletModeSettings();
  void SetCrystalBallModeSettings();
  void SetHypercos0ModeSettings();
  void SetHypercos1ModeSettings();
  void SetHypercos2ModeSettings();
  void SetNormalModeSettings();
  void SetScrunchModeSettings();
  void SetSpeedwayModeSettings();
  void SetWaterModeSettings();
  void SetWaveMode0Settings();
  void SetWaveMode1Settings();
  void SetYOnlyModeSettings();

  struct MinMaxValues
  {
    float minVal;
    float maxVal;
  };

  void SetRotate(float probability);
  void SetHypercosEffect(const MinMaxValues& minMaxFreq, const MinMaxValues& minMaxAmp);
  void SetWaveModeSettings(const MinMaxValues& minMaxFreq, const MinMaxValues& minMaxAmp);
  void SetWaveEffect(const MinMaxValues& minMaxFreq, const MinMaxValues& minMaxAmp);
  auto GetRandomHypercosEffect() const -> ZoomFilterData::HypercosEffect;
};

inline auto FilterControl::GetFilterSettings() const -> const ZoomFilterData&
{
  return m_filterData;
}

inline auto FilterControl::GetHasChangedSinceMark() const -> bool
{
  return m_hasChanged;
}

inline void FilterControl::ClearUnchangedMark()
{
  m_hasChanged = false;
}

inline void FilterControl::SetReverseSetting(const bool value)
{
  m_hasChanged = true;
  m_filterData.reverse = value;
}

inline void FilterControl::ToggleReverseSetting()
{
  m_hasChanged = true;
  m_filterData.reverse = !m_filterData.reverse;
}

inline void FilterControl::SetVitesseSetting(const int32_t value)
{
  m_hasChanged = true;
  m_filterData.vitesse = value;
}

inline void FilterControl::SetNoisifySetting(const bool value)
{
  m_hasChanged = true;
  m_filterData.noisify = value;
}

inline void FilterControl::SetNoiseFactorSetting(const float value)
{
  m_hasChanged = true;
  m_filterData.noiseFactor = value;
}

inline void FilterControl::SetBlockyWavySetting(const bool value)
{
  m_hasChanged = true;
  m_filterData.blockyWavy = value;
}

inline void FilterControl::SetHPlaneEffectSetting(const int32_t value)
{
  m_hasChanged = true;
  m_filterData.hPlaneEffect = value;
}

inline void FilterControl::SetVPlaneEffectSetting(const int32_t value)
{
  m_hasChanged = true;
  m_filterData.vPlaneEffect = value;
}

inline void FilterControl::SetRotateSetting(const float value)
{
  m_hasChanged = true;
  m_filterData.rotateSpeed = value;
}

inline void FilterControl::MultiplyRotateSetting(const float factor)
{
  m_hasChanged = true;
  m_filterData.rotateSpeed *= factor;
}

inline void FilterControl::ToggleRotateSetting()
{
  m_hasChanged = true;
  m_filterData.rotateSpeed = -m_filterData.rotateSpeed;
}

} // namespace GOOM

#endif //VISUALIZATION_GOOM_FILTER_CONTROL_H
