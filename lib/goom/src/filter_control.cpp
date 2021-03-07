#include "filter_control.h"

#include "goom/goom_plugin_info.h"
#include "goomutils/goomrand.h"
#include "image_displacement.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace GOOM
{

using GOOM::UTILS::GetRandInRange;
using GOOM::UTILS::ProbabilityOf;
using GOOM::UTILS::ProbabilityOfMInN;
using GOOM::UTILS::Weights;

constexpr float PROB_HIGH = 0.9;
constexpr float PROB_HALF = 0.5;
constexpr float PROB_LOW = 0.1;

//@formatter:off
// clang-format off
const Weights<ZoomFilterMode> FilterControl::WEIGHTED_FILTER_EVENTS{{
    { ZoomFilterMode::AMULET_MODE,            5 },
    { ZoomFilterMode::CRYSTAL_BALL_MODE,      8 },
    { ZoomFilterMode::HYPERCOS0_MODE,         5 },
    { ZoomFilterMode::HYPERCOS1_MODE,         3 },
    { ZoomFilterMode::HYPERCOS2_MODE,         2 },
    { ZoomFilterMode::IMAGE_DISPLACEMENT_MODE,5 },
    { ZoomFilterMode::NORMAL_MODE,            6 },
    { ZoomFilterMode::SCRUNCH_MODE,           6 },
    { ZoomFilterMode::SPEEDWAY_MODE,          6 },
    { ZoomFilterMode::WAVE_MODE0,             5 },
    { ZoomFilterMode::WAVE_MODE1,             4 },
    { ZoomFilterMode::WATER_MODE,             0 },
    { ZoomFilterMode::Y_ONLY_MODE,            4 },
}};
//@formatter:on
// clang-format on

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
    ROTATE = 0,
    HYPERCOS_EFFECT,
    WAVE_EFFECT,
    ALLOW_STRANGE_WAVE_VALUES,
    CHANGE_SPEED,
    REVERSE_SPEED,
    HYPERCOS_REVERSE,
    HYPERCOS_FREQ_EQUAL,
    HYPERCOS_AMP_EQUAL,
    PLANE_AMP_EQUAL,
    _SIZE // must be last - gives number of enums
  };

  struct Event
  {
    FilterEventTypes event;
    // m out of n
    uint32_t m;
    uint32_t outOf;
  };

  static auto Happens(FilterEventTypes event) -> bool;

private:
  static constexpr size_t NUM_FILTER_EVENT_TYPES = static_cast<size_t>(FilterEventTypes::_SIZE);

  //@formatter:off
  // clang-format off
#if __cplusplus <= 201402L
  static const std::array<Event, NUM_FILTER_EVENT_TYPES> WEIGHTED_EVENTS;
#else
  static constexpr std::array<Event, NUM_FILTER_EVENT_TYPES> WEIGHTED_EVENTS{{
     { .event = FilterEventTypes::ROTATE,                     .m =  8, .outOf = 16 },
     { .event = FilterEventTypes::HYPERCOS_EFFECT,            .m =  8, .outOf = 16 },
     { .event = FilterEventTypes::WAVE_EFFECT,                .m =  8, .outOf = 16 },
     { .event = FilterEventTypes::ALLOW_STRANGE_WAVE_VALUES,  .m =  1, .outOf = 16 },
     { .event = FilterEventTypes::CHANGE_SPEED,               .m =  8, .outOf = 16 },
     { .event = FilterEventTypes::REVERSE_SPEED,              .m =  8, .outOf = 16 },
     { .event = FilterEventTypes::HYPERCOS_REVERSE,           .m =  8, .outOf = 16 },
     { .event = FilterEventTypes::HYPERCOS_FREQ_EQUAL,        .m =  4, .outOf = 16 },
     { .event = FilterEventTypes::HYPERCOS_AMP_EQUAL,         .m =  8, .outOf = 16 },
     { .event = FilterEventTypes::PLANE_AMP_EQUAL,            .m = 12, .outOf = 16 },
  }};
#endif
  // clang-format on
  //@formatter:on
};

