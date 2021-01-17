/**
* file: goom_control.c (onetime goom_core.c)
 * author: Jean-Christophe Hoelt (which is not so proud of it)
 *
 * Contains the core of goom's work.
 *
 * (c)2000-2003, by iOS-software.
 */

#include "goom_control.h"

#include "convolve_fx.h"
#include "filters.h"
#include "flying_stars_fx.h"
#include "goom_config.h"
#include "goom_dots_fx.h"
#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/parallel_utils.h"
#include "goomutils/strutils.h"
#include "ifs_dancers_fx.h"
#include "lines_fx.h"
#include "tentacles_fx.h"
#include "text_draw.h"

#include <array>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/unordered_set.hpp>
#include <cereal/types/vector.hpp>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

CEREAL_REGISTER_TYPE(goom::WritablePluginInfo)
CEREAL_REGISTER_POLYMORPHIC_RELATION(goom::PluginInfo, goom::WritablePluginInfo)

// #define SHOW_STATE_TEXT_ON_SCREEN

namespace goom
{

using namespace goom::utils;

enum class GoomDrawable
{
  IFS = 0,
  dots,
  tentacles,
  stars,
  lines,
  scope,
  farScope,
  _size
};

class GoomEvents
{
public:
  GoomEvents() noexcept;
  ~GoomEvents() = default;
  GoomEvents(const GoomEvents&) = delete;
  GoomEvents(const GoomEvents&&) = delete;
  auto operator=(const GoomEvents&) -> GoomEvents& = delete;
  auto operator=(const GoomEvents&&) -> GoomEvents& = delete;

  void SetGoomInfo(PluginInfo* goomInfo);

  enum class GoomEvent
  {
    changeFilterMode = 0,
    changeFilterFromAmuletMode,
    changeState,
    turnOffNoise,
    changeToMegaLentMode,
    changeLineCircleAmplitude,
    changeLineCircleParams,
    changeHLineParams,
    changeVLineParams,
    hypercosEffectOnWithWaveMode,
    waveEffectOnWithWaveMode,
    changeVitesseWithWaveMode,
    waveEffectOnWithCrystalBallMode,
    hypercosEffectOnWithCrystalBallMode,
    hypercosEffectOnWithHyperCos1Mode,
    hypercosEffectOnWithHyperCos2Mode,
    filterReverseOffAndStopSpeed,
    filterReverseOn,
    filterVitesseStopSpeedMinus1,
    filterVitesseStopSpeedPlus1,
    filterZeroHPlaneEffect,
    filterChangeVitesseAndToggleReverse,
    reduceLineMode,
    updateLineMode,
    changeLineToBlack,
    changeGoomLine,
    ifsRenew,
    changeBlockyWavyToOn,
    changeZoomFilterAllowOverexposedToOn,
    allowStrangeWaveValues,
    _size // must be last - gives number of enums
  };

  enum GoomFilterEvent
  {
    waveModeWithHyperCosEffect,
    waveMode,
    crystalBallMode,
    crystalBallModeWithEffects,
    amuletMode,
    waterMode,
    scrunchMode,
    scrunchModeWithEffects,
    hyperCos1Mode,
    hyperCos2Mode,
    yOnlyMode,
    speedwayMode,
    normalMode,
    _size // must be last - gives number of enums
  };

  auto Happens(GoomEvent event) const -> bool;
  auto GetRandomFilterEvent() const -> GoomFilterEvent;
  auto GetRandomLineTypeEvent() const -> LinesFx::LineType;

private:
  PluginInfo* m_goomInfo{};
  mutable GoomFilterEvent m_lastReturnedFilterEvent = GoomFilterEvent::_size;

  static constexpr size_t NUM_GOOM_EVENTS = static_cast<size_t>(GoomEvent::_size);
  static constexpr size_t NUM_GOOM_FILTER_EVENTS = static_cast<size_t>(GoomFilterEvent::_size);

  struct WeightedEvent
  {
    GoomEvent event;
    // m out of n
    uint32_t m;
    uint32_t outOf;
  };
  // clang-format off
  static constexpr std::array<WeightedEvent, NUM_GOOM_EVENTS> WEIGHTED_EVENTS{{
    { .event = GoomEvent::changeFilterMode,                     .m = 1, .outOf =  16 },
    { .event = GoomEvent::changeFilterFromAmuletMode,           .m = 1, .outOf =   5 },
    { .event = GoomEvent::changeState,                          .m = 1, .outOf =   2 },
    { .event = GoomEvent::turnOffNoise,                         .m = 5, .outOf =   5 },
    { .event = GoomEvent::changeToMegaLentMode,                 .m = 1, .outOf = 700 },
    { .event = GoomEvent::changeLineCircleAmplitude,            .m = 1, .outOf =   3 },
    { .event = GoomEvent::changeLineCircleParams,               .m = 1, .outOf =   2 },
    { .event = GoomEvent::changeHLineParams,                    .m = 3, .outOf =   4 },
    { .event = GoomEvent::changeVLineParams,                    .m = 2, .outOf =   3 },
    { .event = GoomEvent::hypercosEffectOnWithWaveMode,         .m = 1, .outOf =   2 },
    { .event = GoomEvent::waveEffectOnWithWaveMode,             .m = 1, .outOf =   3 },
    { .event = GoomEvent::changeVitesseWithWaveMode,            .m = 1, .outOf =   2 },
    { .event = GoomEvent::waveEffectOnWithCrystalBallMode,      .m = 1, .outOf =   4 },
    { .event = GoomEvent::hypercosEffectOnWithCrystalBallMode,  .m = 1, .outOf =   2 },
    { .event = GoomEvent::hypercosEffectOnWithHyperCos1Mode,    .m = 1, .outOf =   3 },
    { .event = GoomEvent::hypercosEffectOnWithHyperCos2Mode,    .m = 1, .outOf =   6 },
    { .event = GoomEvent::filterReverseOffAndStopSpeed,         .m = 1, .outOf =   5 },
    { .event = GoomEvent::filterReverseOn,                      .m = 1, .outOf =  10 },
    { .event = GoomEvent::filterVitesseStopSpeedMinus1,         .m = 1, .outOf =  10 },
    { .event = GoomEvent::filterVitesseStopSpeedPlus1,          .m = 1, .outOf =  12 },
    { .event = GoomEvent::filterZeroHPlaneEffect,               .m = 1, .outOf =   2 },
    { .event = GoomEvent::filterChangeVitesseAndToggleReverse,  .m = 1, .outOf =  40 },
    { .event = GoomEvent::reduceLineMode,                       .m = 1, .outOf =   5 },
    { .event = GoomEvent::updateLineMode,                       .m = 1, .outOf =   4 },
    { .event = GoomEvent::changeLineToBlack,                    .m = 1, .outOf =   2 },
    { .event = GoomEvent::changeGoomLine,                       .m = 1, .outOf =   3 },
    { .event = GoomEvent::ifsRenew,                             .m = 2, .outOf =   3 },
    { .event = GoomEvent::changeBlockyWavyToOn,                 .m = 1, .outOf =  10 },
    { .event = GoomEvent::changeZoomFilterAllowOverexposedToOn, .m = 8, .outOf =  10 },
    { .event = GoomEvent::allowStrangeWaveValues,               .m = 5, .outOf =  10 },
  }};

  static constexpr std::array<std::pair<GoomFilterEvent, size_t>, NUM_GOOM_FILTER_EVENTS> WEIGHTED_FILTER_EVENTS{{
    { GoomFilterEvent::waveModeWithHyperCosEffect,  3 },
    { GoomFilterEvent::waveMode,                    3 },
    { GoomFilterEvent::crystalBallMode,             1 },
    { GoomFilterEvent::crystalBallModeWithEffects,  2 },
    { GoomFilterEvent::amuletMode,                  2 },
    { GoomFilterEvent::waterMode,                   2 },
    { GoomFilterEvent::scrunchMode,                 2 },
    { GoomFilterEvent::scrunchModeWithEffects,      2 },
    { GoomFilterEvent::hyperCos1Mode,               3 },
    { GoomFilterEvent::hyperCos2Mode,               2 },
    { GoomFilterEvent::yOnlyMode,                   3 },
    { GoomFilterEvent::speedwayMode,                3 },
    { GoomFilterEvent::normalMode,                  2 },
  } };

  static constexpr
  std::array<std::pair<LinesFx::LineType, size_t>, LinesFx::NUM_LINE_TYPES> WEIGHTED_LINE_EVENTS{{
    { LinesFx::LineType::circle, 8 },
    { LinesFx::LineType::hline,  2 },
    { LinesFx::LineType::vline,  2 },
  }};
  // clang-format on
  const Weights<GoomFilterEvent> m_filterWeights;
  const Weights<LinesFx::LineType> m_lineTypeWeights;
};

using GoomEvent = GoomEvents::GoomEvent;
using GoomFilterEvent = GoomEvents::GoomFilterEvent;

class GoomStates
{
public:
  using DrawablesState = std::unordered_set<GoomDrawable>;

  GoomStates();

  [[nodiscard]] auto IsCurrentlyDrawable(GoomDrawable drawable) const -> bool;
  [[nodiscard]] auto GetCurrentStateIndex() const -> size_t;
  [[nodiscard]] auto GetCurrentDrawables() const -> DrawablesState;
  [[nodiscard]] auto GetCurrentBuffSettings(GoomDrawable theFx) const -> FXBuffSettings;

  void DoRandomStateChange();

private:
  struct DrawableInfo
  {
    GoomDrawable fx;
    FXBuffSettings buffSettings;
  };
  using DrawableInfoArray = std::vector<DrawableInfo>;
  struct State
  {
    uint32_t weight;
    const DrawableInfoArray drawables;
  };
  using WeightedStatesArray = std::vector<State>;
  static const WeightedStatesArray g_States;
  static auto GetWeightedStates(const WeightedStatesArray& theStates)
      -> std::vector<std::pair<uint16_t, size_t>>;
  const Weights<uint16_t> m_weightedStates;
  size_t m_currentStateIndex{0};
};

// clang-format off
const GoomStates::WeightedStatesArray GoomStates::g_States{{
  {
    .weight = 1,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 1,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 1,
    .drawables {{
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = false } },
  }},
 },
  {
    .weight = 200,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.1, .allowOverexposed = false } },
    }},
  },
  {
    .weight = 200,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = false } },
    }},
  },
  {
    .weight = 200,
    .drawables {{
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.1, .allowOverexposed = false } },
    }},
  },
  {
    .weight = 200,
    .drawables {{
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 100,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = false } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = false } },
    }},
  },
  {
    .weight = 20,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = false } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = false } },
    }},
  },
  {
    .weight = 60,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = false } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 70,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = false } },
    }},
  },
  {
    .weight = 70,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true } },
    }},
  },
  {
    .weight = 40,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = true  } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = false } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 40,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 100,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = true  } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = false } },
    }},
  },
  {
    .weight = 70,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = true  } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 100,
    .drawables {{
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = false } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 60,
    .drawables {{
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = false } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 60,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = false } },
    }},
  },
  {
    .weight = 60,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = true  } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
    }},
  },
}};

  /**
  { .weight =  60, .drawables = { GD::IFS,                                                 GD::scope, GD::farScope }},
  **/
  /**
//  { .weight =  40, .drawables = { GD::IFS,             GD::tentacles,                      GD::scope, GD::farScope }},
  { .weight =  40, .drawables = {                      GD::tentacles,                      GD::scope, GD::farScope }},
  { .weight =  40, .drawables = {                      GD::tentacles,                                 GD::farScope }},
  { .weight =  40, .drawables = {                      GD::tentacles,                      GD::scope               }},
  { .weight =  40, .drawables = {                      GD::tentacles,                                              }},
  **/
// clang-format on

class GoomStats
{
public:
  GoomStats() noexcept = default;
  GoomStats(const GoomStats&) = delete;
  GoomStats(const GoomStats&&) = delete;
  auto operator=(const GoomStats&) -> GoomStats& = delete;
  auto operator=(const GoomStats&&) -> GoomStats& = delete;

  void SetSongTitle(const std::string& songTitle);
  void SetStateStartValue(uint32_t stateIndex);
  void SetZoomFilterStartValue(ZoomFilterMode filterMode);
  void SetSeedStartValue(uint64_t seed);
  void SetStateLastValue(uint32_t stateIndex);
  void SetZoomFilterLastValue(const ZoomFilterData* filterData);
  void SetSeedLastValue(uint64_t seed);
  void SetNumThreadsUsedValue(size_t numThreads);
  void Reset();
  void Log(const StatsLogValueFunc& val) const;
  void UpdateChange(size_t currentState, ZoomFilterMode currentFilterMode);
  void StateChange(uint32_t timeInState);
  void StateChange(size_t index, uint32_t timeInState);
  void FilterModeChange();
  void FilterModeChange(ZoomFilterMode mode);
  void LockChange();
  void DoIfs();
  void DoDots();
  void DoLines();
  void SwitchLines();
  void DoStars();
  void DoTentacles();
  void TentaclesDisabled();
  void DoBigUpdate();
  void LastTimeGoomChange();
  void MegaLentChange();
  void DoNoise();
  void SetLastNoiseFactor(float val);
  void IfsRenew();
  void ChangeLineColor();
  void DoBlockyWavy();
  void DoZoomFilterAllowOverexposed();
  void SetFontFileUsed(const std::string& f);

private:
  std::string m_songTitle{};
  uint32_t m_startingState = 0;
  ZoomFilterMode m_startingFilterMode = ZoomFilterMode::_size;
  uint64_t m_startingSeed = 0;
  uint32_t m_lastState = 0;
  const ZoomFilterData* m_lastZoomFilterData = nullptr;
  uint64_t m_lastSeed = 0;
  size_t m_numThreadsUsed = 0;
  std::string m_fontFileUsed{};

