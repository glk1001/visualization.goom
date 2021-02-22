#include "filter_control.h"

#include "goom/goom_plugin_info.h"
#include "goomutils/goomrand.h"

namespace GOOM
{

using GOOM::UTILS::GetRandInRange;
using GOOM::UTILS::ProbabilityOf;
using GOOM::UTILS::ProbabilityOfMInN;
using GOOM::UTILS::Weights;

constexpr float PROB_HIGH = 0.9;
constexpr float PROB_HALF = 0.5;
constexpr float PROB_LOW = 0.1;

class FilterControl::FilterEvents
{
public:
  FilterEvents() noexcept = default;
  ~FilterEvents() = default;
  FilterEvents(const FilterEvents&) noexcept = delete;
  FilterEvents(FilterEvents&&) noexcept = delete;
  auto operator=(const FilterEvents&) -> FilterEvents& = delete;
  auto operator=(FilterEvents&&) -> FilterEvents& = delete;

  enum FilterEventTypes
  {
    ROTATE,
    HYPERCOS_EFFECT,
    WAVE_EFFECT,
    ALLOW_STRANGE_WAVE_VALUES,
    CHANGE_SPEED,
    _SIZE // must be last - gives number of enums
  };

  struct Event
  {
    FilterEventTypes event;
    // m out of n
    uint32_t m;
    uint32_t outOf;
  };

  auto Happens(FilterEventTypes event) -> bool;

private:
  static constexpr size_t NUM_FILTER_EVENT_TYPES = static_cast<size_t>(FilterEventTypes::_SIZE);