#if __cplusplus <= 201402L
//@formatter:off
// clang-format off
const std::array<FilterControl::FilterEvents::Event, FilterControl::FilterEvents::NUM_FILTER_EVENT_TYPES>
    FilterControl::FilterEvents::WEIGHTED_EVENTS{{
   {/*.event = */ FilterEventTypes::ROTATE,                    /*.m = */  8, /*.outOf = */ 16},
   {/*.event = */ FilterEventTypes::HYPERCOS_EFFECT,           /*.m = */  1, /*.outOf = */ 50},
   {/*.event = */ FilterEventTypes::WAVE_EFFECT,               /*.m = */ 12, /*.outOf = */ 16},
   {/*.event = */ FilterEventTypes::ALLOW_STRANGE_WAVE_VALUES, /*.m = */ 10, /*.outOf = */ 50},
   {/*.event = */ FilterEventTypes::CHANGE_SPEED,              /*.m = */  8, /*.outOf = */ 16},
   {/*.event = */ FilterEventTypes::REVERSE_SPEED,             /*.m = */  8, /*.outOf = */ 16},
   {/*.event = */ FilterEventTypes::HYPERCOS_REVERSE,          /*.m = */  8, /*.outOf = */ 16},
   {/*.event = */ FilterEventTypes::HYPERCOS_FREQ_EQUAL,       /*.m = */  4, /*.outOf = */ 16},
   {/*.event = */ FilterEventTypes::HYPERCOS_AMP_EQUAL,        /*.m = */  8, /*.outOf = */ 16},
   {/*.event = */ FilterEventTypes::PLANE_AMP_EQUAL,           /*.m = */ 12, /*.outOf = */ 16},
}};
// clang-format on
//@formatter:on
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

void FilterControl::Start()
{
  if (m_resourcesDirectory.empty())
  {
    throw std::logic_error("FilterControl::Start: Resources directory is empty.");
  }

  m_imageDisplacements.resize(IMAGE_FILENAMES.size());
  for (size_t i = 0; i < IMAGE_FILENAMES.size(); i++)
  {
    m_imageDisplacements[i] =
        std::make_shared<ImageDisplacement>(GetImageFilename(IMAGE_FILENAMES[i]));
  }
}

const std::vector<std::string> FilterControl::IMAGE_FILENAMES{
    "pattern1.jpg", "pattern2.jpg", "pattern3.jpg", "chameleon-tail.jpg", "mountain_sunset.png",
};

inline auto FilterControl::GetImageFilename(const std::string& imageFilename) -> std::string
{
  return m_resourcesDirectory + PATH_SEP + IMAGES_DIR + PATH_SEP + IMAGE_DISPLACEMENT_DIR +
         PATH_SEP + imageFilename;
}

void FilterControl::SetRandomFilterSettings()
{
  SetRandomFilterSettings(GetNewRandomMode());
}