  uint64_t m_totalTimeInUpdatesMs = 0;
  uint32_t m_minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  uint32_t m_maxTimeInUpdatesMs = 0;
  std::chrono::high_resolution_clock::time_point m_timeNowHiRes{};
  size_t m_stateAtMin = 0;
  size_t m_stateAtMax = 0;
  ZoomFilterMode m_filterModeAtMin = ZoomFilterMode::_null;
  ZoomFilterMode m_filterModeAtMax = ZoomFilterMode::_null;

  uint32_t m_numUpdates = 0;
  uint32_t m_totalStateChanges = 0;
  uint64_t m_totalStateDurations = 0;
  uint32_t m_totalFilterModeChanges = 0;
  uint32_t m_numLockChanges = 0;
  uint32_t m_numDoIFS = 0;
  uint32_t m_numDoDots = 0;
  uint32_t m_numDoLines = 0;
  uint32_t m_numDoStars = 0;
  uint32_t m_numDoTentacles = 0;
  uint32_t m_numDisabledTentacles = 0;
  uint32_t m_numBigUpdates = 0;
  uint32_t m_numLastTimeGoomChanges = 0;
  uint32_t m_numMegaLentChanges = 0;
  uint32_t m_numDoNoise = 0;
  uint32_t m_numIfsRenew = 0;
  uint32_t m_numChangeLineColor = 0;
  uint32_t m_numSwitchLines = 0;
  uint32_t m_numBlockyWavy = 0;
  uint32_t m_numZoomFilterAllowOverexposed = 0;
  std::array<uint32_t, static_cast<size_t>(ZoomFilterMode::_size)> m_numFilterModeChanges{0};
  std::vector<uint32_t> m_numStateChanges{};
  std::vector<uint64_t> m_stateDurations{};
};

void GoomStats::Reset()
{
  m_startingState = 0;
  m_startingFilterMode = ZoomFilterMode::_size;
  m_startingSeed = 0;
  m_lastState = 0;
  m_lastZoomFilterData = nullptr;
  m_lastSeed = 0;
  m_numThreadsUsed = 0;

  m_stateAtMin = 0;
  m_stateAtMax = 0;
  m_filterModeAtMin = ZoomFilterMode::_null;
  m_filterModeAtMax = ZoomFilterMode::_null;

  m_numUpdates = 0;
  m_totalTimeInUpdatesMs = 0;
  m_minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  m_maxTimeInUpdatesMs = 0;
  m_timeNowHiRes = std::chrono::high_resolution_clock::now();
  m_totalStateChanges = 0;
  m_totalStateDurations = 0;
  std::fill(m_numStateChanges.begin(), m_numStateChanges.end(), 0);
  std::fill(m_stateDurations.begin(), m_stateDurations.end(), 0);
  m_totalFilterModeChanges = 0;
  m_numFilterModeChanges.fill(0);
  m_numLockChanges = 0;
  m_numDoIFS = 0;
  m_numDoDots = 0;
  m_numDoLines = 0;
  m_numDoStars = 0;
  m_numDoTentacles = 0;
  m_numDisabledTentacles = 0;
  m_numBigUpdates = 0;
  m_numLastTimeGoomChanges = 0;
  m_numMegaLentChanges = 0;
  m_numDoNoise = 0;
  m_numIfsRenew = 0;
  m_numChangeLineColor = 0;
  m_numSwitchLines = 0;
  m_numBlockyWavy = 0;
  m_numZoomFilterAllowOverexposed = 0;
}

void GoomStats::Log(const StatsLogValueFunc& logVal) const
{
  const constexpr char* MODULE = "goom_core";

  logVal(MODULE, "songTitle", m_songTitle);
  logVal(MODULE, "startingState", m_startingState);
  logVal(MODULE, "startingFilterMode", enumToString(m_startingFilterMode));
  logVal(MODULE, "startingSeed", m_startingSeed);
  logVal(MODULE, "lastState", m_lastState);
  logVal(MODULE, "lastSeed", m_lastSeed);
  logVal(MODULE, "numThreadsUsed", m_numThreadsUsed);
  logVal(MODULE, "m_fontFileUsed", m_fontFileUsed);

  if (m_lastZoomFilterData == nullptr)
  {
    logVal(MODULE, "lastZoomFilterData", 0U);
  }
  else
  {
    logVal(MODULE, "lastZoomFilterData->mode", enumToString(m_lastZoomFilterData->mode));
    logVal(MODULE, "lastZoomFilterData->vitesse", m_lastZoomFilterData->vitesse);
    logVal(MODULE, "lastZoomFilterData->pertedec", static_cast<uint32_t>(ZoomFilterData::pertedec));
    logVal(MODULE, "lastZoomFilterData->middleX", m_lastZoomFilterData->middleX);
    logVal(MODULE, "lastZoomFilterData->middleY", m_lastZoomFilterData->middleY);
    logVal(MODULE, "lastZoomFilterData->reverse",
           static_cast<uint32_t>(m_lastZoomFilterData->reverse));
    logVal(MODULE, "lastZoomFilterData->hPlaneEffect", m_lastZoomFilterData->hPlaneEffect);
    logVal(MODULE, "lastZoomFilterData->vPlaneEffect", m_lastZoomFilterData->vPlaneEffect);
    logVal(MODULE, "lastZoomFilterData->waveEffect",
           static_cast<uint32_t>(m_lastZoomFilterData->waveEffect));
    logVal(MODULE, "lastZoomFilterData->hypercosEffect",
           enumToString(m_lastZoomFilterData->hypercosEffect));
    logVal(MODULE, "lastZoomFilterData->noisify",
           static_cast<uint32_t>(m_lastZoomFilterData->noisify));
    logVal(MODULE, "lastZoomFilterData->noiseFactor",
           static_cast<float>(m_lastZoomFilterData->noiseFactor));
    logVal(MODULE, "lastZoomFilterData->blockyWavy",
           static_cast<uint32_t>(m_lastZoomFilterData->blockyWavy));
    logVal(MODULE, "lastZoomFilterData->waveFreqFactor", m_lastZoomFilterData->waveFreqFactor);
    logVal(MODULE, "lastZoomFilterData->waveAmplitude", m_lastZoomFilterData->waveAmplitude);
    logVal(MODULE, "lastZoomFilterData->waveEffectType",
           enumToString(m_lastZoomFilterData->waveEffectType));
    logVal(MODULE, "lastZoomFilterData->scrunchAmplitude", m_lastZoomFilterData->scrunchAmplitude);
    logVal(MODULE, "lastZoomFilterData->speedwayAmplitude",
           m_lastZoomFilterData->speedwayAmplitude);
    logVal(MODULE, "lastZoomFilterData->amuletteAmplitude",
           m_lastZoomFilterData->amuletteAmplitude);
    logVal(MODULE, "lastZoomFilterData->crystalBallAmplitude",
           m_lastZoomFilterData->crystalBallAmplitude);
    logVal(MODULE, "lastZoomFilterData->hypercosFreq", m_lastZoomFilterData->hypercosFreq);
    logVal(MODULE, "lastZoomFilterData->hypercosAmplitude",
           m_lastZoomFilterData->hypercosAmplitude);
    logVal(MODULE, "lastZoomFilterData->hPlaneEffectAmplitude",
           m_lastZoomFilterData->hPlaneEffectAmplitude);
    logVal(MODULE, "lastZoomFilterData->vPlaneEffectAmplitude",
           m_lastZoomFilterData->vPlaneEffectAmplitude);
  }

  logVal(MODULE, "numUpdates", m_numUpdates);
  const int32_t avTimeInUpdateMs =
      std::lround(m_numUpdates == 0 ? -1.0
                                    : static_cast<float>(m_totalTimeInUpdatesMs) /
                                          static_cast<float>(m_numUpdates));
  logVal(MODULE, "avTimeInUpdateMs", avTimeInUpdateMs);
  logVal(MODULE, "minTimeInUpdatesMs", m_minTimeInUpdatesMs);
  logVal(MODULE, "stateAtMin", m_stateAtMin);
  logVal(MODULE, "filterModeAtMin", enumToString(m_filterModeAtMin));
  logVal(MODULE, "maxTimeInUpdatesMs", m_maxTimeInUpdatesMs);
  logVal(MODULE, "stateAtMax", m_stateAtMax);
  logVal(MODULE, "filterModeAtMax", enumToString(m_filterModeAtMax));
  logVal(MODULE, "totalStateChanges", m_totalStateChanges);
  const float avStateDuration =
      m_totalStateChanges == 0
          ? -1.0F
          : static_cast<float>(m_totalStateDurations) / static_cast<float>(m_totalStateChanges);
  logVal(MODULE, "averageStateDuration", avStateDuration);
  for (size_t i = 0; i < m_numStateChanges.size(); i++)
  {
    logVal(MODULE, "numState_" + std::to_string(i) + "_Changes", m_numStateChanges[i]);
  }
  for (size_t i = 0; i < m_stateDurations.size(); i++)
  {
    const float avStateTime =
        m_numStateChanges[i] == 0
            ? -1.0F
            : static_cast<float>(m_stateDurations[i]) / static_cast<float>(m_numStateChanges[i]);
    logVal(MODULE, "averageState_" + std::to_string(i) + "_Duration", avStateTime);
  }
  logVal(MODULE, "totalFilterModeChanges", m_totalFilterModeChanges);
  for (size_t i = 0; i < m_numFilterModeChanges.size(); i++)
  {
    logVal(MODULE, "numFilterMode_" + enumToString(static_cast<ZoomFilterMode>(i)) + "_Changes",
           m_numFilterModeChanges[i]);
  }
  logVal(MODULE, "numLockChanges", m_numLockChanges);
  logVal(MODULE, "numDoIFS", m_numDoIFS);
  logVal(MODULE, "numDoDots", m_numDoDots);
  logVal(MODULE, "numDoLines", m_numDoLines);
  logVal(MODULE, "numDoStars", m_numDoStars);
  logVal(MODULE, "numDoTentacles", m_numDoTentacles);
  logVal(MODULE, "numDisabledTentacles", m_numDisabledTentacles);
  logVal(MODULE, "numLastTimeGoomChanges", m_numLastTimeGoomChanges);
  logVal(MODULE, "numMegaLentChanges", m_numMegaLentChanges);
  logVal(MODULE, "numDoNoise", m_numDoNoise);
  logVal(MODULE, "numIfsRenew", m_numIfsRenew);
  logVal(MODULE, "numChangeLineColor", m_numChangeLineColor);
  logVal(MODULE, "numSwitchLines", m_numSwitchLines);
  logVal(MODULE, "numBlockyWavy", m_numBlockyWavy);
  logVal(MODULE, "numZoomFilterAllowOverexposed", m_numZoomFilterAllowOverexposed);
}

void GoomStats::SetSongTitle(const std::string& s)
{
  m_songTitle = s;
}

void GoomStats::SetStateStartValue(const uint32_t stateIndex)
{
  m_startingState = stateIndex;
}

void GoomStats::SetZoomFilterStartValue(const ZoomFilterMode filterMode)
{
  m_startingFilterMode = filterMode;
}

void GoomStats::SetSeedStartValue(const uint64_t seed)
{
  m_startingSeed = seed;
}

void GoomStats::SetStateLastValue(const uint32_t stateIndex)
{
  m_lastState = stateIndex;
}

void GoomStats::SetZoomFilterLastValue(const ZoomFilterData* filterData)
{
  m_lastZoomFilterData = filterData;
}

void GoomStats::SetSeedLastValue(const uint64_t seed)
{
  m_lastSeed = seed;
}

void GoomStats::SetNumThreadsUsedValue(const size_t n)
{
  m_numThreadsUsed = n;
}

void GoomStats::SetFontFileUsed(const std::string& f)
{
  m_fontFileUsed = f;
}

inline void GoomStats::UpdateChange(const size_t currentState,
                                    const ZoomFilterMode currentFilterMode)
{
  const auto timeNow = std::chrono::high_resolution_clock::now();
  if (m_numUpdates > 0)
  {
    using ms = std::chrono::milliseconds;
    const ms diff = std::chrono::duration_cast<ms>(timeNow - m_timeNowHiRes);
    const auto timeInUpdateMs = static_cast<uint32_t>(diff.count());
    if (timeInUpdateMs < m_minTimeInUpdatesMs)
    {
      m_minTimeInUpdatesMs = timeInUpdateMs;
      m_stateAtMin = currentState;
      m_filterModeAtMin = currentFilterMode;
    }
    else if (timeInUpdateMs > m_maxTimeInUpdatesMs)
    {
      m_maxTimeInUpdatesMs = timeInUpdateMs;
      m_stateAtMax = currentState;
      m_filterModeAtMax = currentFilterMode;
    }
    m_totalTimeInUpdatesMs += timeInUpdateMs;
  }
  m_timeNowHiRes = timeNow;

  m_numUpdates++;
}

inline void GoomStats::StateChange(const uint32_t timeInState)
{
  m_totalStateChanges++;
  m_totalStateDurations += timeInState;
}