  //@formatter:off
  // clang-format off
#if __cplusplus <= 201402L
  static const std::array<Event, NUM_FILTER_EVENT_TYPES> WEIGHTED_EVENTS;
#else
  static constexpr std::array<Event, NUM_FILTER_EVENT_TYPES> WEIGHTED_EVENTS{{
     { /*.event = */FilterEventTypes::ROTATE,                     /*.m = */8, /*.outOf = */ 16 },
     { /*.event = */FilterEventTypes::HYPERCOS_EFFECT,            /*.m = */8, /*.outOf = */ 16 },
     { /*.event = */FilterEventTypes::WAVE_EFFECT,                /*.m = */8, /*.outOf = */ 16 },
     { /*.event = */FilterEventTypes::ALLOW_STRANGE_WAVE_VALUES,  /*.m = */8, /*.outOf = */ 16 },
     { /*.event = */FilterEventTypes::CHANGE_SPEED,               /*.m = */8, /*.outOf = */ 16 },
  }};
#endif
  // clang-format on
  //@formatter:on
};

#if __cplusplus <= 201402L
const std::array<FilterControl::FilterEvents::Event,
                 FilterControl::FilterEvents::NUM_FILTER_EVENT_TYPES>
    FilterControl::FilterEvents::WEIGHTED_EVENTS{{
        {/*.event = */ FilterControl::FilterEvents::FilterEventTypes::ROTATE, /*.m = */ 8,
         /*.outOf = */ 16},
        {/*.event = */ FilterControl::FilterEvents::FilterEventTypes::HYPERCOS_EFFECT, /*.m = */ 8,
         /*.outOf = */ 16},
        {/*.event = */ FilterControl::FilterEvents::FilterEventTypes::WAVE_EFFECT, /*.m = */ 8,
         /*.outOf = */ 16},
        {/*.event = */ FilterControl::FilterEvents::FilterEventTypes::ALLOW_STRANGE_WAVE_VALUES,
         /*.m = */ 8, /*.outOf = */ 16},
        {/*.event = */ FilterControl::FilterEvents::FilterEventTypes::CHANGE_SPEED, /*.m = */ 8,
         /*.outOf = */ 16},
    }};
#endif

inline auto FilterControl::FilterEvents::Happens(const FilterEventTypes event) -> bool
{
  const Event& weightedEvent = WEIGHTED_EVENTS[static_cast<size_t>(event)];
  return ProbabilityOfMInN(weightedEvent.m, weightedEvent.outOf);
}

FilterControl::FilterControl(const std::shared_ptr<const PluginInfo>& goomInfo) noexcept
  : m_goomInfo{goomInfo}, m_filterEvents{std::make_unique<FilterEvents>()}
{
}

FilterControl::~FilterControl() noexcept = default;

void FilterControl::SetRandomFilterSettings()
{
  SetRandomFilterSettings(GetNewRandomMode());
}

void FilterControl::SetRandomFilterSettings(ZoomFilterMode mode)
{
  m_hasChanged = true;

  m_filterData.mode = mode;

  SetDefaultSettings();

  switch (m_filterData.mode)
  {
    case ZoomFilterMode::AMULET_MODE:
      SetAmuletModeSettings();
      break;
    case ZoomFilterMode::CRYSTAL_BALL_MODE:
      SetCrystalBallModeSettings();
      break;
    case ZoomFilterMode::HYPERCOS1_MODE:
      SetHypercos1ModeSettings();
      break;
    case ZoomFilterMode::HYPERCOS2_MODE:
      SetHypercos2ModeSettings();
      break;
    case ZoomFilterMode::NORMAL_MODE:
      SetNormalModeSettings();
      break;
    case ZoomFilterMode::SCRUNCH_MODE:
      SetScrunchModeSettings();
      break;
    case ZoomFilterMode::SPEEDWAY_MODE:
      SetSpeedwayModeSettings();
      break;
    case ZoomFilterMode::WATER_MODE:
      SetWaterModeSettings();
      break;
    case ZoomFilterMode::WAVE_MODE:
      SetWaveModeSettings();
      break;
    case ZoomFilterMode::Y_ONLY_MODE:
      SetYOnlyModeSettings();
      break;
    default:
      throw std::logic_error("ZoomFilterMode not implemented.");
  }
}

auto FilterControl::GetNewRandomMode() const -> ZoomFilterMode
{
  //@formatter:off
  // clang-format off
  static const Weights<ZoomFilterMode> s_weightedFilterEvents{{
    { ZoomFilterMode::AMULET_MODE,       2 },
    { ZoomFilterMode::CRYSTAL_BALL_MODE, 3 },
    { ZoomFilterMode::HYPERCOS1_MODE,    3 },
    { ZoomFilterMode::HYPERCOS2_MODE,    3 },
    { ZoomFilterMode::NORMAL_MODE,       2 },
    { ZoomFilterMode::SCRUNCH_MODE,      3 },
    { ZoomFilterMode::SPEEDWAY_MODE,     3 },
    { ZoomFilterMode::WAVE_MODE,         5 },
    { ZoomFilterMode::WATER_MODE,        0 },
    { ZoomFilterMode::Y_ONLY_MODE,       1 },
  }};
  //@formatter:on
  // clang-format on

  uint32_t numTries = 0;
  constexpr uint32_t MAX_TRIES = 20;

  while (true)
  {
    const auto newMode = s_weightedFilterEvents.GetRandomWeighted();
    if (newMode != m_filterData.mode)
    {
      return newMode;
    }
    numTries++;
    if (numTries >= MAX_TRIES)
    {
      break;
    }
  }

  return m_filterData.mode;
}

void FilterControl::SetDefaultSettings()
{
  m_filterData.middleX = 16;
  m_filterData.middleY = 1;

  m_filterData.vitesse = ZoomFilterData::DEFAULT_VITESSE;
  m_filterData.coeffVitesseDenominator = ZoomFilterData::DEFAULT_COEFF_VITESSE_DENOMINATOR;
  m_filterData.reverse = true;

  m_filterData.tanEffect = ProbabilityOfMInN(1, 10);
  m_filterData.rotateSpeed = 0.0;
  m_filterData.noisify = false;
  m_filterData.noiseFactor = 1;
  m_filterData.blockyWavy = false;

  m_filterData.waveEffect = false; // TODO - not used?

  m_filterData.hPlaneEffect = 0;
  m_filterData.hPlaneEffectAmplitude = ZoomFilterData::DEFAULT_H_PLANE_EFFECT_AMPLITUDE;

  m_filterData.vPlaneEffect = 0;
  m_filterData.vPlaneEffectAmplitude = ZoomFilterData::DEFAULT_V_PLANE_EFFECT_AMPLITUDE;

  m_filterData.amuletAmplitude = ZoomFilterData::DEFAULT_AMULET_AMPLITUDE;
  m_filterData.crystalBallAmplitude = ZoomFilterData::DEFAULT_CRYSTAL_BALL_AMPLITUDE;
  m_filterData.scrunchAmplitude = ZoomFilterData::DEFAULT_SCRUNCH_AMPLITUDE;
  m_filterData.speedwayAmplitude = ZoomFilterData::DEFAULT_SPEEDWAY_AMPLITUDE;

  m_filterData.waveEffectType = ZoomFilterData::DEFAULT_WAVE_EFFECT_TYPE;
  m_filterData.waveFreqFactor = ZoomFilterData::DEFAULT_WAVE_FREQ_FACTOR;
  m_filterData.waveAmplitude = ZoomFilterData::DEFAULT_WAVE_AMPLITUDE;

  m_filterData.hypercosEffect = ZoomFilterData::HypercosEffect::NONE;
  m_filterData.hypercosReverse = false;
  m_filterData.hypercosFreqX = ZoomFilterData::DEFAULT_HYPERCOS_FREQ;
  m_filterData.hypercosFreqY = ZoomFilterData::DEFAULT_HYPERCOS_FREQ;
  m_filterData.hypercosAmplitudeX = ZoomFilterData::DEFAULT_HYPERCOS_AMPLITUDE;
  m_filterData.hypercosAmplitudeY = ZoomFilterData::DEFAULT_HYPERCOS_AMPLITUDE;
}

void FilterControl::SetAmuletModeSettings()
{
  SetRotate(PROB_HIGH);

  m_filterData.amuletAmplitude =
      GetRandInRange(ZoomFilterData::MIN_AMULET_AMPLITUDE, ZoomFilterData::MAX_AMULET_AMPLITUDE);
}

void FilterControl::SetCrystalBallModeSettings()
{
  SetRotate(PROB_LOW);

  m_filterData.crystalBallAmplitude = GetRandInRange(ZoomFilterData::MIN_CRYSTAL_BALL_AMPLITUDE,
                                                     ZoomFilterData::MAX_CRYSTAL_BALL_AMPLITUDE);
}

void FilterControl::SetHypercos1ModeSettings()
{
  SetRotate(PROB_HIGH);
  SetHypercosEffect(false);
}

void FilterControl::SetHypercos2ModeSettings()
{
  SetRotate(PROB_HIGH);
  SetHypercosEffect(true);
}

void FilterControl::SetNormalModeSettings()
{
}

void FilterControl::SetScrunchModeSettings()
{
  SetRotate(PROB_HALF);

  m_filterData.scrunchAmplitude =
      GetRandInRange(ZoomFilterData::MIN_SCRUNCH_AMPLITUDE, ZoomFilterData::MAX_SCRUNCH_AMPLITUDE);
}

void FilterControl::SetSpeedwayModeSettings()
{
  SetRotate(PROB_LOW);

  m_filterData.speedwayAmplitude = GetRandInRange(ZoomFilterData::MIN_SPEEDWAY_AMPLITUDE,
                                                  ZoomFilterData::MAX_SPEEDWAY_AMPLITUDE);
}

void FilterControl::SetWaterModeSettings()
{
}

void FilterControl::SetWaveModeSettings()
{
  SetRotate(PROB_LOW);

  using EventTypes = FilterControl::FilterEvents::FilterEventTypes;

  m_filterData.reverse = ProbabilityOfMInN(1, 2);
  if (m_filterEvents->Happens(EventTypes::CHANGE_SPEED))
  {
    m_filterData.vitesse = (m_filterData.vitesse + ZoomFilterData::DEFAULT_VITESSE) >> 1;
  }
  m_filterData.waveEffectType = static_cast<ZoomFilterData::WaveEffect>(GetRandInRange(0, 2));
  if (m_filterEvents->Happens(EventTypes::ALLOW_STRANGE_WAVE_VALUES))
  {
    // BUG HERE - wrong ranges - BUT GIVES GOOD AFFECT
    m_filterData.waveAmplitude = GetRandInRange(ZoomFilterData::MIN_LARGE_WAVE_AMPLITUDE,
                                                ZoomFilterData::MAX_LARGE_WAVE_AMPLITUDE);
    m_filterData.waveFreqFactor = GetRandInRange(ZoomFilterData::MIN_WAVE_SMALL_FREQ_FACTOR,
                                                 ZoomFilterData::MAX_WAVE_SMALL_FREQ_FACTOR);
  }
  else
  {
    m_filterData.waveAmplitude =
        GetRandInRange(ZoomFilterData::MIN_WAVE_AMPLITUDE, ZoomFilterData::MAX_WAVE_AMPLITUDE);
    m_filterData.waveFreqFactor =
        GetRandInRange(ZoomFilterData::MIN_WAVE_FREQ_FACTOR, ZoomFilterData::MAX_WAVE_FREQ_FACTOR);
  }
}

void FilterControl::SetYOnlyModeSettings()
{
  SetRotate(PROB_HALF);
}

void FilterControl::SetRotate(const float probability)
{
  if (!ProbabilityOf(probability))
  {
    return;
  }

  m_filterData.rotateSpeed =
      GetRandInRange(ZoomFilterData::MIN_ROTATE_SPEED, ZoomFilterData::MAX_ROTATE_SPEED);
}

void FilterControl::SetWaveEffect()
{
  using EventTypes = FilterControl::FilterEvents::FilterEventTypes;

  m_filterData.waveEffect = m_filterEvents->Happens(EventTypes::WAVE_EFFECT);
}

void FilterControl::SetHypercosEffect(const bool allowBigFrequency)
{
  m_filterData.hypercosEffect = GetRandomHypercosEffect();
  if (m_filterData.hypercosEffect == ZoomFilterData::HypercosEffect::NONE)
  {
    return;
  }

  const float maxFreq =
      allowBigFrequency ? ZoomFilterData::BIG_MAX_HYPERCOS_FREQ : ZoomFilterData::MAX_HYPERCOS_FREQ;
  m_filterData.hypercosFreqX = GetRandInRange(ZoomFilterData::MIN_HYPERCOS_FREQ, maxFreq);
  if (ProbabilityOfMInN(1, 4))
  {
    m_filterData.hypercosFreqY = m_filterData.hypercosFreqX;
  }
  else
  {
    m_filterData.hypercosFreqY = GetRandInRange(ZoomFilterData::MIN_HYPERCOS_FREQ, maxFreq);
  }

  m_filterData.hypercosReverse = ProbabilityOfMInN(1, 2);

  m_filterData.hypercosAmplitudeX = GetRandInRange(ZoomFilterData::MIN_HYPERCOS_AMPLITUDE,
                                                   ZoomFilterData::MAX_HYPERCOS_AMPLITUDE);
  if (ProbabilityOfMInN(1, 2))
  {
    m_filterData.hypercosAmplitudeY = m_filterData.hypercosAmplitudeX;
  }
  else
  {
    m_filterData.hypercosAmplitudeY = GetRandInRange(ZoomFilterData::MIN_HYPERCOS_AMPLITUDE,
                                                     ZoomFilterData::MAX_HYPERCOS_AMPLITUDE);
  }
}

inline auto FilterControl::GetRandomHypercosEffect() const -> ZoomFilterData::HypercosEffect
{
  using EventTypes = FilterControl::FilterEvents::FilterEventTypes;

  if (!m_filterEvents->Happens(EventTypes::HYPERCOS_EFFECT))
  {
    return ZoomFilterData::HypercosEffect::NONE;
  }

  return static_cast<ZoomFilterData::HypercosEffect>(
      GetRandInRange(static_cast<uint32_t>(ZoomFilterData::HypercosEffect::NONE) + 1,
                     static_cast<uint32_t>(ZoomFilterData::HypercosEffect::_SIZE)));
}

void FilterControl::ChangeMilieu()
{
  m_hasChanged = true;

  if ((m_filterData.mode == ZoomFilterMode::WATER_MODE) ||
      (m_filterData.mode == ZoomFilterMode::Y_ONLY_MODE) ||
      (m_filterData.mode == ZoomFilterMode::AMULET_MODE))
  {
    m_filterData.middleX = m_goomInfo->GetScreenInfo().width / 2;
    m_filterData.middleY = m_goomInfo->GetScreenInfo().height / 2;
  }
  else
  {
    // clang-format off
    enum class MiddlePointEvents { EVENT1, EVENT2, EVENT3, EVENT4 };
    static const Weights<MiddlePointEvents> s_middlePointWeights{{
       { MiddlePointEvents::EVENT1,  3 },
       { MiddlePointEvents::EVENT2,  2 },
       { MiddlePointEvents::EVENT3,  2 },
       { MiddlePointEvents::EVENT4, 18 },
    }};
    // clang-format on

    switch (s_middlePointWeights.GetRandomWeighted())
    {
      case MiddlePointEvents::EVENT1:
        m_filterData.middleX = m_goomInfo->GetScreenInfo().width / 2;
        m_filterData.middleY = m_goomInfo->GetScreenInfo().height - 1;
        break;
      case MiddlePointEvents::EVENT2:
        m_filterData.middleX = m_goomInfo->GetScreenInfo().width - 1;
        break;
      case MiddlePointEvents::EVENT3:
        m_filterData.middleX = 1;
        break;
      case MiddlePointEvents::EVENT4:
        m_filterData.middleX = m_goomInfo->GetScreenInfo().width / 2;
        m_filterData.middleY = m_goomInfo->GetScreenInfo().height / 2;
        break;
      default:
        throw std::logic_error("Unknown MiddlePointEvents enum.");
    }
  }

  // clang-format off
  // @formatter:off
  enum class PlaneEffectEvents { EVENT1, EVENT2, EVENT3, EVENT4, EVENT5, EVENT6, EVENT7, EVENT8 };
  static const Weights<PlaneEffectEvents> s_planeEffectWeights{{
     { PlaneEffectEvents::EVENT1,  1 },
     { PlaneEffectEvents::EVENT2,  1 },
     { PlaneEffectEvents::EVENT3,  4 },
     { PlaneEffectEvents::EVENT4,  1 },
     { PlaneEffectEvents::EVENT5,  1 },
     { PlaneEffectEvents::EVENT6,  1 },
     { PlaneEffectEvents::EVENT7,  1 },
     { PlaneEffectEvents::EVENT8,  2 },
  }};
  // clang-format on
  // @formatter:on

  switch (s_planeEffectWeights.GetRandomWeighted())
  {
    case PlaneEffectEvents::EVENT1:
      m_filterData.vPlaneEffect = GetRandInRange(-2, +3);
      m_filterData.hPlaneEffect = GetRandInRange(-2, +3);
      break;
    case PlaneEffectEvents::EVENT2:
      m_filterData.vPlaneEffect = 0;
      m_filterData.hPlaneEffect = GetRandInRange(-7, +8);
      break;
    case PlaneEffectEvents::EVENT3:
      m_filterData.vPlaneEffect = GetRandInRange(-5, +6);
      m_filterData.hPlaneEffect = -m_filterData.vPlaneEffect;
      break;
    case PlaneEffectEvents::EVENT4:
      m_filterData.hPlaneEffect = static_cast<int>(GetRandInRange(5U, 13U));
      m_filterData.vPlaneEffect = -m_filterData.hPlaneEffect;
      break;
    case PlaneEffectEvents::EVENT5:
      m_filterData.vPlaneEffect = static_cast<int>(GetRandInRange(5U, 13U));
      m_filterData.hPlaneEffect = -m_filterData.hPlaneEffect;
      break;
    case PlaneEffectEvents::EVENT6:
      m_filterData.hPlaneEffect = 0;
      m_filterData.vPlaneEffect = GetRandInRange(-9, +10);
      break;
    case PlaneEffectEvents::EVENT7:
      m_filterData.hPlaneEffect = GetRandInRange(-9, +10);
      m_filterData.vPlaneEffect = GetRandInRange(-9, +10);
      break;
    case PlaneEffectEvents::EVENT8:
      m_filterData.vPlaneEffect = 0;
      m_filterData.hPlaneEffect = 0;
      break;
    default:
      throw std::logic_error("Unknown MiddlePointEvents enum.");
  }
  m_filterData.hPlaneEffectAmplitude = GetRandInRange(ZoomFilterData::MIN_H_PLANE_EFFECT_AMPLITUDE,
                                                      ZoomFilterData::MAX_H_PLANE_EFFECT_AMPLITUDE);
  // TODO I think 'vPlaneEffectAmplitude' has to be the same as 'hPlaneEffectAmplitude' otherwise
  //   buffer breaking effects occur.
  m_filterData.vPlaneEffectAmplitude =
      m_filterData.hPlaneEffectAmplitude + GetRandInRange(-0.0009F, 0.0009F);
  //  goomData.zoomFilterData.vPlaneEffectAmplitude = getRandInRange(
  //      ZoomFilterData::minVPlaneEffectAmplitude, ZoomFilterData::maxVPlaneEffectAmplitude);
}

} // namespace GOOM