void FilterControl::SetDefaultFilterSettings(ZoomFilterMode mode)
{
  m_hasChanged = true;

  m_filterData.mode = mode;

  SetDefaultSettings();
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
    case ZoomFilterMode::HYPERCOS0_MODE:
      SetHypercos0ModeSettings();
      break;
    case ZoomFilterMode::HYPERCOS1_MODE:
      SetHypercos1ModeSettings();
      break;
    case ZoomFilterMode::HYPERCOS2_MODE:
      SetHypercos2ModeSettings();
      break;
    case ZoomFilterMode::IMAGE_DISPLACEMENT_MODE:
      SetImageDisplacementModeSettings();
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
    case ZoomFilterMode::WAVE_MODE0:
      SetWaveMode0Settings();
      break;
    case ZoomFilterMode::WAVE_MODE1:
      SetWaveMode1Settings();
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
  uint32_t numTries = 0;
  constexpr uint32_t MAX_TRIES = 20;

  while (true)
  {
    const auto newMode = WEIGHTED_FILTER_EVENTS.GetRandomWeighted();
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
  m_filterData.reverseSpeed = true;

  m_filterData.tanEffect = false; //ProbabilityOfMInN(1, 10);
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
  m_filterData.crystalBallSqDistOffset = ZoomFilterData::DEFAULT_CRYSTAL_BALL_SQ_DIST_OFFSET;
  m_filterData.imageDisplacementAmplitude = ZoomFilterData::DEFAULT_IMAGE_DISPL_AMPLITUDE;
  m_filterData.imageDisplacement = nullptr;
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

  using EventTypes = FilterControl::FilterEvents::FilterEventTypes;
  if (m_filterEvents->Happens(EventTypes::HYPERCOS_EFFECT))
  {
    SetHypercos2ModeSettings();
  }
}

void FilterControl::SetCrystalBallModeSettings()
{
  SetRotate(PROB_LOW);

  m_filterData.crystalBallAmplitude = GetRandInRange(ZoomFilterData::MIN_CRYSTAL_BALL_AMPLITUDE,
                                                     ZoomFilterData::MAX_CRYSTAL_BALL_AMPLITUDE);
  m_filterData.crystalBallSqDistOffset =
      GetRandInRange(ZoomFilterData::MIN_CRYSTAL_BALL_SQ_DIST_OFFSET,
                     ZoomFilterData::MAX_CRYSTAL_BALL_SQ_DIST_OFFSET);

  using EventTypes = FilterControl::FilterEvents::FilterEventTypes;
  if (m_filterEvents->Happens(EventTypes::HYPERCOS_EFFECT))
  {
    SetHypercos2ModeSettings();
  }
}

void FilterControl::SetHypercos0ModeSettings()
{
  SetRotate(PROB_LOW);
  SetHypercosEffect(
      {ZoomFilterData::MIN_HYPERCOS_FREQ, 0.10F * ZoomFilterData::MAX_HYPERCOS_FREQ},
      {ZoomFilterData::MIN_HYPERCOS_AMPLITUDE, ZoomFilterData::MAX_HYPERCOS_AMPLITUDE});
}

void FilterControl::SetHypercos1ModeSettings()
{
  SetRotate(PROB_LOW);
  SetHypercosEffect(
      {ZoomFilterData::MIN_HYPERCOS_FREQ, ZoomFilterData::MAX_HYPERCOS_FREQ},
      {ZoomFilterData::MIN_HYPERCOS_AMPLITUDE, ZoomFilterData::MAX_HYPERCOS_AMPLITUDE});
}

void FilterControl::SetHypercos2ModeSettings()
{
  SetRotate(PROB_LOW);
  SetHypercosEffect(
      {ZoomFilterData::MIN_HYPERCOS_FREQ, ZoomFilterData::BIG_MAX_HYPERCOS_FREQ},
      {ZoomFilterData::MIN_HYPERCOS_AMPLITUDE, ZoomFilterData::MAX_HYPERCOS_AMPLITUDE});
}

void FilterControl::SetImageDisplacementModeSettings()
{
  m_filterData.imageDisplacement =
      m_imageDisplacements[GetRandInRange(0U, m_imageDisplacements.size())];

  m_filterData.imageDisplacementAmplitude = GetRandInRange(
      ZoomFilterData::MIN_IMAGE_DISPL_AMPLITUDE, ZoomFilterData::MAX_IMAGE_DISPL_AMPLITUDE);

  m_filterData.imageDisplacementXColorCutoff = GetRandInRange(
      ZoomFilterData::MIN_IMAGE_DISPL_COLOR_CUTOFF, ZoomFilterData::MAX_IMAGE_DISPL_COLOR_CUTOFF);
  m_filterData.imageDisplacementYColorCutoff = GetRandInRange(
      ZoomFilterData::MIN_IMAGE_DISPL_COLOR_CUTOFF, ZoomFilterData::MAX_IMAGE_DISPL_COLOR_CUTOFF);

  using EventTypes = FilterControl::FilterEvents::FilterEventTypes;
  if (m_filterEvents->Happens(EventTypes::HYPERCOS_EFFECT))
  {
    SetHypercos1ModeSettings();
  }
}

void FilterControl::SetNormalModeSettings()
{
}

void FilterControl::SetScrunchModeSettings()
{
  SetRotate(PROB_HALF);

  m_filterData.scrunchAmplitude =
      GetRandInRange(ZoomFilterData::MIN_SCRUNCH_AMPLITUDE, ZoomFilterData::MAX_SCRUNCH_AMPLITUDE);

  using EventTypes = FilterControl::FilterEvents::FilterEventTypes;
  if (m_filterEvents->Happens(EventTypes::HYPERCOS_EFFECT))
  {
    SetHypercos1ModeSettings();
  }
}

void FilterControl::SetSpeedwayModeSettings()
{
  SetRotate(PROB_LOW);

  m_filterData.speedwayAmplitude = GetRandInRange(ZoomFilterData::MIN_SPEEDWAY_AMPLITUDE,
                                                  ZoomFilterData::MAX_SPEEDWAY_AMPLITUDE);

  using EventTypes = FilterControl::FilterEvents::FilterEventTypes;
  if (m_filterEvents->Happens(EventTypes::HYPERCOS_EFFECT))
  {
    SetHypercos0ModeSettings();
  }
}

void FilterControl::SetWaterModeSettings()
{
}

void FilterControl::SetWaveMode0Settings()
{
  SetRotate(PROB_LOW);

  SetWaveModeSettings({ZoomFilterData::MIN_WAVE_FREQ_FACTOR, ZoomFilterData::MAX_WAVE_FREQ_FACTOR},
                      {ZoomFilterData::MIN_WAVE_AMPLITUDE, ZoomFilterData::MAX_WAVE_AMPLITUDE});
}

void FilterControl::SetWaveMode1Settings()
{
  SetRotate(PROB_HIGH);

  using EventTypes = FilterControl::FilterEvents::FilterEventTypes;

  if (m_filterEvents->Happens(EventTypes::ALLOW_STRANGE_WAVE_VALUES))
  {
    SetWaveModeSettings(
        {ZoomFilterData::SMALL_MIN_WAVE_FREQ_FACTOR, ZoomFilterData::SMALL_MAX_WAVE_FREQ_FACTOR},
        {ZoomFilterData::BIG_MIN_WAVE_AMPLITUDE, ZoomFilterData::BIG_MAX_WAVE_AMPLITUDE});
  }
  else
  {
    SetWaveModeSettings(
        {ZoomFilterData::MIN_WAVE_FREQ_FACTOR, ZoomFilterData::MAX_WAVE_FREQ_FACTOR},
        {ZoomFilterData::MIN_WAVE_AMPLITUDE, ZoomFilterData::MAX_WAVE_AMPLITUDE});
  }
}

void FilterControl::SetWaveModeSettings(const MinMaxValues& minMaxFreq,
                                        const MinMaxValues& minMaxAmp)
{
  using EventTypes = FilterControl::FilterEvents::FilterEventTypes;

  m_filterData.reverseSpeed = m_filterEvents->Happens(EventTypes::REVERSE_SPEED);

  if (m_filterEvents->Happens(EventTypes::CHANGE_SPEED))
  {
    m_filterData.vitesse = (m_filterData.vitesse + ZoomFilterData::DEFAULT_VITESSE) >> 1;
  }

  m_filterData.waveEffectType = static_cast<ZoomFilterData::WaveEffect>(GetRandInRange(0, 2));

  SetWaveEffect(minMaxFreq, minMaxAmp);
}

void FilterControl::SetWaveEffect(const MinMaxValues& minMaxFreq, const MinMaxValues& minMaxAmp)
{
  m_filterData.waveFreqFactor = GetRandInRange(minMaxFreq.minVal, minMaxFreq.maxVal);
  m_filterData.waveAmplitude = GetRandInRange(minMaxAmp.minVal, minMaxAmp.maxVal);
}

void FilterControl::SetYOnlyModeSettings()
{
  SetRotate(PROB_HALF);

  m_filterData.yOnlyEffect = static_cast<ZoomFilterData::YOnlyEffect>(
      GetRandInRange(static_cast<uint32_t>(ZoomFilterData::YOnlyEffect::NONE) + 1,
                     static_cast<uint32_t>(ZoomFilterData::YOnlyEffect::_SIZE)));

  m_filterData.yOnlyFreqFactor = GetRandInRange(ZoomFilterData::MIN_Y_ONLY_FREQ_FACTOR,
                                                ZoomFilterData::MAX_Y_ONLY_FREQ_FACTOR);
  m_filterData.yOnlyXFreqFactor = GetRandInRange(ZoomFilterData::MIN_Y_ONLY_X_FREQ_FACTOR,
                                                 ZoomFilterData::MAX_Y_ONLY_X_FREQ_FACTOR);
  m_filterData.yOnlyAmplitude =
      GetRandInRange(ZoomFilterData::MIN_Y_ONLY_AMPLITUDE, ZoomFilterData::MAX_Y_ONLY_AMPLITUDE);

  using EventTypes = FilterControl::FilterEvents::FilterEventTypes;
  if (m_filterEvents->Happens(EventTypes::HYPERCOS_EFFECT))
  {
    SetHypercos1ModeSettings();
  }
}

void FilterControl::SetRotate(const float probability)
{
  if (!ProbabilityOf(probability))
  {
    return;
  }

  m_filterData.rotateSpeed = probability * GetRandInRange(ZoomFilterData::MIN_ROTATE_SPEED,
                                                          ZoomFilterData::MAX_ROTATE_SPEED);
}

void FilterControl::SetHypercosEffect(const MinMaxValues& minMaxFreq, const MinMaxValues& minMaxAmp)
{
  using EventTypes = FilterControl::FilterEvents::FilterEventTypes;

  m_filterData.hypercosEffect = GetRandomHypercosEffect();
  if (m_filterData.hypercosEffect == ZoomFilterData::HypercosEffect::NONE)
  {
    return;
  }

  m_filterData.hypercosFreqX = GetRandInRange(minMaxFreq.minVal, minMaxFreq.maxVal);
  if (m_filterEvents->Happens(EventTypes::HYPERCOS_FREQ_EQUAL))
  {
    m_filterData.hypercosFreqY = m_filterData.hypercosFreqX;
  }
  else
  {
    m_filterData.hypercosFreqY = GetRandInRange(minMaxFreq.minVal, minMaxFreq.maxVal);
  }

  m_filterData.hypercosReverse = m_filterEvents->Happens(EventTypes::HYPERCOS_REVERSE);

  m_filterData.hypercosAmplitudeX = GetRandInRange(minMaxAmp.minVal, minMaxAmp.maxVal);
  if (m_filterEvents->Happens(EventTypes::HYPERCOS_AMP_EQUAL))
  {
    m_filterData.hypercosAmplitudeY = m_filterData.hypercosAmplitudeX;
  }
  else
  {
    m_filterData.hypercosAmplitudeY = GetRandInRange(minMaxAmp.minVal, minMaxAmp.maxVal);
  }
}

inline auto FilterControl::GetRandomHypercosEffect() const -> ZoomFilterData::HypercosEffect
{
  return static_cast<ZoomFilterData::HypercosEffect>(
      GetRandInRange(static_cast<uint32_t>(ZoomFilterData::HypercosEffect::NONE) + 1,
                     static_cast<uint32_t>(ZoomFilterData::HypercosEffect::_SIZE)));
}

void FilterControl::ChangeMilieu()
{
  m_hasChanged = true;

  SetMiddlePoints();
  SetPlaneEffects();
}

void FilterControl::SetMiddlePoints()
{
  if ((m_filterData.mode == ZoomFilterMode::WATER_MODE) ||
      (m_filterData.mode == ZoomFilterMode::Y_ONLY_MODE) ||
      (m_filterData.mode == ZoomFilterMode::AMULET_MODE))
  {
    m_filterData.middleX = m_goomInfo->GetScreenInfo().width / 2;
    m_filterData.middleY = m_goomInfo->GetScreenInfo().height / 2;
    return;
  }

  // clang-format off
  // @formatter:off
  enum class MiddlePointEvents { EVENT1, EVENT2, EVENT3, EVENT4, EVENT5, EVENT6 };
  static const Weights<MiddlePointEvents> s_middlePointWeights{{
     { MiddlePointEvents::EVENT1,  3 },
     { MiddlePointEvents::EVENT2,  2 },
     { MiddlePointEvents::EVENT3,  2 },
     { MiddlePointEvents::EVENT4, 18 },
     { MiddlePointEvents::EVENT5, 10 },
     { MiddlePointEvents::EVENT6, 10 },
  }};
  // @formatter:on
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
    case MiddlePointEvents::EVENT5:
      m_filterData.middleX = m_goomInfo->GetScreenInfo().width / 4;
      m_filterData.middleY = m_goomInfo->GetScreenInfo().height / 4;
      break;
    case MiddlePointEvents::EVENT6:
      m_filterData.middleX = 3 * m_goomInfo->GetScreenInfo().width / 4;
      m_filterData.middleY = 3 * m_goomInfo->GetScreenInfo().height / 4;
      break;
    default:
      throw std::logic_error("Unknown MiddlePointEvents enum.");
  }
}

void FilterControl::SetPlaneEffects()
{
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
  if (m_filterEvents->Happens(FilterEvents::PLANE_AMP_EQUAL))
  {
    m_filterData.vPlaneEffectAmplitude = m_filterData.hPlaneEffectAmplitude;
  }
  else
  {
    m_filterData.vPlaneEffectAmplitude = GetRandInRange(
        ZoomFilterData::MIN_V_PLANE_EFFECT_AMPLITUDE, ZoomFilterData::MAX_V_PLANE_EFFECT_AMPLITUDE);
  }
}

} // namespace GOOM