inline void GoomStats::StateChange(const size_t index, const uint32_t timeInState)
{
  if (index >= m_numStateChanges.size())
  {
    for (size_t i = m_numStateChanges.size(); i <= index; i++)
    {
      m_numStateChanges.push_back(0);
    }
  }
  m_numStateChanges[index]++;

  if (index >= m_stateDurations.size())
  {
    for (size_t i = m_stateDurations.size(); i <= index; i++)
    {
      m_stateDurations.push_back(0);
    }
  }
  m_stateDurations[index] += timeInState;
}

inline void GoomStats::FilterModeChange()
{
  m_totalFilterModeChanges++;
}

inline void GoomStats::FilterModeChange(const ZoomFilterMode mode)
{
  m_numFilterModeChanges.at(static_cast<size_t>(mode))++;
}

inline void GoomStats::LockChange()
{
  m_numLockChanges++;
}

inline void GoomStats::DoIfs()
{
  m_numDoIFS++;
}

inline void GoomStats::DoDots()
{
  m_numDoDots++;
}

inline void GoomStats::DoLines()
{
  m_numDoLines++;
}

inline void GoomStats::DoStars()
{
  m_numDoStars++;
}

inline void GoomStats::DoTentacles()
{
  m_numDoTentacles++;
}

inline void GoomStats::TentaclesDisabled()
{
  m_numDisabledTentacles++;
}

inline void GoomStats::DoBigUpdate()
{
  m_numBigUpdates++;
}

inline void GoomStats::LastTimeGoomChange()
{
  m_numLastTimeGoomChanges++;
}

inline void GoomStats::MegaLentChange()
{
  m_numMegaLentChanges++;
}

inline void GoomStats::DoNoise()
{
  m_numDoNoise++;
}

inline void GoomStats::IfsRenew()
{
  m_numIfsRenew++;
}

inline void GoomStats::ChangeLineColor()
{
  m_numChangeLineColor++;
}

inline void GoomStats::SwitchLines()
{
  m_numSwitchLines++;
}

inline void GoomStats::DoBlockyWavy()
{
  m_numBlockyWavy++;
}

inline void GoomStats::DoZoomFilterAllowOverexposed()
{
  m_numZoomFilterAllowOverexposed++;
}

constexpr int32_t STOP_SPEED = 128;
// TODO: put that as variable in PluginInfo
constexpr int32_t TIME_BETWEEN_CHANGE = 300;

struct LogStatsVisitor
{
  LogStatsVisitor(const std::string& m, const std::string& n) : module{m}, name{n} {}
  const std::string& module;
  const std::string& name;
  void operator()(const std::string& s) const { logInfo("{}.{} = '{}'", module, name, s); }
  void operator()(const uint32_t i) const { logInfo("{}.{} = {}", module, name, i); }
  void operator()(const int32_t i) const { logInfo("{}.{} = {}", module, name, i); }
  void operator()(const uint64_t i) const { logInfo("{}.{} = {}", module, name, i); }
  void operator()(const float f) const { logInfo("{}.{} = {:.3}", module, name, f); }
};


class GoomImageBuffers
{
public:
  GoomImageBuffers() noexcept = default;
  explicit GoomImageBuffers(uint32_t resx, uint32_t resy) noexcept;
  ~GoomImageBuffers() noexcept;
  GoomImageBuffers(const GoomImageBuffers& val) = delete;
  GoomImageBuffers(const GoomImageBuffers&& val) = delete;
  auto operator=(const GoomImageBuffers&) -> GoomImageBuffers& = delete;
  auto operator=(const GoomImageBuffers&&) -> GoomImageBuffers& = delete;

  void SetResolution(uint32_t resx, uint32_t resy);

  [[nodiscard]] auto GetP1() const -> PixelBuffer& { return *m_p1; }
  [[nodiscard]] auto GetP2() const -> PixelBuffer& { return *m_p2; }

  [[nodiscard]] auto GetOutputBuff() const -> PixelBuffer& { return *m_outputBuff; }
  void SetOutputBuff(PixelBuffer& val) { m_outputBuff = &val; }

  static constexpr size_t MAX_NUM_BUFFS = 10;
  static constexpr size_t m_maxBuffInc = MAX_NUM_BUFFS / 2;
  void SetBuffInc(size_t i);
  void RotateBuffers();

private:
  std::vector<std::unique_ptr<PixelBuffer>> buffs{};
  PixelBuffer* m_p1{};
  PixelBuffer* m_p2{};
  PixelBuffer* m_outputBuff{};
  size_t m_nextBuff = 0;
  size_t m_buffInc = 1;
  static auto GetPixelBuffs(uint32_t resx, uint32_t resy)
      -> std::vector<std::unique_ptr<PixelBuffer>>;
};

auto GoomImageBuffers::GetPixelBuffs(const uint32_t resx, const uint32_t resy)
    -> std::vector<std::unique_ptr<PixelBuffer>>
{
  std::vector<std::unique_ptr<PixelBuffer>> newBuffs(MAX_NUM_BUFFS);
  for (auto& b : newBuffs)
  {
    // Allocate one extra row and column to help the filter process handle edges.
    b = std::make_unique<PixelBuffer>(resx + 1, resy + 1);
  }
  return newBuffs;
}

GoomImageBuffers::GoomImageBuffers(const uint32_t resx, const uint32_t resy) noexcept
  : buffs{GetPixelBuffs(resx, resy)},
    m_p1{buffs[0].get()},
    m_p2{buffs[1].get()},
    m_nextBuff{MAX_NUM_BUFFS == 2 ? 0 : 2}
{
}

GoomImageBuffers::~GoomImageBuffers() noexcept = default;

void GoomImageBuffers::SetResolution(const uint32_t resx, const uint32_t resy)
{
  buffs = GetPixelBuffs(resx, resy);
  m_p1 = buffs[0].get();
  m_p2 = buffs[1].get();
  m_nextBuff = MAX_NUM_BUFFS == 2 ? 0 : 2;
  m_buffInc = 1;
}

void GoomImageBuffers::SetBuffInc(const size_t i)
{
  if (i < 1 || i > m_maxBuffInc)
  {
    throw std::logic_error("Cannot set buff inc: i out of range.");
  }
  m_buffInc = i;
}

void GoomImageBuffers::RotateBuffers()
{
  m_p1 = m_p2;
  m_p2 = buffs[m_nextBuff].get();
  m_nextBuff += +m_buffInc;
  if (m_nextBuff >= buffs.size())
  {
    m_nextBuff = 0;
  }
}

struct GoomMessage
{
  std::string message;
  uint32_t numberOfLinesInMessage = 0;
  uint32_t affiche = 0;

  auto operator==(const GoomMessage&) const -> bool = default;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(CEREAL_NVP(message), CEREAL_NVP(numberOfLinesInMessage), CEREAL_NVP(affiche));
  }
};

struct GoomVisualFx
{
  GoomVisualFx() noexcept = default;
  explicit GoomVisualFx(Parallel& p, const std::shared_ptr<const PluginInfo>& goomInfo) noexcept;

  std::shared_ptr<ConvolveFx> convolve_fx{};
  std::shared_ptr<ZoomFilterFx> zoomFilter_fx{};
  std::shared_ptr<FlyingStarsFx> star_fx{};
  std::shared_ptr<GoomDotsFx> goomDots_fx{};
  std::shared_ptr<IfsDancersFx> ifs_fx{};
  std::shared_ptr<TentaclesFx> tentacles_fx{};

  std::vector<std::shared_ptr<IVisualFx>> list{};

  auto operator==(const GoomVisualFx& f) const -> bool;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(CEREAL_NVP(zoomFilter_fx), CEREAL_NVP(ifs_fx), CEREAL_NVP(star_fx), CEREAL_NVP(convolve_fx),
       CEREAL_NVP(tentacles_fx), CEREAL_NVP(goomDots_fx), CEREAL_NVP(list));
  }
};

GoomVisualFx::GoomVisualFx(Parallel& p, const std::shared_ptr<const PluginInfo>& goomInfo) noexcept
  : convolve_fx{new ConvolveFx{p, goomInfo}},
    zoomFilter_fx{new ZoomFilterFx{p, goomInfo}},
    star_fx{new FlyingStarsFx{goomInfo}},
    goomDots_fx{new GoomDotsFx{goomInfo}},
    ifs_fx{new IfsDancersFx{goomInfo}},
    tentacles_fx{new TentaclesFx{goomInfo}},
    // clang-format off
    list{
      convolve_fx,
      zoomFilter_fx,
      star_fx,
      ifs_fx,
      goomDots_fx,
      tentacles_fx,
    }
// clang-format on
{
}

auto GoomVisualFx::operator==(const GoomVisualFx& f) const -> bool
{
  bool result = *convolve_fx == *f.convolve_fx && *zoomFilter_fx == *f.zoomFilter_fx &&
                *star_fx == *f.star_fx && *goomDots_fx == *f.goomDots_fx && *ifs_fx == *f.ifs_fx &&
                *tentacles_fx == *f.tentacles_fx;

  if (!result)
  {
    logDebug("result == {}", result);
    logDebug("convolve_fx == f.convolve_fx = {}", *convolve_fx == *f.convolve_fx);
    logDebug("zoomFilter_fx == f.zoomFilter_fx = {}", *zoomFilter_fx == *f.zoomFilter_fx);
    logDebug("star_fx == f.star_fx = {}", *star_fx == *f.star_fx);
    logDebug("goomDots_fx == f.goomDots_fx = {}", *goomDots_fx == *f.goomDots_fx);
    logDebug("ifs_fx == f.ifs_fx = {}", *ifs_fx == *f.ifs_fx);
    logDebug("tentacles_fx == f.tentacles_fx = {}", *tentacles_fx == *f.tentacles_fx);
    return result;
  }

  for (size_t i = 0; i < list.size(); i++)
  {
    if (typeid(*list[i]) != typeid(*f.list[i]))
    {
      logDebug("lists not same type at index {}.", i);
      return false;
    }
    if (list[i]->getFxName() != f.list[i]->getFxName())
    {
      logDebug("list {} not same name as list {}.", list[i]->getFxName(), f.list[i]->getFxName());
      return false;
    }
  }
  return true;
}

struct GoomData
{
  ZoomFilterData zoomFilterData{};

  int lockVar = 0; // pour empecher de nouveaux changements
  int stopLines = 0;
  int cyclesSinceLastChange = 0; // nombre de Cycle Depuis Dernier Changement
  int drawLinesDuration = 80; // duree de la transition entre afficher les lignes ou pas
  int lineMode = 80; // l'effet lineaire a dessiner

  static constexpr float switchMultAmount = 29.0 / 30.0;
  float switchMult = switchMultAmount;
  static constexpr int switchIncrAmount = 0x7f;
  int switchIncr = switchIncrAmount;
  uint32_t stateSelectionBlocker = 0;
  int32_t previousZoomSpeed = 128;

  static constexpr int maxTitleDisplayTime = 200;
  static constexpr int timeToSpaceTitleDisplay = 100;
  static constexpr int timeToFadeTitleDisplay = 50;
  int timeOfTitleDisplay = 0;
  std::string title{};

  auto operator==(const GoomData& d) const -> bool;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(CEREAL_NVP(lockVar), CEREAL_NVP(stopLines), CEREAL_NVP(cyclesSinceLastChange),
       CEREAL_NVP(drawLinesDuration), CEREAL_NVP(lineMode), CEREAL_NVP(switchMult),
       CEREAL_NVP(switchIncr), CEREAL_NVP(stateSelectionBlocker), CEREAL_NVP(previousZoomSpeed),
       CEREAL_NVP(timeOfTitleDisplay), CEREAL_NVP(title), CEREAL_NVP(zoomFilterData));
  }
};

auto GoomData::operator==(const GoomData& d) const -> bool
{
  return lockVar == d.lockVar && stopLines == d.stopLines &&
         cyclesSinceLastChange == d.cyclesSinceLastChange &&
         drawLinesDuration == d.drawLinesDuration && lineMode == d.lineMode &&
         switchMult == d.switchMult && switchIncr == d.switchIncr &&
         stateSelectionBlocker == d.stateSelectionBlocker &&
         previousZoomSpeed == d.previousZoomSpeed && timeOfTitleDisplay == d.timeOfTitleDisplay &&
         title == d.title && zoomFilterData == d.zoomFilterData;
}

class GoomControl::GoomControlImpl
{
public:
  GoomControlImpl() noexcept;
  GoomControlImpl(uint32_t screenWidth, uint32_t screenHeight) noexcept;
  ~GoomControlImpl() noexcept;
  GoomControlImpl(const GoomControlImpl&) noexcept = delete;
  GoomControlImpl(const GoomControlImpl&&) noexcept = delete;
  auto operator=(const GoomControlImpl&) noexcept -> GoomControlImpl& = delete;
  auto operator=(const GoomControlImpl&&) noexcept -> GoomControlImpl& = delete;

  void Swap(GoomControl::GoomControlImpl& other) noexcept = delete;

  auto GetScreenBuffer() const -> PixelBuffer&;
  void SetScreenBuffer(PixelBuffer& buff);
  void SetFontFile(const std::string& f);

  auto GetScreenWidth() const -> uint32_t;
  auto GetScreenHeight() const -> uint32_t;

  void Start();
  void Finish();

  void Update(const AudioSamples& soundData,
              int forceMode,
              float fps,
              const char* songTitle,
              const char* message);

  auto operator==(const GoomControlImpl& c) const -> bool;

private:
  Parallel m_parallel;
  std::shared_ptr<WritablePluginInfo> m_goomInfo{};
  GoomImageBuffers m_imageBuffers{};
  GoomVisualFx m_visualFx{};
  GoomStats m_stats{};
  GoomStates m_states{};
  GoomEvents m_goomEvent{};
  uint32_t m_timeInState = 0;
  uint32_t m_cycle = 0;
  std::unordered_set<GoomDrawable> m_curGDrawables{};
  GoomMessage m_messageData{};
  GoomData m_goomData{};
  GoomDraw m_draw{};
  TextDraw m_text{};

  // Line Fx
  LinesFx m_gmline1{};
  LinesFx m_gmline2{};

  auto ChangeFilterModeEventHappens() -> bool;
  void SetNextFilterMode();
  void ChangeState();
  void ChangeFilterMode();
  void ChangeMilieu();
  void BigNormalUpdate(ZoomFilterData** pzfd);
  void MegaLentUpdate(ZoomFilterData** pzfd);

  // baisser regulierement la vitesse
  void RegularlyLowerTheSpeed(ZoomFilterData** pzfd);
  void LowerSpeed(ZoomFilterData** pzfd);

  // on verifie qu'il ne se pas un truc interressant avec le son.
  void ChangeFilterModeIfMusicChanges(int forceMode);

  // Changement d'effet de zoom !
  void ChangeZoomEffect(ZoomFilterData* pzfd, int forceMode);

  void ApplyIfsIfRequired();
  void ApplyTentaclesIfRequired();
  void ApplyStarsIfRequired();

  void DisplayText(const char* songTitle, const char* message, float fps);

#ifdef SHOW_STATE_TEXT_ON_SCREEN
  void displayStateText();
#endif

  void ApplyDotsIfRequired();

  void ChooseGoomLine(float* param1,
                      float* param2,
                      Pixel* couleur,
                      LinesFx::LineType* mode,
                      float* amplitude,
                      int far);

  // si on est dans un goom : afficher les lignes
  void DisplayLinesIfInAGoom(const AudioSamples& soundData);
  void DisplayLines(const AudioSamples& soundData);

  // arret demande
  void StopRequest();
  void StopIfRequested();

  // arret aleatore.. changement de mode de ligne..
  void StopRandomLineChangeMode();

  // Permet de forcer un effet.
  void ForceFilterModeIfSet(ZoomFilterData** pzfd, int forceMode);
  void ForceFilterMode(int forceMode, ZoomFilterData** pzfd);

  // arreter de decrémenter au bout d'un certain temps
  void StopDecrementingAfterAWhile(ZoomFilterData** pzfd);
  void StopDecrementing(ZoomFilterData** pzfd);

  // tout ceci ne sera fait qu'en cas de non-blocage
  void BigUpdateIfNotLocked(ZoomFilterData** pzfd);
  void BigUpdate(ZoomFilterData** pzfd);

  // gros frein si la musique est calme
  void BigBreakIfMusicIsCalm(ZoomFilterData** pzfd);
  void BigBreak(ZoomFilterData** pzfd);

  void UpdateMessage(const char* message);
  void DrawText(const std::string&, int xPos, int yPos, float spacing, PixelBuffer&);

  friend class cereal::access;
  template<class Archive>
  void save(Archive& ar) const;
  template<class Archive>
  void load(Archive& ar);
};

auto GoomControl::GetRandSeed() -> uint64_t
{
  return goom::utils::getRandSeed();
}

void GoomControl::SetRandSeed(const uint64_t seed)
{
  logDebug("Set goom seed = {}.", seed);
  goom::utils::setRandSeed(seed);
}

GoomControl::GoomControl() noexcept : m_controller{new GoomControlImpl{}}
{
}

GoomControl::GoomControl(const uint32_t resx, const uint32_t resy) noexcept
  : m_controller{new GoomControlImpl{resx, resy}}
{
}

GoomControl::~GoomControl() noexcept = default;

auto GoomControl::operator==(const GoomControl& c) const -> bool
{
  return m_controller->operator==(*c.m_controller);
}

void GoomControl::SaveState(std::ostream& f) const
{
  cereal::JSONOutputArchive archive(f);
  archive(*this);
}

void GoomControl::RestoreState(std::istream& f)
{
  PixelBuffer& outputBuffer = m_controller->GetScreenBuffer();
  cereal::JSONInputArchive archive(f);
  archive(*this);
  m_controller->SetScreenBuffer(outputBuffer);
}

void GoomControl::SetScreenBuffer(PixelBuffer& buffer)
{
  m_controller->SetScreenBuffer(buffer);
}

void GoomControl::SetFontFile(const std::string& filename)
{
  m_controller->SetFontFile(filename);
}

void GoomControl::Start()
{
  m_controller->Start();
}

void GoomControl::Finish()
{
  m_controller->Finish();
}

void GoomControl::Update(const AudioSamples& soundData,
                         const int forceMode,
                         const float fps,
                         const char* songTitle,
                         const char* message)
{
  m_controller->Update(soundData, forceMode, fps, songTitle, message);
}

template<class Archive>
void GoomControl::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_controller));
}

// Need to explicitly instantiate template functions for serialization.
template void GoomControl::serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);
template void GoomControl::serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template void GoomControl::GoomControlImpl::save<cereal::JSONOutputArchive>(
    cereal::JSONOutputArchive&) const;
template void GoomControl::GoomControlImpl::load<cereal::JSONInputArchive>(
    cereal::JSONInputArchive&);

template<class Archive>
void GoomControl::GoomControlImpl::save(Archive& ar) const
{
  ar(CEREAL_NVP(m_goomInfo), CEREAL_NVP(m_timeInState), CEREAL_NVP(m_cycle), CEREAL_NVP(m_visualFx),
     CEREAL_NVP(m_curGDrawables), CEREAL_NVP(m_messageData), CEREAL_NVP(m_goomData),
     CEREAL_NVP(m_gmline1), CEREAL_NVP(m_gmline2));
}

template<class Archive>
void GoomControl::GoomControlImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(m_goomInfo), CEREAL_NVP(m_timeInState), CEREAL_NVP(m_cycle), CEREAL_NVP(m_visualFx),
     CEREAL_NVP(m_curGDrawables), CEREAL_NVP(m_messageData), CEREAL_NVP(m_goomData),
     CEREAL_NVP(m_gmline1), CEREAL_NVP(m_gmline2));

  m_imageBuffers.SetResolution(m_goomInfo->GetScreenInfo().width,
                               m_goomInfo->GetScreenInfo().height);
}

auto GoomControl::GoomControlImpl::operator==(const GoomControlImpl& c) const -> bool
{
  if (m_goomInfo == nullptr && c.m_goomInfo != nullptr)
  {
    return false;
  }
  if (m_goomInfo != nullptr && c.m_goomInfo == nullptr)
  {
    return false;
  }

  bool result =
      ((m_goomInfo == nullptr && c.m_goomInfo == nullptr) || (*m_goomInfo == *c.m_goomInfo)) &&
      m_timeInState == c.m_timeInState && m_cycle == c.m_cycle && m_visualFx == c.m_visualFx &&
      m_curGDrawables == c.m_curGDrawables && m_messageData == c.m_messageData &&
      m_goomData == c.m_goomData && m_gmline1 == c.m_gmline1 && m_gmline2 == c.m_gmline2;

  if (!result)
  {
    logDebug("result == {}", result);
    logDebug("goomInfo->getScreenInfo().width = {}", m_goomInfo->GetScreenInfo().width);
    logDebug("c.goomInfo->getScreenInfo().width = {}", c.m_goomInfo->GetScreenInfo().width);
    logDebug("goomInfo->getScreenInfo().height = {}", m_goomInfo->GetScreenInfo().height);
    logDebug("c.goomInfo->getScreenInfo().height = {}", c.m_goomInfo->GetScreenInfo().height);
    logDebug("timeInState = {}, c.timeInState = {}", m_timeInState, c.m_timeInState);
    logDebug("cycle = {}, c.cycle = {}", m_cycle, c.m_cycle);
    logDebug("visualFx == c.visualFx = {}", m_visualFx == c.m_visualFx);
    logDebug("curGDrawables == c.curGDrawables = {}", m_curGDrawables == c.m_curGDrawables);
    logDebug("messageData == c.messageData = {}", m_messageData == c.m_messageData);
    logDebug("goomData == c.goomData = {}", m_goomData == c.m_goomData);
    logDebug("gmline1 == c.gmline1 = {}", m_gmline1 == c.m_gmline1);
    logDebug("gmline2 == c.gmline2 = {}", m_gmline2 == c.m_gmline2);
  }
  return result;
}

static const Pixel lRed = GetRedLineColor();
static const Pixel lGreen = GetGreenLineColor();
static const Pixel lBlack = GetBlackLineColor();

GoomControl::GoomControlImpl::GoomControlImpl() noexcept : m_parallel{}
{
}

GoomControl::GoomControlImpl::GoomControlImpl(const uint32_t screenWidth,
                                              const uint32_t screenHeight) noexcept
  : m_parallel{-1}, // max cores - 1
    m_goomInfo{new WritablePluginInfo{screenWidth, screenHeight}},
    m_imageBuffers{screenWidth, screenHeight},
    m_visualFx{m_parallel, std::const_pointer_cast<const PluginInfo>(
                               std::dynamic_pointer_cast<PluginInfo>(m_goomInfo))},
    m_draw{screenWidth, screenHeight},
    m_text{screenWidth, screenHeight},
    m_gmline1{std::const_pointer_cast<const PluginInfo>(
                  std::dynamic_pointer_cast<PluginInfo>(m_goomInfo)),
              LinesFx::LineType::hline,
              static_cast<float>(screenHeight),
              lBlack,
              LinesFx::LineType::circle,
              0.4f * static_cast<float>(screenHeight),
              lGreen},
    m_gmline2{std::const_pointer_cast<const PluginInfo>(
                  std::dynamic_pointer_cast<PluginInfo>(m_goomInfo)),
              LinesFx::LineType::hline,
              0,
              lBlack,
              LinesFx::LineType::circle,
              0.2F * static_cast<float>(screenHeight),
              lRed}
{
  logDebug("Initialize goom: screenWidth = {}, screenHeight = {}.", screenWidth, screenHeight);
}

GoomControl::GoomControlImpl::~GoomControlImpl() noexcept = default;

auto GoomControl::GoomControlImpl::GetScreenBuffer() const -> PixelBuffer&
{
  return m_imageBuffers.GetOutputBuff();
}

void GoomControl::GoomControlImpl::SetScreenBuffer(PixelBuffer& buffer)
{
  m_imageBuffers.SetOutputBuff(buffer);
}

void GoomControl::GoomControlImpl::SetFontFile(const std::string& filename)
{
  m_stats.SetFontFileUsed(filename);

  m_text.SetFontFile(filename);
  m_text.SetFontSize(30);
  m_text.SetOutlineWidth(1);
  m_text.SetAlignment(TextDraw::TextAlignment::left);
}

auto GoomControl::GoomControlImpl::GetScreenWidth() const -> uint32_t
{
  return m_goomInfo->GetScreenInfo().width;
}

auto GoomControl::GoomControlImpl::GetScreenHeight() const -> uint32_t
{
  return m_goomInfo->GetScreenInfo().height;
}

inline auto GoomControl::GoomControlImpl::ChangeFilterModeEventHappens() -> bool
{
  // If we're in amulette mode and the state contains tentacles,
  // then get out with a different probability.
  // (Rationale: get tentacles going earlier with another mode.)
  if ((m_goomData.zoomFilterData.mode == ZoomFilterMode::amuletMode) &&
      m_states.GetCurrentDrawables().contains(GoomDrawable::tentacles))
  {
    return m_goomEvent.Happens(GoomEvent::changeFilterFromAmuletMode);
  }

  return m_goomEvent.Happens(GoomEvent::changeFilterMode);
}

void GoomControl::GoomControlImpl::Start()
{
  m_timeInState = 0;
  ChangeState();
  ChangeFilterMode();
  m_goomEvent.SetGoomInfo(m_goomInfo.get());

  m_curGDrawables = m_states.GetCurrentDrawables();
  SetNextFilterMode();
  m_goomData.zoomFilterData.middleX = GetScreenWidth();
  m_goomData.zoomFilterData.middleY = GetScreenHeight();

  m_stats.Reset();
  m_stats.SetStateStartValue(m_states.GetCurrentStateIndex());
  m_stats.SetZoomFilterStartValue(m_goomData.zoomFilterData.mode);
  m_stats.SetSeedStartValue(GetRandSeed());

  for (auto& v : m_visualFx.list)
  {
    v->start();
  }
}

static void LogStatsValue(const std::string& module,
                          const std::string& name,
                          const StatsLogValue& value)
{
  std::visit(LogStatsVisitor{module, name}, value);
}

void GoomControl::GoomControlImpl::Finish()
{
  for (auto& v : m_visualFx.list)
  {
    v->finish();
  }

  for (auto& v : m_visualFx.list)
  {
    v->log(LogStatsValue);
  }

  m_stats.SetStateLastValue(m_states.GetCurrentStateIndex());
  m_stats.SetZoomFilterLastValue(&m_goomData.zoomFilterData);
  m_stats.SetSeedLastValue(GetRandSeed());
  m_stats.SetNumThreadsUsedValue(m_parallel.getNumThreadsUsed());

  m_stats.Log(LogStatsValue);
}

void GoomControl::GoomControlImpl::Update(const AudioSamples& soundData,
                                          const int forceMode,
                                          const float fps,
                                          const char* songTitle,
                                          const char* message)
{
  m_stats.UpdateChange(m_states.GetCurrentStateIndex(), m_goomData.zoomFilterData.mode);

  m_timeInState++;

  // elargissement de l'intervalle d'évolution des points
  // ! calcul du deplacement des petits points ...

  logDebug("sound getTimeSinceLastGoom() = {}", m_goomInfo->GetSoundInfo().getTimeSinceLastGoom());

  /* ! etude du signal ... */
  m_goomInfo->ProcessSoundSample(soundData);

  // applyIfsIfRequired();

  ApplyDotsIfRequired();

  /* par défaut pas de changement de zoom */
  if (forceMode != 0)
  {
    logDebug("forceMode = {}\n", forceMode);
  }
  m_goomData.lockVar--;
  if (m_goomData.lockVar < 0)
  {
    m_goomData.lockVar = 0;
  }
  m_stats.LockChange();
  /* note pour ceux qui n'ont pas suivis : le lockVar permet d'empecher un */
  /* changement d'etat du plugin juste apres un autre changement d'etat. oki */
  // -- Note for those who have not followed: the lockVar prevents a change
  // of state of the plugin just after another change of state.

  ChangeFilterModeIfMusicChanges(forceMode);

  ZoomFilterData* pzfd{};

  BigUpdateIfNotLocked(&pzfd);

  BigBreakIfMusicIsCalm(&pzfd);

  RegularlyLowerTheSpeed(&pzfd);

  StopDecrementingAfterAWhile(&pzfd);

  ForceFilterModeIfSet(&pzfd, forceMode);

  ChangeZoomEffect(pzfd, forceMode);

  // Zoom here!
  m_visualFx.zoomFilter_fx->ZoomFilterFastRgb(m_imageBuffers.GetP1(), m_imageBuffers.GetP2(), pzfd,
                                              m_goomData.switchIncr, m_goomData.switchMult);

  // applyDotsIfRequired();
  ApplyIfsIfRequired();
  ApplyTentaclesIfRequired();
  ApplyStarsIfRequired();

  /**
#ifdef SHOW_STATE_TEXT_ON_SCREEN
  displayStateText();
#endif
  displayText(songTitle, message, fps);
**/

  // Gestion du Scope - Scope management
  StopIfRequested();
  StopRandomLineChangeMode();
  DisplayLinesIfInAGoom(soundData);

  // affichage et swappage des buffers...
  m_visualFx.convolve_fx->Convolve(m_imageBuffers.GetP2(), m_imageBuffers.GetOutputBuff());

  m_imageBuffers.SetBuffInc(getRandInRange(1U, GoomImageBuffers::m_maxBuffInc + 1));
  m_imageBuffers.RotateBuffers();

#ifdef SHOW_STATE_TEXT_ON_SCREEN
  DisplayStateText();
#endif
  DisplayText(songTitle, message, fps);

  m_cycle++;

  logDebug("About to return.");
}

void GoomControl::GoomControlImpl::ChooseGoomLine(float* param1,
                                                  float* param2,
                                                  Pixel* couleur,
                                                  LinesFx::LineType* mode,
                                                  float* amplitude,
                                                  const int far)
{
  *amplitude = 1.0F;
  *mode = m_goomEvent.GetRandomLineTypeEvent();

  switch (*mode)
  {
    case LinesFx::LineType::circle:
      if (far)
      {
        *param1 = *param2 = 0.47F;
        *amplitude = 0.8F;
        break;
      }
      if (m_goomEvent.Happens(GoomEvent::changeLineCircleAmplitude))
      {
        *param1 = *param2 = 0;
        *amplitude = 3.0F;
      }
      else if (m_goomEvent.Happens(GoomEvent::changeLineCircleParams))
      {
        *param1 = 0.40F * GetScreenHeight();
        *param2 = 0.22F * GetScreenHeight();
      }
      else
      {
        *param1 = *param2 = GetScreenHeight() * 0.35F;
      }
      break;
    case LinesFx::LineType::hline:
      if (m_goomEvent.Happens(GoomEvent::changeHLineParams) || far)
      {
        *param1 = GetScreenHeight() / 7.0F;
        *param2 = 6.0F * GetScreenHeight() / 7.0F;
      }
      else
      {
        *param1 = *param2 = GetScreenHeight() / 2.0F;
        *amplitude = 2.0F;
      }
      break;
    case LinesFx::LineType::vline:
      if (m_goomEvent.Happens(GoomEvent::changeVLineParams) || far)
      {
        *param1 = GetScreenWidth() / 7.0F;
        *param2 = 6.0F * GetScreenWidth() / 7.0F;
      }
      else
      {
        *param1 = *param2 = GetScreenWidth() / 2.0F;
        *amplitude = 1.5F;
      }
      break;
    default:
      throw std::logic_error("Unknown LineTypes enum.");
  }

  m_stats.ChangeLineColor();
  *couleur = m_gmline1.GetRandomLineColor();
}

void GoomControl::GoomControlImpl::ChangeFilterModeIfMusicChanges(const int forceMode)
{
  logDebug("forceMode = {}", forceMode);
  if (forceMode == -1)
  {
    return;
  }

  logDebug("sound getTimeSinceLastGoom() = {}, goomData.cyclesSinceLastChange = {}",
           m_goomInfo->GetSoundInfo().getTimeSinceLastGoom(), m_goomData.cyclesSinceLastChange);
  if ((m_goomInfo->GetSoundInfo().getTimeSinceLastGoom() == 0) ||
      (m_goomData.cyclesSinceLastChange > TIME_BETWEEN_CHANGE) || (forceMode > 0))
  {
    logDebug("Try to change the filter mode.");
    if (ChangeFilterModeEventHappens())
    {
      ChangeFilterMode();
    }
  }
  logDebug("sound getTimeSinceLastGoom() = {}", m_goomInfo->GetSoundInfo().getTimeSinceLastGoom());
}

inline auto GetHypercosEffect(const bool active) -> ZoomFilterData::HypercosEffect
{
  if (!active)
  {
    return ZoomFilterData::HypercosEffect::none;
  }

  return static_cast<ZoomFilterData::HypercosEffect>(
      getRandInRange(static_cast<uint32_t>(ZoomFilterData::HypercosEffect::none) + 1,
                     static_cast<uint32_t>(ZoomFilterData::HypercosEffect::_size)));
}

void GoomControl::GoomControlImpl::SetNextFilterMode()
{
  //  goomData.zoomFilterData.vitesse = 127;
  //  goomData.zoomFilterData.middleX = 16;
  //  goomData.zoomFilterData.middleY = 1;
  m_goomData.zoomFilterData.reverse = true;
  //  goomData.zoomFilterData.hPlaneEffect = 0;
  //  goomData.zoomFilterData.vPlaneEffect = 0;
  m_goomData.zoomFilterData.waveEffect = false;
  m_goomData.zoomFilterData.hypercosEffect = ZoomFilterData::HypercosEffect::none;
  //  goomData.zoomFilterData.noisify = false;
  //  goomData.zoomFilterData.noiseFactor = 1;
  //  goomData.zoomFilterData.blockyWavy = false;
  m_goomData.zoomFilterData.waveFreqFactor = ZoomFilterData::DEFAULT_WAVE_FREQ_FACTOR;
  m_goomData.zoomFilterData.waveAmplitude = ZoomFilterData::DEFAULT_WAVE_AMPLITUDE;
  m_goomData.zoomFilterData.waveEffectType = ZoomFilterData::DEFAULT_WAVE_EFFECT_TYPE;
  m_goomData.zoomFilterData.scrunchAmplitude = ZoomFilterData::DEFAULT_SCRUNCH_AMPLITUDE;
  m_goomData.zoomFilterData.speedwayAmplitude = ZoomFilterData::DEFAULT_SPEEDWAY_AMPLITUDE;
  m_goomData.zoomFilterData.amuletteAmplitude = ZoomFilterData::DEFAULT_AMULET_AMPLITUDE;
  m_goomData.zoomFilterData.crystalBallAmplitude = ZoomFilterData::DEFAULT_CRYSTAL_BALL_AMPLITUDE;
  m_goomData.zoomFilterData.hypercosFreq = ZoomFilterData::DEFAULT_HYPERCOS_FREQ;
  m_goomData.zoomFilterData.hypercosAmplitude = ZoomFilterData::DEFAULT_HYPERCOS_AMPLITUDE;
  m_goomData.zoomFilterData.hPlaneEffectAmplitude =
      ZoomFilterData::DEFAULT_H_PLANE_EFFECT_AMPLITUDE;
  m_goomData.zoomFilterData.vPlaneEffectAmplitude =
      ZoomFilterData::DEFAULT_V_PLANE_EFFECT_AMPLITUDE;

  switch (m_goomEvent.GetRandomFilterEvent())
  {
    case GoomFilterEvent::yOnlyMode:
      m_goomData.zoomFilterData.mode = ZoomFilterMode::yOnlyMode;
      break;
    case GoomFilterEvent::speedwayMode:
      m_goomData.zoomFilterData.mode = ZoomFilterMode::speedwayMode;
      m_goomData.zoomFilterData.speedwayAmplitude = getRandInRange(
          ZoomFilterData::MIN_SPEEDWAY_AMPLITUDE, ZoomFilterData::MAX_SPEEDWAY_AMPLITUDE);
      break;
    case GoomFilterEvent::normalMode:
      m_goomData.zoomFilterData.mode = ZoomFilterMode::normalMode;
      break;
    case GoomFilterEvent::waveModeWithHyperCosEffect:
      m_goomData.zoomFilterData.hypercosEffect =
          GetHypercosEffect(m_goomEvent.Happens(GoomEvent::hypercosEffectOnWithWaveMode));
      [[fallthrough]];
    case GoomFilterEvent::waveMode:
      m_goomData.zoomFilterData.mode = ZoomFilterMode::waveMode;
      m_goomData.zoomFilterData.reverse = false;
      m_goomData.zoomFilterData.waveEffect =
          m_goomEvent.Happens(GoomEvent::waveEffectOnWithWaveMode);
      if (m_goomEvent.Happens(GoomEvent::changeVitesseWithWaveMode))
      {
        m_goomData.zoomFilterData.vitesse = (m_goomData.zoomFilterData.vitesse + 127) >> 1;
      }
      m_goomData.zoomFilterData.waveEffectType =
          static_cast<ZoomFilterData::WaveEffect>(getRandInRange(0, 2));
      if (m_goomEvent.Happens(GoomEvent::allowStrangeWaveValues))
      {
        // BUG HERE - wrong ranges - BUT GIVES GOOD AFFECT
        m_goomData.zoomFilterData.waveAmplitude = getRandInRange(
            ZoomFilterData::MIN_LARGE_WAVE_AMPLITUDE, ZoomFilterData::MAX_LARGE_WAVE_AMPLITUDE);
        m_goomData.zoomFilterData.waveFreqFactor = getRandInRange(
            ZoomFilterData::MIN_WAVE_SMALL_FREQ_FACTOR, ZoomFilterData::MAX_WAVE_SMALL_FREQ_FACTOR);
      }
      else
      {
        m_goomData.zoomFilterData.waveAmplitude =
            getRandInRange(ZoomFilterData::MIN_WAVE_AMPLITUDE, ZoomFilterData::MAX_WAVE_AMPLITUDE);
        m_goomData.zoomFilterData.waveFreqFactor = getRandInRange(
            ZoomFilterData::MIN_WAVE_FREQ_FACTOR, ZoomFilterData::MAX_WAVE_FREQ_FACTOR);
      }
      break;
    case GoomFilterEvent::crystalBallMode:
      m_goomData.zoomFilterData.mode = ZoomFilterMode::crystalBallMode;
      m_goomData.zoomFilterData.crystalBallAmplitude = getRandInRange(
          ZoomFilterData::MIN_CRYSTAL_BALL_AMPLITUDE, ZoomFilterData::MAX_CRYSTAL_BALL_AMPLITUDE);
      break;
    case GoomFilterEvent::crystalBallModeWithEffects:
      m_goomData.zoomFilterData.mode = ZoomFilterMode::crystalBallMode;
      m_goomData.zoomFilterData.waveEffect =
          m_goomEvent.Happens(GoomEvent::waveEffectOnWithCrystalBallMode);
      m_goomData.zoomFilterData.hypercosEffect =
          GetHypercosEffect(m_goomEvent.Happens(GoomEvent::hypercosEffectOnWithCrystalBallMode));
      m_goomData.zoomFilterData.crystalBallAmplitude = getRandInRange(
          ZoomFilterData::MIN_CRYSTAL_BALL_AMPLITUDE, ZoomFilterData::MAX_CRYSTAL_BALL_AMPLITUDE);
      break;
    case GoomFilterEvent::amuletMode:
      m_goomData.zoomFilterData.mode = ZoomFilterMode::amuletMode;
      m_goomData.zoomFilterData.amuletteAmplitude = getRandInRange(
          ZoomFilterData::MIN_AMULET_AMPLITUDE, ZoomFilterData::MAX_AMULET_AMPLITUDE);
      break;
    case GoomFilterEvent::waterMode:
      m_goomData.zoomFilterData.mode = ZoomFilterMode::waterMode;
      break;
    case GoomFilterEvent::scrunchMode:
      m_goomData.zoomFilterData.mode = ZoomFilterMode::scrunchMode;
      m_goomData.zoomFilterData.scrunchAmplitude = getRandInRange(
          ZoomFilterData::MIN_SCRUNCH_AMPLITUDE, ZoomFilterData::MAX_SCRUNCH_AMPLITUDE);
      break;
    case GoomFilterEvent::scrunchModeWithEffects:
      m_goomData.zoomFilterData.mode = ZoomFilterMode::scrunchMode;
      m_goomData.zoomFilterData.waveEffect = true;
      m_goomData.zoomFilterData.hypercosEffect = GetHypercosEffect(true);
      m_goomData.zoomFilterData.scrunchAmplitude = getRandInRange(
          ZoomFilterData::MIN_SCRUNCH_AMPLITUDE, ZoomFilterData::MAX_SCRUNCH_AMPLITUDE);
      break;
    case GoomFilterEvent::hyperCos1Mode:
      m_goomData.zoomFilterData.mode = ZoomFilterMode::hyperCos1Mode;
      m_goomData.zoomFilterData.hypercosEffect =
          GetHypercosEffect(m_goomEvent.Happens(GoomEvent::hypercosEffectOnWithHyperCos1Mode));
      break;
    case GoomFilterEvent::hyperCos2Mode:
      m_goomData.zoomFilterData.mode = ZoomFilterMode::hyperCos2Mode;
      m_goomData.zoomFilterData.hypercosEffect =
          GetHypercosEffect(m_goomEvent.Happens(GoomEvent::hypercosEffectOnWithHyperCos2Mode));
      break;
    default:
      throw std::logic_error("GoomFilterEvent not implemented.");
  }

  if (m_goomData.zoomFilterData.hypercosEffect != ZoomFilterData::HypercosEffect::none)
  {
    m_goomData.zoomFilterData.hypercosFreq =
        getRandInRange(ZoomFilterData::MIN_HYPERCOS_FREQ, ZoomFilterData::MAX_HYPERCOS_FREQ);
    m_goomData.zoomFilterData.hypercosAmplitude = getRandInRange(
        ZoomFilterData::MIN_HYPERCOS_AMPLITUDE, ZoomFilterData::MAX_HYPERCOS_AMPLITUDE);
  }

  if (m_goomData.zoomFilterData.mode == ZoomFilterMode::amuletMode)
  {
    m_curGDrawables.erase(GoomDrawable::tentacles);
    m_stats.TentaclesDisabled();
  }
  else
  {
    m_curGDrawables = m_states.GetCurrentDrawables();
  }
}

void GoomControl::GoomControlImpl::ChangeFilterMode()
{
  logDebug("Time to change the filter mode.");

  m_stats.FilterModeChange();

  SetNextFilterMode();

  m_stats.FilterModeChange(m_goomData.zoomFilterData.mode);

  m_visualFx.ifs_fx->Renew();
  m_stats.IfsRenew();
}

void GoomControl::GoomControlImpl::ChangeState()
{
  const auto oldGDrawables = m_states.GetCurrentDrawables();

  const size_t oldStateIndex = m_states.GetCurrentStateIndex();
  for (size_t numTry = 0; numTry < 10; numTry++)
  {
    m_states.DoRandomStateChange();
    if (oldStateIndex != m_states.GetCurrentStateIndex())
    {
      // Only pick a different state.
      break;
    }
  }

  m_curGDrawables = m_states.GetCurrentDrawables();
  logDebug("Changed goom state to {}", m_states.GetCurrentStateIndex());
  m_stats.StateChange(m_timeInState);
  m_stats.StateChange(m_states.GetCurrentStateIndex(), m_timeInState);
  m_timeInState = 0;

  if (m_states.IsCurrentlyDrawable(GoomDrawable::IFS))
  {
    if (!oldGDrawables.contains(GoomDrawable::IFS))
    {
      m_visualFx.ifs_fx->Init();
    }
    else if (m_goomEvent.Happens(GoomEvent::ifsRenew))
    {
      m_visualFx.ifs_fx->Renew();
      m_stats.IfsRenew();
    }
    m_visualFx.ifs_fx->UpdateIncr();
  }

  if (!m_states.IsCurrentlyDrawable(GoomDrawable::scope))
  {
    m_goomData.stopLines = 0xf000 & 5;
  }
  if (!m_states.IsCurrentlyDrawable(GoomDrawable::farScope))
  {
    m_goomData.stopLines = 0;
    m_goomData.lineMode = m_goomData.drawLinesDuration;
  }

  // Tentacles and amulette don't look so good together.
  if (m_states.IsCurrentlyDrawable(GoomDrawable::tentacles) &&
      (m_goomData.zoomFilterData.mode == ZoomFilterMode::amuletMode))
  {
    ChangeFilterMode();
  }
}

void GoomControl::GoomControlImpl::ChangeMilieu()
{

  if ((m_goomData.zoomFilterData.mode == ZoomFilterMode::waterMode) ||
      (m_goomData.zoomFilterData.mode == ZoomFilterMode::yOnlyMode) ||
      (m_goomData.zoomFilterData.mode == ZoomFilterMode::amuletMode))
  {
    m_goomData.zoomFilterData.middleX = GetScreenWidth() / 2;
    m_goomData.zoomFilterData.middleY = GetScreenHeight() / 2;
  }
  else
  {
    // clang-format off
    enum class MiddlePointEvents { event1, event2, event3, event4 };
    static const Weights<MiddlePointEvents> middlePointWeights{{
      { MiddlePointEvents::event1,  3 },
      { MiddlePointEvents::event2,  2 },
      { MiddlePointEvents::event3,  2 },
      { MiddlePointEvents::event4, 18 },
    }};
    // clang-format on

    switch (middlePointWeights.getRandomWeighted())
    {
      case MiddlePointEvents::event1:
        m_goomData.zoomFilterData.middleX = GetScreenWidth() / 2;
        m_goomData.zoomFilterData.middleY = GetScreenHeight() - 1;
        break;
      case MiddlePointEvents::event2:
        m_goomData.zoomFilterData.middleX = GetScreenWidth() - 1;
        break;
      case MiddlePointEvents::event3:
        m_goomData.zoomFilterData.middleX = 1;
        break;
      case MiddlePointEvents::event4:
        m_goomData.zoomFilterData.middleX = GetScreenWidth() / 2;
        m_goomData.zoomFilterData.middleY = GetScreenHeight() / 2;
        break;
      default:
        throw std::logic_error("Unknown MiddlePointEvents enum.");
    }
  }

  // clang-format off
  // @formatter:off
  enum class PlaneEffectEvents { event1, event2, event3, event4, event5, event6, event7, event8 };
  static const Weights<PlaneEffectEvents> s_planeEffectWeights{{
    { PlaneEffectEvents::event1,  1 },
    { PlaneEffectEvents::event2,  1 },
    { PlaneEffectEvents::event3,  4 },
    { PlaneEffectEvents::event4,  1 },
    { PlaneEffectEvents::event5,  1 },
    { PlaneEffectEvents::event6,  1 },
    { PlaneEffectEvents::event7,  1 },
    { PlaneEffectEvents::event8,  2 },
  }};
  // clang-format on
  // @formatter:on

  switch (s_planeEffectWeights.getRandomWeighted())
  {
    case PlaneEffectEvents::event1:
      m_goomData.zoomFilterData.vPlaneEffect = getRandInRange(-2, +3);
      m_goomData.zoomFilterData.hPlaneEffect = getRandInRange(-2, +3);
      break;
    case PlaneEffectEvents::event2:
      m_goomData.zoomFilterData.vPlaneEffect = 0;
      m_goomData.zoomFilterData.hPlaneEffect = getRandInRange(-7, +8);
      break;
    case PlaneEffectEvents::event3:
      m_goomData.zoomFilterData.vPlaneEffect = getRandInRange(-5, +6);
      m_goomData.zoomFilterData.hPlaneEffect = -m_goomData.zoomFilterData.vPlaneEffect;
      break;
    case PlaneEffectEvents::event4:
      m_goomData.zoomFilterData.hPlaneEffect = static_cast<int>(getRandInRange(5U, 13U));
      m_goomData.zoomFilterData.vPlaneEffect = -m_goomData.zoomFilterData.hPlaneEffect;
      break;
    case PlaneEffectEvents::event5:
      m_goomData.zoomFilterData.vPlaneEffect = static_cast<int>(getRandInRange(5U, 13U));
      m_goomData.zoomFilterData.hPlaneEffect = -m_goomData.zoomFilterData.hPlaneEffect;
      break;
    case PlaneEffectEvents::event6:
      m_goomData.zoomFilterData.hPlaneEffect = 0;
      m_goomData.zoomFilterData.vPlaneEffect = getRandInRange(-9, +10);
      break;
    case PlaneEffectEvents::event7:
      m_goomData.zoomFilterData.hPlaneEffect = getRandInRange(-9, +10);
      m_goomData.zoomFilterData.vPlaneEffect = getRandInRange(-9, +10);
      break;
    case PlaneEffectEvents::event8:
      m_goomData.zoomFilterData.vPlaneEffect = 0;
      m_goomData.zoomFilterData.hPlaneEffect = 0;
      break;
    default:
      throw std::logic_error("Unknown MiddlePointEvents enum.");
  }
  m_goomData.zoomFilterData.hPlaneEffectAmplitude = getRandInRange(
      ZoomFilterData::MIN_H_PLANE_EFFECT_AMPLITUDE, ZoomFilterData::MAX_H_PLANE_EFFECT_AMPLITUDE);
  // I think 'vPlaneEffectAmplitude' has to be the same as 'hPlaneEffectAmplitude' otherwise
  //   buffer breaking effects occur.
  m_goomData.zoomFilterData.vPlaneEffectAmplitude =
      m_goomData.zoomFilterData.hPlaneEffectAmplitude + getRandInRange(-0.0009F, 0.0009F);
  //  goomData.zoomFilterData.vPlaneEffectAmplitude = getRandInRange(
  //      ZoomFilterData::minVPlaneEffectAmplitude, ZoomFilterData::maxVPlaneEffectAmplitude);
}

void GoomControl::GoomControlImpl::BigNormalUpdate(ZoomFilterData** pzfd)
{
  if (m_goomData.stateSelectionBlocker)
  {
    m_goomData.stateSelectionBlocker--;
  }
  else if (m_goomEvent.Happens(GoomEvent::changeState))
  {
    m_goomData.stateSelectionBlocker = 3;
    ChangeState();
  }

  m_goomData.lockVar = 50;
  m_stats.LockChange();
  const int32_t newvit =
      STOP_SPEED + 1 -
      static_cast<int32_t>(3.5F * std::log10(m_goomInfo->GetSoundInfo().getSpeed() * 60 + 1));
  // retablir le zoom avant..
  if ((m_goomData.zoomFilterData.reverse) && (!(m_cycle % 13)) &&
      m_goomEvent.Happens(GoomEvent::filterReverseOffAndStopSpeed))
  {
    m_goomData.zoomFilterData.reverse = false;
    m_goomData.zoomFilterData.vitesse = STOP_SPEED - 2;
    m_goomData.lockVar = 75;
    m_stats.LockChange();
  }
  if (m_goomEvent.Happens(GoomEvent::filterReverseOn))
  {
    m_goomData.zoomFilterData.reverse = true;
    m_goomData.lockVar = 100;
    m_stats.LockChange();
  }

  if (m_goomEvent.Happens(GoomEvent::filterVitesseStopSpeedMinus1))
  {
    m_goomData.zoomFilterData.vitesse = STOP_SPEED - 1;
  }
  if (m_goomEvent.Happens(GoomEvent::filterVitesseStopSpeedPlus1))
  {
    m_goomData.zoomFilterData.vitesse = STOP_SPEED + 1;
  }

  ChangeMilieu();

  if (m_goomEvent.Happens(GoomEvent::turnOffNoise))
  {
    m_goomData.zoomFilterData.noisify = false;
  }
  else
  {
    m_goomData.zoomFilterData.noisify = true;
    m_goomData.zoomFilterData.noiseFactor = 1;
    m_goomData.lockVar *= 2;
    m_stats.LockChange();
    m_stats.DoNoise();
  }

  if (!m_goomEvent.Happens(GoomEvent::changeBlockyWavyToOn))
  {
    m_goomData.zoomFilterData.blockyWavy = false;
  }
  else
  {
    m_stats.DoBlockyWavy();
    m_goomData.zoomFilterData.blockyWavy = true;
  }

  if (!m_goomEvent.Happens(GoomEvent::changeZoomFilterAllowOverexposedToOn))
  {
    m_visualFx.zoomFilter_fx->setBuffSettings({.buffIntensity = 0.5, .allowOverexposed = false});
  }
  else
  {
    m_stats.DoZoomFilterAllowOverexposed();
    m_visualFx.zoomFilter_fx->setBuffSettings({.buffIntensity = 0.5, .allowOverexposed = true});
  }

  if (m_goomData.zoomFilterData.mode == ZoomFilterMode::amuletMode)
  {
    m_goomData.zoomFilterData.vPlaneEffect = 0;
    m_goomData.zoomFilterData.hPlaneEffect = 0;
    //    goomData.zoomFilterData.noisify = false;
  }

  if ((m_goomData.zoomFilterData.middleX == 1) ||
      (m_goomData.zoomFilterData.middleX == GetScreenWidth() - 1))
  {
    m_goomData.zoomFilterData.vPlaneEffect = 0;
    if (m_goomEvent.Happens(GoomEvent::filterZeroHPlaneEffect))
    {
      m_goomData.zoomFilterData.hPlaneEffect = 0;
    }
  }

  logDebug("newvit = {}, goomData.zoomFilterData.vitesse = {}", newvit,
           m_goomData.zoomFilterData.vitesse);
  if (newvit < m_goomData.zoomFilterData.vitesse)
  {
    // on accelere
    logDebug("newvit = {} < {} = goomData.zoomFilterData.vitesse", newvit,
             m_goomData.zoomFilterData.vitesse);
    *pzfd = &m_goomData.zoomFilterData;
    if (((newvit < (STOP_SPEED - 7)) && (m_goomData.zoomFilterData.vitesse < STOP_SPEED - 6) &&
         (m_cycle % 3 == 0)) ||
        m_goomEvent.Happens(GoomEvent::filterChangeVitesseAndToggleReverse))
    {
      m_goomData.zoomFilterData.vitesse =
          STOP_SPEED - static_cast<int32_t>(getNRand(2)) + static_cast<int32_t>(getNRand(2));
      m_goomData.zoomFilterData.reverse = !m_goomData.zoomFilterData.reverse;
    }
    else
    {
      m_goomData.zoomFilterData.vitesse = (newvit + m_goomData.zoomFilterData.vitesse * 7) / 8;
    }
    m_goomData.lockVar += 50;
    m_stats.LockChange();
  }

  if (m_goomData.lockVar > 150)
  {
    m_goomData.switchIncr = GoomData::switchIncrAmount;
    m_goomData.switchMult = 1.0F;
  }
}

void GoomControl::GoomControlImpl::MegaLentUpdate(ZoomFilterData** pzfd)
{
  logDebug("mega lent change");
  *pzfd = &m_goomData.zoomFilterData;
  m_goomData.zoomFilterData.vitesse = STOP_SPEED - 1;
  m_goomData.lockVar += 50;
  m_stats.LockChange();
  m_goomData.switchIncr = GoomData::switchIncrAmount;
  m_goomData.switchMult = 1.0F;
}

void GoomControl::GoomControlImpl::BigUpdate(ZoomFilterData** pzfd)
{
  m_stats.DoBigUpdate();

  // reperage de goom (acceleration forte de l'acceleration du volume)
  //   -> coup de boost de la vitesse si besoin..
  logDebug("sound getTimeSinceLastGoom() = {}", m_goomInfo->GetSoundInfo().getTimeSinceLastGoom());
  if (m_goomInfo->GetSoundInfo().getTimeSinceLastGoom() == 0)
  {
    logDebug("sound getTimeSinceLastGoom() = 0.");
    m_stats.LastTimeGoomChange();
    BigNormalUpdate(pzfd);
  }

  // mode mega-lent
  if (m_goomEvent.Happens(GoomEvent::changeToMegaLentMode))
  {
    m_stats.MegaLentChange();
    MegaLentUpdate(pzfd);
  }
}

/* Changement d'effet de zoom !
 */
void GoomControl::GoomControlImpl::ChangeZoomEffect(ZoomFilterData* pzfd, const int forceMode)
{
  if (!m_goomEvent.Happens(GoomEvent::changeBlockyWavyToOn))
  {
    m_goomData.zoomFilterData.blockyWavy = false;
  }
  else
  {
    m_goomData.zoomFilterData.blockyWavy = true;
    m_stats.DoBlockyWavy();
  }

  if (!m_goomEvent.Happens(GoomEvent::changeZoomFilterAllowOverexposedToOn))
  {
    m_visualFx.zoomFilter_fx->setBuffSettings({.buffIntensity = 0.5, .allowOverexposed = false});
  }
  else
  {
    m_stats.DoZoomFilterAllowOverexposed();
    m_visualFx.zoomFilter_fx->setBuffSettings({.buffIntensity = 0.5, .allowOverexposed = true});
  }

  if (pzfd)
  {
    logDebug("pzfd != nullptr");

    m_goomData.cyclesSinceLastChange = 0;
    m_goomData.switchIncr = GoomData::switchIncrAmount;

    int diff = m_goomData.zoomFilterData.vitesse - m_goomData.previousZoomSpeed;
    if (diff < 0)
    {
      diff = -diff;
    }

    if (diff > 2)
    {
      m_goomData.switchIncr *= (diff + 2) / 2;
    }
    m_goomData.previousZoomSpeed = m_goomData.zoomFilterData.vitesse;
    m_goomData.switchMult = 1.0F;

    if (((m_goomInfo->GetSoundInfo().getTimeSinceLastGoom() == 0) &&
         (m_goomInfo->GetSoundInfo().getTotalGoom() < 2)) ||
        (forceMode > 0))
    {
      m_goomData.switchIncr = 0;
      m_goomData.switchMult = GoomData::switchMultAmount;

      m_visualFx.ifs_fx->Renew();
      m_stats.IfsRenew();
    }
  }
  else
  {
    logDebug("pzfd = nullptr");
    logDebug("goomData.cyclesSinceLastChange = {}", m_goomData.cyclesSinceLastChange);
    if (m_goomData.cyclesSinceLastChange > TIME_BETWEEN_CHANGE)
    {
      logDebug("goomData.cyclesSinceLastChange = {} > {} = timeBetweenChange",
               m_goomData.cyclesSinceLastChange, TIME_BETWEEN_CHANGE);
      pzfd = &m_goomData.zoomFilterData;
      m_goomData.cyclesSinceLastChange = 0;
      m_visualFx.ifs_fx->Renew();
      m_stats.IfsRenew();
    }
    else
    {
      m_goomData.cyclesSinceLastChange++;
    }
  }

  if (pzfd)
  {
    logDebug("pzfd->mode = {}", pzfd->mode);
  }
}

void GoomControl::GoomControlImpl::ApplyTentaclesIfRequired()
{
  if (!m_curGDrawables.contains(GoomDrawable::tentacles))
  {
    m_visualFx.tentacles_fx->applyNoDraw();
    return;
  }

  logDebug("curGDrawables tentacles is set.");
  m_stats.DoTentacles();
  m_visualFx.tentacles_fx->setBuffSettings(
      m_states.GetCurrentBuffSettings(GoomDrawable::tentacles));
  m_visualFx.tentacles_fx->apply(m_imageBuffers.GetP2(), m_imageBuffers.GetP1());
}

void GoomControl::GoomControlImpl::ApplyStarsIfRequired()
{
  if (!m_curGDrawables.contains(GoomDrawable::stars))
  {
    return;
  }

  logDebug("curGDrawables stars is set.");
  m_stats.DoStars();
  m_visualFx.star_fx->setBuffSettings(m_states.GetCurrentBuffSettings(GoomDrawable::stars));
  //  visualFx.star_fx->apply(imageBuffers.getP2(), imageBuffers.getP1());
  m_visualFx.star_fx->apply(m_imageBuffers.GetP2(), m_imageBuffers.GetP1());
}

#ifdef SHOW_STATE_TEXT_ON_SCREEN

void GoomControl::GoomControlImpl::DisplayStateText()
{
  std::string message = "";

  message += std20::format("State: {}\n", states.getCurrentStateIndex());
  message += std20::format("Filter: {}\n", enumToString(goomData.zoomFilterData.mode));
  message += std20::format("switchIncr: {}\n", goomData.switchIncr);
  message += std20::format("switchIncrAmount: {}\n", goomData.switchIncrAmount);
  message += std20::format("switchMult: {}\n", goomData.switchMult);
  message += std20::format("switchMultAmount: {}\n", goomData.switchMultAmount);
  message += std20::format("previousZoomSpeed: {}\n", goomData.previousZoomSpeed);
  message += std20::format("vitesse: {}\n", goomData.zoomFilterData.vitesse);
  message += std20::format("pertedec: {}\n", goomData.zoomFilterData.pertedec);
  message += std20::format("reverse: {}\n", goomData.zoomFilterData.reverse);
  message += std20::format("hPlaneEffect: {}\n", goomData.zoomFilterData.hPlaneEffect);
  message += std20::format("vPlaneEffect: {}\n", goomData.zoomFilterData.vPlaneEffect);
  message += std20::format("hypercosEffect: {}\n", goomData.zoomFilterData.hypercosEffect);
  message += std20::format("middleX: {}\n", goomData.zoomFilterData.middleX);
  message += std20::format("middleY: {}\n", goomData.zoomFilterData.middleY);
  message += std20::format("noisify: {}\n", goomData.zoomFilterData.noisify);
  message += std20::format("noiseFactor: {}\n", goomData.zoomFilterData.noiseFactor);
  message += std20::format("cyclesSinceLastChange: {}\n", goomData.cyclesSinceLastChange);
  message += std20::format("lineMode: {}\n", goomData.lineMode);
  message += std20::format("lockVar: {}\n", goomData.lockVar);
  message += std20::format("stopLines: {}\n", goomData.stopLines);

  updateMessage(message.c_str());
}

#endif

void GoomControl::GoomControlImpl::DisplayText(const char* songTitle,
                                               const char* message,
                                               const float fps)
{
  UpdateMessage(message);

  if (fps > 0)
  {
    const std::string text = std20::format("{.0f} fps", fps);
    m_draw.Text(m_imageBuffers.GetP1(), 10, 24, text, 1, false);
  }

  if (songTitle != nullptr)
  {
    m_stats.SetSongTitle(songTitle);
    m_goomData.title = songTitle;
    m_goomData.timeOfTitleDisplay = GoomData::maxTitleDisplayTime;
  }

  if (m_goomData.timeOfTitleDisplay)
  {
    const auto xPos = static_cast<int>(GetScreenWidth() / 10);
    const auto yPos = static_cast<int>(GetScreenHeight() / 4 + 7);
    const auto spacing = m_goomData.timeOfTitleDisplay > GoomData::timeToSpaceTitleDisplay
                             ? 0.0F
                             : static_cast<float>(GoomData::timeToSpaceTitleDisplay -
                                                  m_goomData.timeOfTitleDisplay) /
                                   40.0F;

    DrawText(m_goomData.title, xPos, yPos, spacing, m_imageBuffers.GetOutputBuff());

    m_goomData.timeOfTitleDisplay--;

    if (m_goomData.timeOfTitleDisplay < GoomData::timeToFadeTitleDisplay)
    {
      DrawText(m_goomData.title, xPos, yPos, spacing, m_imageBuffers.GetP1());
    }
  }
}

void GoomControl::GoomControlImpl::DrawText(const std::string& str,
                                            const int xPos,
                                            const int yPos,
                                            const float spacing,
                                            PixelBuffer& buffer)
{
  /**
  const auto getFontColor = [&](const size_t textIndexOfChar, float x, float y, float width,
                                float height) {
    return Pixel{{.r = 255, .g = 255, .b = 0, .a = 255}};
  };
  const auto getOutlineFontColor = [&](const size_t textIndexOfChar, float x, float y, float width,
                                       float height) {
    return Pixel{{.r = 100, .g = 200, .b = 255, .a = 255}};
  };
  **/

  const ColorMaps colorMaps{};
  const ColorMap& colorMap1 = colorMaps.GetRandomColorMap();
  const ColorMap& colorMap2 = colorMaps.GetRandomColorMap();
  //  const ColorMap& colorMap1 = colorMaps.getColorMap(colordata::ColorMapName::autumn);
  //  const ColorMap& colorMap2 = colorMaps.getColorMap(colordata::ColorMapName::Blues);
  Pixel fontColor{{.r = 100, .g = 90, .b = 150, .a = 100}};
  Pixel outlineColor{{.r = 255, .g = 255, .b = 50, .a = 255}};
  const auto getFontColor = [&](const size_t textIndexOfChar, float x, float y, float width,
                                float height) {
    //return fontColor;
    const Pixel col1 = colorMap1.GetColor(x / width);
    const Pixel col2 = colorMap2.GetColor(y / height);
    //return col2;
    //return col1;
    return ColorMap::GetColorMix(col1, col2, 0.5);
  };
  const auto getOutlineFontColor = [&](const size_t textIndexOfChar, float x, float y, float width,
                                       float height) { return outlineColor; };

  //  CALL UP TO PREPARE ONCE ONLY
  m_text.SetText(str);
  m_text.SetFontColorFunc(getFontColor);
  m_text.SetOutlineFontColorFunc(getOutlineFontColor);
  m_text.SetCharSpacing(spacing);
  m_text.Prepare();
  m_text.Draw(xPos, yPos, buffer);
}

/*
 * Met a jour l'affichage du message defilant
 */
void GoomControl::GoomControlImpl::UpdateMessage(const char* message)
{
  // TODO FIX THIS!!!!
  return;
  if (message != nullptr)
  {
    m_messageData.message = message;
    const std::vector<std::string> msgLines = splitString(m_messageData.message, "\n");
    m_messageData.numberOfLinesInMessage = msgLines.size();
    m_messageData.affiche = 100 + 25 * m_messageData.numberOfLinesInMessage;
  }
  if (m_messageData.affiche)
  {
    TextDraw updateMessageText{m_goomInfo->GetScreenInfo().width,
                               m_goomInfo->GetScreenInfo().height};
    updateMessageText.SetFontFile(m_text.GetFontFile());
    updateMessageText.SetFontSize(15);
    updateMessageText.SetOutlineWidth(1);
    updateMessageText.SetAlignment(TextDraw::TextAlignment::left);
    const std::vector<std::string> msgLines = splitString(m_messageData.message, "\n");
    for (size_t i = 0; i < msgLines.size(); i++)
    {
      const auto yPos = static_cast<int>(10 + m_messageData.affiche -
                                         (m_messageData.numberOfLinesInMessage - i) * 25);
      updateMessageText.SetText(msgLines[i]);
      updateMessageText.Prepare();
      updateMessageText.Draw(50, yPos, m_imageBuffers.GetOutputBuff());
    }
    m_messageData.affiche--;
  }
}

void GoomControl::GoomControlImpl::StopRequest()
{
  logDebug("goomData.stopLines = {},"
           " curGDrawables.contains(GoomDrawable::scope) = {}",
           m_goomData.stopLines, m_curGDrawables.contains(GoomDrawable::scope));

  float param1 = 0;
  float param2 = 0;
  float amplitude = 0;
  Pixel couleur{};
  LinesFx::LineType mode;
  ChooseGoomLine(&param1, &param2, &couleur, &mode, &amplitude, 1);
  couleur = GetBlackLineColor();

  m_gmline1.SwitchLines(mode, param1, amplitude, couleur);
  m_gmline2.SwitchLines(mode, param2, amplitude, couleur);
  m_stats.SwitchLines();
  m_goomData.stopLines &= 0x0fff;
}

/* arret aleatore.. changement de mode de ligne..
  */
void GoomControl::GoomControlImpl::StopRandomLineChangeMode()
{
  if (m_goomData.lineMode != m_goomData.drawLinesDuration)
  {
    m_goomData.lineMode--;
    if (m_goomData.lineMode == -1)
    {
      m_goomData.lineMode = 0;
    }
  }
  else if ((m_cycle % 80 == 0) && m_goomEvent.Happens(GoomEvent::reduceLineMode) &&
           m_goomData.lineMode)
  {
    m_goomData.lineMode--;
  }

  if ((m_cycle % 120 == 0) && m_goomEvent.Happens(GoomEvent::updateLineMode) &&
      m_curGDrawables.contains(GoomDrawable::scope))
  {
    if (m_goomData.lineMode == 0)
    {
      m_goomData.lineMode = m_goomData.drawLinesDuration;
    }
    else if (m_goomData.lineMode == m_goomData.drawLinesDuration)
    {
      m_goomData.lineMode--;

      float param1 = 0;
      float param2 = 0;
      float amplitude = 0;
      Pixel color1{};
      LinesFx::LineType mode;
      ChooseGoomLine(&param1, &param2, &color1, &mode, &amplitude, m_goomData.stopLines);

      Pixel color2 = m_gmline2.GetRandomLineColor();
      if (m_goomData.stopLines)
      {
        m_goomData.stopLines--;
        if (m_goomEvent.Happens(GoomEvent::changeLineToBlack))
        {
          color2 = color1 = GetBlackLineColor();
        }
      }

      logDebug("goomData.lineMode = {} == {} = goomData.drawLinesDuration", m_goomData.lineMode,
               m_goomData.drawLinesDuration);
      m_gmline1.SwitchLines(mode, param1, amplitude, color1);
      m_gmline2.SwitchLines(mode, param2, amplitude, color2);
      m_stats.SwitchLines();
    }
  }
}

void GoomControl::GoomControlImpl::DisplayLines(const AudioSamples& soundData)
{
  if (!m_curGDrawables.contains(GoomDrawable::lines))
  {
    return;
  }

  logDebug("curGDrawables lines is set.");

  m_stats.DoLines();

  m_gmline2.SetPower(m_gmline1.GetPower());

  const std::vector<int16_t>& audioSample = soundData.getSample(0);
  m_gmline1.DrawLines(audioSample, m_imageBuffers.GetP1(), m_imageBuffers.GetP2());
  m_gmline2.DrawLines(audioSample, m_imageBuffers.GetP1(), m_imageBuffers.GetP2());
  //  gmline2.drawLines(soundData.getSample(1), imageBuffers.getP1(), imageBuffers.getP2());

  if (((m_cycle % 121) == 9) && m_goomEvent.Happens(GoomEvent::changeGoomLine) &&
      ((m_goomData.lineMode == 0) || (m_goomData.lineMode == m_goomData.drawLinesDuration)))
  {
    logDebug("cycle % 121 etc.: goomInfo->cycle = {}, rand1_3 = ?", m_cycle);
    float param1 = 0;
    float param2 = 0;
    float amplitude = 0;
    Pixel color1{};
    LinesFx::LineType mode;
    ChooseGoomLine(&param1, &param2, &color1, &mode, &amplitude, m_goomData.stopLines);

    Pixel color2 = m_gmline2.GetRandomLineColor();
    if (m_goomData.stopLines)
    {
      m_goomData.stopLines--;
      if (m_goomEvent.Happens(GoomEvent::changeLineToBlack))
      {
        color2 = color1 = GetBlackLineColor();
      }
    }
    m_gmline1.SwitchLines(mode, param1, amplitude, color1);
    m_gmline2.SwitchLines(mode, param2, amplitude, color2);
  }
}

void GoomControl::GoomControlImpl::BigBreakIfMusicIsCalm(ZoomFilterData** pzfd)
{
  logDebug("sound getSpeed() = {:.2}, goomData.zoomFilterData.vitesse = {}, "
           "cycle = {}",
           m_goomInfo->GetSoundInfo().getSpeed(), m_goomData.zoomFilterData.vitesse, m_cycle);
  if ((m_goomInfo->GetSoundInfo().getSpeed() < 0.01F) &&
      (m_goomData.zoomFilterData.vitesse < (STOP_SPEED - 4)) && (m_cycle % 16 == 0))
  {
    logDebug("sound getSpeed() = {:.2}", m_goomInfo->GetSoundInfo().getSpeed());
    BigBreak(pzfd);
  }
}

void GoomControl::GoomControlImpl::BigBreak(ZoomFilterData** pzfd)
{
  *pzfd = &m_goomData.zoomFilterData;
  m_goomData.zoomFilterData.vitesse += 3;
}

void GoomControl::GoomControlImpl::ForceFilterMode(const int forceMode, ZoomFilterData** pzfd)
{
  *pzfd = &m_goomData.zoomFilterData;
  (*pzfd)->mode = static_cast<ZoomFilterMode>(forceMode - 1);
}

void GoomControl::GoomControlImpl::LowerSpeed(ZoomFilterData** pzfd)
{
  *pzfd = &m_goomData.zoomFilterData;
  m_goomData.zoomFilterData.vitesse++;
}

void GoomControl::GoomControlImpl::StopDecrementing(ZoomFilterData** pzfd)
{
  *pzfd = &m_goomData.zoomFilterData;
}

void GoomControl::GoomControlImpl::BigUpdateIfNotLocked(ZoomFilterData** pzfd)
{
  logDebug("goomData.lockVar = {}", m_goomData.lockVar);
  if (m_goomData.lockVar == 0)
  {
    logDebug("goomData.lockVar = 0");
    BigUpdate(pzfd);
  }
  logDebug("sound getTimeSinceLastGoom() = {}", m_goomInfo->GetSoundInfo().getTimeSinceLastGoom());
}

void GoomControl::GoomControlImpl::ForceFilterModeIfSet(ZoomFilterData** pzfd, const int forceMode)
{
  constexpr auto NUM_FILTER_FX = static_cast<size_t>(ZoomFilterMode::_size);

  logDebug("forceMode = {}", forceMode);
  if ((forceMode > 0) && (size_t(forceMode) <= NUM_FILTER_FX))
  {
    logDebug("forceMode = {} <= numFilterFx = {}.", forceMode, NUM_FILTER_FX);
    ForceFilterMode(forceMode, pzfd);
  }
  if (forceMode == -1)
  {
    pzfd = nullptr;
  }
}

void GoomControl::GoomControlImpl::StopIfRequested()
{
  logDebug("goomData.stopLines = {}, curGState->scope = {}", m_goomData.stopLines,
           m_states.IsCurrentlyDrawable(GoomDrawable::scope));
  if ((m_goomData.stopLines & 0xf000) || !m_states.IsCurrentlyDrawable(GoomDrawable::scope))
  {
    StopRequest();
  }
}

void GoomControl::GoomControlImpl::DisplayLinesIfInAGoom(const AudioSamples& soundData)
{
  logDebug("goomData.lineMode = {} != 0 || sound getTimeSinceLastGoom() = {}", m_goomData.lineMode,
           m_goomInfo->GetSoundInfo().getTimeSinceLastGoom());
  if ((m_goomData.lineMode != 0) || (m_goomInfo->GetSoundInfo().getTimeSinceLastGoom() < 5))
  {
    logDebug("goomData.lineMode = {} != 0 || sound getTimeSinceLastGoom() = {} < 5",
             m_goomData.lineMode, m_goomInfo->GetSoundInfo().getTimeSinceLastGoom());

    DisplayLines(soundData);
  }
}

void GoomControl::GoomControlImpl::ApplyIfsIfRequired()
{
  if (!m_curGDrawables.contains(GoomDrawable::IFS))
  {
    m_visualFx.ifs_fx->applyNoDraw();
    return;
  }

  logDebug("curGDrawables IFS is set");
  m_stats.DoIfs();
  m_visualFx.ifs_fx->setBuffSettings(m_states.GetCurrentBuffSettings(GoomDrawable::IFS));
  m_visualFx.ifs_fx->apply(m_imageBuffers.GetP2(), m_imageBuffers.GetP1());
}

void GoomControl::GoomControlImpl::RegularlyLowerTheSpeed(ZoomFilterData** pzfd)
{
  logDebug("goomData.zoomFilterData.vitesse = {}, cycle = {}", m_goomData.zoomFilterData.vitesse,
           m_cycle);
  if ((m_cycle % 73 == 0) && (m_goomData.zoomFilterData.vitesse < (STOP_SPEED - 5)))
  {
    logDebug("cycle % 73 = 0 && dgoomData.zoomFilterData.vitesse = {} < {} - 5, ", m_cycle,
             m_goomData.zoomFilterData.vitesse, STOP_SPEED);
    LowerSpeed(pzfd);
  }
}

void GoomControl::GoomControlImpl::StopDecrementingAfterAWhile(ZoomFilterData** pzfd)
{
  logDebug("cycle = {}, goomData.zoomFilterData.pertedec = {}", m_cycle,
           m_goomData.zoomFilterData.pertedec);
  if ((m_cycle % 101 == 0) && (ZoomFilterData::pertedec == 7))
  {
    logDebug("cycle % 101 = 0 && goomData.zoomFilterData.pertedec = 7, ", m_cycle,
             m_goomData.zoomFilterData.vitesse);
    StopDecrementing(pzfd);
  }
}

void GoomControl::GoomControlImpl::ApplyDotsIfRequired()
{
  if (!m_curGDrawables.contains(GoomDrawable::dots))
  {
    return;
  }

  logDebug("goomInfo->curGDrawables points is set.");
  m_stats.DoDots();
  m_visualFx.goomDots_fx->apply(m_imageBuffers.GetP1());
  logDebug("sound getTimeSinceLastGoom() = {}", m_goomInfo->GetSoundInfo().getTimeSinceLastGoom());
}

GoomEvents::GoomEvents() noexcept
  : m_filterWeights{{WEIGHTED_FILTER_EVENTS.begin(), WEIGHTED_FILTER_EVENTS.end()}},
    m_lineTypeWeights{{WEIGHTED_LINE_EVENTS.begin(), WEIGHTED_LINE_EVENTS.end()}}
{
}

void GoomEvents::SetGoomInfo(PluginInfo* info)
{
  m_goomInfo = info;
}

inline auto GoomEvents::Happens(const GoomEvent event) const -> bool
{
  const WeightedEvent& weightedEvent = WEIGHTED_EVENTS[static_cast<size_t>(event)];
  return probabilityOfMInN(weightedEvent.m, weightedEvent.outOf);
}

inline auto GoomEvents::GetRandomFilterEvent() const -> GoomEvents::GoomFilterEvent
{
  //////////////////return GoomFilterEvent::amuletMode;
  //////////////////return GoomFilterEvent::waveModeWithHyperCosEffect;

  GoomEvents::GoomFilterEvent nextEvent = m_filterWeights.getRandomWeighted();
  for (size_t i = 0; i < 10; i++)
  {
    if (nextEvent != m_lastReturnedFilterEvent)
    {
      break;
    }
    nextEvent = m_filterWeights.getRandomWeighted();
  }
  m_lastReturnedFilterEvent = nextEvent;

  return nextEvent;
}

inline auto GoomEvents::GetRandomLineTypeEvent() const -> LinesFx::LineType
{
  return m_lineTypeWeights.getRandomWeighted();
}

GoomStates::GoomStates() : m_weightedStates{GetWeightedStates(g_States)}
{
  DoRandomStateChange();
}

inline auto GoomStates::IsCurrentlyDrawable(const GoomDrawable drawable) const -> bool
{
  return GetCurrentDrawables().contains(drawable);
}

inline auto GoomStates::GetCurrentStateIndex() const -> size_t
{
  return m_currentStateIndex;
}

inline auto GoomStates::GetCurrentDrawables() const -> GoomStates::DrawablesState
{
  GoomStates::DrawablesState currentDrawables{};
  for (const auto d : g_States[m_currentStateIndex].drawables)
  {
    currentDrawables.insert(d.fx);
  }
  return currentDrawables;
}

auto GoomStates::GetCurrentBuffSettings(const GoomDrawable theFx) const -> FXBuffSettings
{
  for (const auto& d : g_States[m_currentStateIndex].drawables)
  {
    if (d.fx == theFx)
    {
      return d.buffSettings;
    }
  }
  return FXBuffSettings{};
}

inline void GoomStates::DoRandomStateChange()
{
  m_currentStateIndex = static_cast<size_t>(m_weightedStates.getRandomWeighted());
}

auto GoomStates::GetWeightedStates(const GoomStates::WeightedStatesArray& theStates)
    -> std::vector<std::pair<uint16_t, size_t>>
{
  std::vector<std::pair<uint16_t, size_t>> weightedVals(theStates.size());
  for (size_t i = 0; i < theStates.size(); i++)
  {
    weightedVals[i] = std::make_pair(i, theStates[i].weight);
  }
  return weightedVals;
}

} // namespace goom
