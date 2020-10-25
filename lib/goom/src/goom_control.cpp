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
#include "gfontlib.h"
#include "goom_config.h"
#include "goom_dots_fx.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "goomutils/strutils.h"
#include "ifs_fx.h"
#include "lines_fx.h"
#include "tentacles_fx.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// #define SHOW_STATE_TEXT_ON_SCREEN

namespace goom
{

using namespace goom::utils;

class GoomEvents
{
public:
  GoomEvents() noexcept;

  void setGoomInfo(PluginInfo* goomInfo);

  enum class GoomEvent
  {
    changeFilterMode = 0,
    changeFilterFromAmuletteMode,
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
    amuletteMode,
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

  bool happens(const GoomEvent) const;
  GoomFilterEvent getRandomFilterEvent() const;
  LineType getRandomLineTypeEvent() const;

private:
  PluginInfo* goomInfo = nullptr;
  mutable GoomFilterEvent lastReturnedFilterEvent = GoomFilterEvent::_size;

  static constexpr size_t numGoomEvents = static_cast<size_t>(GoomEvent::_size);
  static constexpr size_t numGoomFilterEvents = static_cast<size_t>(GoomFilterEvent::_size);

  struct WeightedEvent
  {
    GoomEvent event;
    // m out of n
    uint32_t m;
    uint32_t outOf;
  };
  // clang-format off
  static constexpr std::array<WeightedEvent, numGoomEvents> weightedEvents{{
    { .event = GoomEvent::changeFilterMode,                     .m = 1, .outOf =  16 },
    { .event = GoomEvent::changeFilterFromAmuletteMode,         .m = 1, .outOf =   5 },
    { .event = GoomEvent::changeState,                          .m = 1, .outOf =   2 },
    { .event = GoomEvent::turnOffNoise,                         .m = 4, .outOf =   5 },
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

  static constexpr std::array<std::pair<GoomFilterEvent, size_t>, numGoomFilterEvents> weightedFilterEvents{{
    { GoomFilterEvent::waveModeWithHyperCosEffect,  2 },
    { GoomFilterEvent::waveMode,                    3 },
    { GoomFilterEvent::crystalBallMode,             1 },
    { GoomFilterEvent::crystalBallModeWithEffects,  2 },
    { GoomFilterEvent::amuletteMode,                2 },
    { GoomFilterEvent::waterMode,                   2 },
    { GoomFilterEvent::scrunchMode,                 2 },
    { GoomFilterEvent::scrunchModeWithEffects,      2 },
    { GoomFilterEvent::hyperCos1Mode,               3 },
    { GoomFilterEvent::hyperCos2Mode,               2 },
    { GoomFilterEvent::yOnlyMode,                   3 },
    { GoomFilterEvent::speedwayMode,                3 },
    { GoomFilterEvent::normalMode,                  2 },
  } };

  static constexpr std::array<std::pair<LineType, size_t>, numLineTypes> weightedLineEvents{{
    { LineType::circle, 8 },
    { LineType::hline,  2 },
    { LineType::vline,  2 },
  }};
  // clang-format on
  const Weights<GoomFilterEvent> filterWeights;
  const Weights<LineType> lineTypeWeights;
};

using GoomEvent = GoomEvents::GoomEvent;
using GoomFilterEvent = GoomEvents::GoomFilterEvent;

class GoomStates
{
public:
  using DrawablesState = std::unordered_set<GoomDrawable>;

  GoomStates();

  bool isCurrentlyDrawable(const GoomDrawable) const;
  size_t getCurrentStateIndex() const;
  DrawablesState getCurrentDrawables() const;
  const FXBuffSettings getCurrentBuffSettings(const GoomDrawable) const;

  void doRandomStateChange();

private:
  using GD = GoomDrawable;
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
  static const WeightedStatesArray states;
  static std::vector<std::pair<uint16_t, size_t>> getWeightedStates(const WeightedStatesArray&);
  const Weights<uint16_t> weightedStates;
  size_t currentStateIndex;
};

// clang-format off
const GoomStates::WeightedStatesArray GoomStates::states{{
  {
    .weight = 100,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.6, .allowOverexposed = true  } },
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = false } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 40,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = false } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 60,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = true  } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 70,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = true } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = false } },
    }},
  },
  {
    .weight = 70,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true } },
    }},
  },
  {
    .weight = 60,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = true } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = true } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = true } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = true } },
    }},
  },
  {
    .weight = 60,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = true  } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = true  } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 100,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.8, .allowOverexposed = true  } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 70,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = true  } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 100,
    .drawables {{
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = true  } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 60,
    .drawables {{
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.4, .allowOverexposed = true  } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 60,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 60,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
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
  void setSongTitle(const std::string& songTitle);
  void setStateStartValue(const uint32_t stateIndex);
  void setZoomFilterStartValue(const ZoomFilterMode filterMode);
  void setSeedStartValue(const uint64_t seed);
  void setStateLastValue(const uint32_t stateIndex);
  void setZoomFilterLastValue(const ZoomFilterData* filterData);
  void setSeedLastValue(const uint64_t seed);
  void reset();
  void log(const StatsLogValueFunc) const;
  void updateChange(const size_t currentState, const ZoomFilterMode currentFilterMode);
  void stateChange(const uint32_t timeInState);
  void stateChange(const size_t index, const uint32_t timeInState);
  void filterModeChange();
  void filterModeChange(const ZoomFilterMode);
  void lockChange();
  void doIFS();
  void doDots();
  void doLines();
  void switchLines();
  void doStars();
  void doTentacles();
  void tentaclesDisabled();
  void doBigUpdate();
  void lastTimeGoomChange();
  void megaLentChange();
  void doNoise();
  void setLastNoiseFactor(const float val);
  void ifsRenew();
  void ifsIncrLessThanEqualZero();
  void ifsIncrGreaterThanZero();
  void changeLineColor();
  void doBlockyWavy();
  void doZoomFilterAlloOverexposed();

private:
  std::string songTitle;
  uint32_t startingState = 0;
  ZoomFilterMode startingFilterMode = ZoomFilterMode::_size;
  uint64_t startingSeed = 0;
  uint32_t lastState = 0;
  const ZoomFilterData* lastZoomFilterData = nullptr;
  uint64_t lastSeed = 0;

  uint32_t minTimeInUpdatesMs = 0;
  uint32_t maxTimeInUpdatesMs = 0;
  std::chrono::high_resolution_clock::time_point timeNowHiRes{};
  size_t stateAtMin;
  size_t stateAtMax;
  ZoomFilterMode filterModeAtMin;
  ZoomFilterMode filterModeAtMax;

  uint32_t numUpdates = 0;
  uint64_t totalTimeInUpdatesMs = 0;
  uint32_t totalStateChanges = 0;
  uint64_t totalStateDurations = 0;
  uint32_t totalFilterModeChanges = 0;
  uint32_t numLockChanges = 0;
  uint32_t numDoIFS = 0;
  uint32_t numDoDots = 0;
  uint32_t numDoLines = 0;
  uint32_t numDoStars = 0;
  uint32_t numDoTentacles = 0;
  uint32_t numDisabledTentacles = 0;
  uint32_t numBigUpdates = 0;
  uint32_t numLastTimeGoomChanges = 0;
  uint32_t numMegaLentChanges = 0;
  uint32_t numDoNoise = 0;
  uint32_t numIfsRenew = 0;
  uint32_t numIfsIncrLessThanEqualZero = 0;
  uint32_t numIfsIncrGreaterThanZero = 0;
  uint32_t numChangeLineColor = 0;
  uint32_t numSwitchLines = 0;
  uint32_t numBlockyWavy = 0;
  uint32_t numZoomFilterAllowOverexposed = 0;
  std::array<uint32_t, static_cast<size_t>(ZoomFilterMode::_size)> numFilterModeChanges{0};
  std::vector<uint32_t> numStateChanges;
  std::vector<uint64_t> stateDurations;
};

void GoomStats::reset()
{
  startingState = 0;
  startingFilterMode = ZoomFilterMode::_size;
  startingSeed = 0;
  lastState = 0;
  lastZoomFilterData = nullptr;
  lastSeed = 0;

  numUpdates = 0;
  totalTimeInUpdatesMs = 0;
  minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  maxTimeInUpdatesMs = 0;
  timeNowHiRes = std::chrono::high_resolution_clock::now();
  totalStateChanges = 0;
  totalStateDurations = 0;
  std::fill(numStateChanges.begin(), numStateChanges.end(), 0);
  std::fill(stateDurations.begin(), stateDurations.end(), 0);
  totalFilterModeChanges = 0;
  numFilterModeChanges.fill(0);
  numLockChanges = 0;
  numDoIFS = 0;
  numDoDots = 0;
  numDoLines = 0;
  numDoStars = 0;
  numDoTentacles = 0;
  numDisabledTentacles = 0;
  numBigUpdates = 0;
  numLastTimeGoomChanges = 0;
  numMegaLentChanges = 0;
  numDoNoise = 0;
  numIfsRenew = 0;
  numIfsIncrLessThanEqualZero = 0;
  numIfsIncrGreaterThanZero = 0;
  numChangeLineColor = 0;
  numSwitchLines = 0;
  numBlockyWavy = 0;
  numZoomFilterAllowOverexposed = 0;
}

void GoomStats::log(const StatsLogValueFunc logVal) const
{
  const constexpr char* module = "goom_core";

  logVal(module, "songTitle", songTitle);
  logVal(module, "startingState", startingState);
  logVal(module, "startingFilterMode", enumToString(startingFilterMode));
  logVal(module, "startingSeed", startingSeed);
  logVal(module, "lastState", lastState);
  logVal(module, "lastSeed", lastSeed);

  if (!lastZoomFilterData)
  {
    logVal(module, "lastZoomFilterData", 0u);
  }
  else
  {
    logVal(module, "lastZoomFilterData->mode", enumToString(lastZoomFilterData->mode));
    logVal(module, "lastZoomFilterData->vitesse", lastZoomFilterData->vitesse);
    logVal(module, "lastZoomFilterData->pertedec",
           static_cast<uint32_t>(lastZoomFilterData->pertedec));
    logVal(module, "lastZoomFilterData->middleX", lastZoomFilterData->middleX);
    logVal(module, "lastZoomFilterData->middleY", lastZoomFilterData->middleY);
    logVal(module, "lastZoomFilterData->reverse",
           static_cast<uint32_t>(lastZoomFilterData->reverse));
    logVal(module, "lastZoomFilterData->hPlaneEffect", lastZoomFilterData->hPlaneEffect);
    logVal(module, "lastZoomFilterData->vPlaneEffect", lastZoomFilterData->vPlaneEffect);
    logVal(module, "lastZoomFilterData->waveEffect",
           static_cast<uint32_t>(lastZoomFilterData->waveEffect));
    logVal(module, "lastZoomFilterData->hypercosEffect", lastZoomFilterData->hypercosEffect);
    logVal(module, "lastZoomFilterData->noisify",
           static_cast<uint32_t>(lastZoomFilterData->noisify));
    logVal(module, "lastZoomFilterData->noiseFactor",
           static_cast<float>(lastZoomFilterData->noiseFactor));
    logVal(module, "lastZoomFilterData->blockyWavy",
           static_cast<uint32_t>(lastZoomFilterData->blockyWavy));
    logVal(module, "lastZoomFilterData->waveFreqFactor", lastZoomFilterData->waveFreqFactor);
    logVal(module, "lastZoomFilterData->waveAmplitude", lastZoomFilterData->waveAmplitude);
    logVal(module, "lastZoomFilterData->waveEffectType",
           enumToString(lastZoomFilterData->waveEffectType));
    logVal(module, "lastZoomFilterData->scrunchAmplitude", lastZoomFilterData->scrunchAmplitude);
    logVal(module, "lastZoomFilterData->speedwayAmplitude", lastZoomFilterData->speedwayAmplitude);
    logVal(module, "lastZoomFilterData->amuletteAmplitude", lastZoomFilterData->amuletteAmplitude);
    logVal(module, "lastZoomFilterData->crystalBallAmplitude",
           lastZoomFilterData->crystalBallAmplitude);
    logVal(module, "lastZoomFilterData->hypercosFreq", lastZoomFilterData->hypercosFreq);
    logVal(module, "lastZoomFilterData->hypercosAmplitude", lastZoomFilterData->hypercosAmplitude);
    logVal(module, "lastZoomFilterData->hPlaneEffectAmplitude",
           lastZoomFilterData->hPlaneEffectAmplitude);
    logVal(module, "lastZoomFilterData->vPlaneEffectAmplitude",
           lastZoomFilterData->vPlaneEffectAmplitude);
  }

  logVal(module, "numUpdates", numUpdates);
  const int32_t avTimeInUpdateMs = std::lround(
      numUpdates == 0 ? -1.0
                      : static_cast<float>(totalTimeInUpdatesMs) / static_cast<float>(numUpdates));
  logVal(module, "avTimeInUpdateMs", avTimeInUpdateMs);
  logVal(module, "minTimeInUpdatesMs", minTimeInUpdatesMs);
  logVal(module, "stateAtMin", stateAtMin);
  logVal(module, "filterModeAtMin", enumToString(filterModeAtMin));
  logVal(module, "maxTimeInUpdatesMs", maxTimeInUpdatesMs);
  logVal(module, "stateAtMax", stateAtMax);
  logVal(module, "filterModeAtMax", enumToString(filterModeAtMax));
  logVal(module, "totalStateChanges", totalStateChanges);
  const float avStateDuration = totalStateChanges == 0 ? -1.0F
                                                       : static_cast<float>(totalStateDurations) /
                                                             static_cast<float>(totalStateChanges);
  logVal(module, "averageStateDuration", avStateDuration);
  for (size_t i = 0; i < numStateChanges.size(); i++)
  {
    logVal(module, "numState_" + std::to_string(i) + "_Changes", numStateChanges[i]);
  }
  for (size_t i = 0; i < stateDurations.size(); i++)
  {
    const float avStateDuration =
        numStateChanges[i] == 0
            ? -1.0F
            : static_cast<float>(stateDurations[i]) / static_cast<float>(numStateChanges[i]);
    logVal(module, "averageState_" + std::to_string(i) + "_Duration", avStateDuration);
  }
  logVal(module, "totalFilterModeChanges", totalFilterModeChanges);
  for (size_t i = 0; i < numFilterModeChanges.size(); i++)
  {
    logVal(module, "numFilterMode_" + enumToString(static_cast<ZoomFilterMode>(i)) + "_Changes",
           numFilterModeChanges[i]);
  }
  logVal(module, "numLockChanges", numLockChanges);
  logVal(module, "numDoIFS", numDoIFS);
  logVal(module, "numDoDots", numDoDots);
  logVal(module, "numDoLines", numDoLines);
  logVal(module, "numDoStars", numDoStars);
  logVal(module, "numDoTentacles", numDoTentacles);
  logVal(module, "numDisabledTentacles", numDisabledTentacles);
  logVal(module, "numLastTimeGoomChanges", numLastTimeGoomChanges);
  logVal(module, "numMegaLentChanges", numMegaLentChanges);
  logVal(module, "numDoNoise", numDoNoise);
  logVal(module, "numIfsRenew", numIfsRenew);
  logVal(module, "numIfsIncrLessThanEqualZero", numIfsIncrLessThanEqualZero);
  logVal(module, "numIfsIncrGreaterThanZero", numIfsIncrGreaterThanZero);
  logVal(module, "numChangeLineColor", numChangeLineColor);
  logVal(module, "numSwitchLines", numSwitchLines);
  logVal(module, "numBlockyWavy", numBlockyWavy);
  logVal(module, "numZoomFilterAllowOverexposed", numZoomFilterAllowOverexposed);
}

void GoomStats::setSongTitle(const std::string& s)
{
  songTitle = s;
}

void GoomStats::setStateStartValue(const uint32_t stateIndex)
{
  startingState = stateIndex;
}

void GoomStats::setZoomFilterStartValue(const ZoomFilterMode filterMode)
{
  startingFilterMode = filterMode;
}

void GoomStats::setSeedStartValue(const uint64_t seed)
{
  startingSeed = seed;
}

void GoomStats::setStateLastValue(const uint32_t stateIndex)
{
  lastState = stateIndex;
}

void GoomStats::setZoomFilterLastValue(const ZoomFilterData* filterData)
{
  lastZoomFilterData = filterData;
}

void GoomStats::setSeedLastValue(const uint64_t seed)
{
  lastSeed = seed;
}

inline void GoomStats::updateChange(const size_t currentState,
                                    const ZoomFilterMode currentFilterMode)
{
  const auto timeNow = std::chrono::high_resolution_clock::now();
  if (numUpdates > 0)
  {
    using ms = std::chrono::milliseconds;
    const ms diff = std::chrono::duration_cast<ms>(timeNow - timeNowHiRes);
    const uint32_t timeInUpdateMs = static_cast<uint32_t>(diff.count());
    if (timeInUpdateMs < minTimeInUpdatesMs)
    {
      minTimeInUpdatesMs = timeInUpdateMs;
      stateAtMin = currentState;
      filterModeAtMin = currentFilterMode;
    }
    else if (timeInUpdateMs > maxTimeInUpdatesMs)
    {
      maxTimeInUpdatesMs = timeInUpdateMs;
      stateAtMax = currentState;
      filterModeAtMax = currentFilterMode;
    }
    totalTimeInUpdatesMs += timeInUpdateMs;
  }
  timeNowHiRes = timeNow;

  numUpdates++;
}

inline void GoomStats::stateChange(const uint32_t timeInState)
{
  totalStateChanges++;
  totalStateDurations += timeInState;
}

inline void GoomStats::stateChange(const size_t index, const uint32_t timeInState)
{
  if (index >= numStateChanges.size())
  {
    for (size_t i = numStateChanges.size(); i <= index; i++)
    {
      numStateChanges.push_back(0);
    }
  }
  numStateChanges[index]++;

  if (index >= stateDurations.size())
  {
    for (size_t i = stateDurations.size(); i <= index; i++)
    {
      stateDurations.push_back(0);
    }
  }
  stateDurations[index] += timeInState;
}

inline void GoomStats::filterModeChange()
{
  totalFilterModeChanges++;
}

inline void GoomStats::filterModeChange(const ZoomFilterMode mode)
{
  numFilterModeChanges.at(static_cast<size_t>(mode))++;
}

inline void GoomStats::lockChange()
{
  numLockChanges++;
}

inline void GoomStats::doIFS()
{
  numDoIFS++;
}

inline void GoomStats::doDots()
{
  numDoDots++;
}

inline void GoomStats::doLines()
{
  numDoLines++;
}

inline void GoomStats::doStars()
{
  numDoStars++;
}

inline void GoomStats::doTentacles()
{
  numDoTentacles++;
}

inline void GoomStats::tentaclesDisabled()
{
  numDisabledTentacles++;
}

inline void GoomStats::doBigUpdate()
{
  numBigUpdates++;
}

inline void GoomStats::lastTimeGoomChange()
{
  numLastTimeGoomChanges++;
}

inline void GoomStats::megaLentChange()
{
  numMegaLentChanges++;
}

inline void GoomStats::doNoise()
{
  numDoNoise++;
}

inline void GoomStats::ifsRenew()
{
  numIfsRenew++;
}

inline void GoomStats::ifsIncrLessThanEqualZero()
{
  numIfsIncrLessThanEqualZero++;
}

inline void GoomStats::ifsIncrGreaterThanZero()
{
  numIfsIncrGreaterThanZero++;
}

inline void GoomStats::changeLineColor()
{
  numChangeLineColor++;
}

inline void GoomStats::switchLines()
{
  numSwitchLines++;
}

inline void GoomStats::doBlockyWavy()
{
  numBlockyWavy++;
}

inline void GoomStats::doZoomFilterAlloOverexposed()
{
  numZoomFilterAllowOverexposed++;
}

constexpr int32_t stopSpeed = 128;
// TODO: put that as variable in PluginInfo
constexpr int32_t timeBetweenChange = 300;

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
  explicit GoomImageBuffers(const uint16_t resx, const uint16_t resy);
  ~GoomImageBuffers() noexcept;

  Pixel* getP1() const { return p1; }
  Pixel* getP2() const { return p2; }
  uint32_t* getOutputBuff() const { return outputBuff; }
  void setOutputBuff(uint32_t* val) { outputBuff = val; }

  void rotateBuffers();

private:
  static constexpr size_t numBuffs = 5;
  const std::vector<Pixel*> buffs;
  Pixel* p1;
  Pixel* p2;
  uint32_t* outputBuff = nullptr;
  size_t nextBuff;
  static std::vector<Pixel*> getPixelBuffs(const uint16_t resx, const uint16_t resy);
};

std::vector<Pixel*> GoomImageBuffers::getPixelBuffs(const uint16_t resx, const uint16_t resy)
{
  std::vector<Pixel*> newBuffs(numBuffs);
  for (auto& b : newBuffs)
  {
    b = new Pixel[static_cast<size_t>(resx) * static_cast<size_t>(resy)]{};
  }
  return newBuffs;
}

GoomImageBuffers::GoomImageBuffers(const uint16_t resx, const uint16_t resy)
  : buffs{getPixelBuffs(resx, resy)}, p1{buffs[0]}, p2{buffs[1]}, nextBuff{2}
{
}

GoomImageBuffers::~GoomImageBuffers() noexcept
{
  for (auto& b : buffs)
  {
    delete[] b;
  }
}

inline void GoomImageBuffers::rotateBuffers()
{
  p1 = p2;
  p2 = buffs[nextBuff];
  nextBuff++;
  if (nextBuff >= buffs.size())
  {
    nextBuff = 0;
  }
}

struct GoomMessage
{
  std::string message = "";
  uint32_t numberOfLinesInMessage = 0;
  uint32_t affiche = 0;
};


struct GoomVisualFx
{
  explicit GoomVisualFx(PluginInfo*);
  std::unique_ptr<ZoomFilterFx> zoomFilter_fx;
  std::unique_ptr<IfsFx> ifs_fx;
  std::unique_ptr<VisualFx> star_fx;
  std::unique_ptr<ConvolveFx> convolve_fx;
  std::unique_ptr<VisualFx> tentacles_fx;
  std::unique_ptr<VisualFx> goomDots;

  std::vector<VisualFx*> list;
};

GoomVisualFx::GoomVisualFx(PluginInfo* goomInfo)
  : zoomFilter_fx{new ZoomFilterFx{goomInfo}},
    ifs_fx{new IfsFx{goomInfo}},
    star_fx{new FlyingStarsFx{goomInfo}},
    convolve_fx{new ConvolveFx{goomInfo}},
    tentacles_fx{new TentaclesFx{goomInfo}},
    goomDots{new GoomDots{goomInfo}},
    // clang-format off
    list{
      zoomFilter_fx.get(),
      ifs_fx.get(),
      star_fx.get(),
      convolve_fx.get(),
      tentacles_fx.get(),
      goomDots.get(),
    }
// clang-format on
{
}

class GoomControl::GoomControlImp
{
public:
  GoomControlImp() noexcept = delete;
  GoomControlImp(const uint16_t resx, const uint16_t resy, const int seed);
  ~GoomControlImp();
  void swap(GoomControl::GoomControlImp& other) noexcept = delete;

  void setScreenBuffer(uint32_t* buffer);

  void start();
  void finish();

  void update(const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN],
              const int forceMode,
              const float fps,
              const char* songTitle,
              const char* message);

private:
  std::unique_ptr<PluginInfo> goomInfo;
  GoomImageBuffers imageBuffers;
  GoomVisualFx visualFx;
  GoomStats stats{};
  GoomStates states{};
  GoomEvents goomEvent{};
  uint32_t timeInState = 0;
  uint32_t cycle = 0;
  std::unordered_set<GoomDrawable> curGDrawables;
  GoomMessage messageData;

  // Line Fx
  GMLine gmline1;
  GMLine gmline2;

  void initBuffers();
  bool changeFilterModeEventHappens();
  void setNextFilterMode();
  void changeState();
  void changeFilterMode();
  void changeMilieu();
  void bigNormalUpdate(ZoomFilterData** pzfd);
  void megaLentUpdate(ZoomFilterData** pzfd);

  void updateDecayRecay();

  // baisser regulierement la vitesse
  void regularlyLowerTheSpeed(ZoomFilterData** pzfd);
  void lowerSpeed(ZoomFilterData** pzfd);

  // on verifie qu'il ne se pas un truc interressant avec le son.
  void changeFilterModeIfMusicChanges(const int forceMode);

  // Changement d'effet de zoom !
  void changeZoomEffect(ZoomFilterData*, const int forceMode);

  void applyIfsIfRequired();
  void applyTentaclesIfRequired();
  void applyStarsIfRequired();

  void displayText(const char* songTitle, const char* message, const float fps);

#ifdef SHOW_STATE_TEXT_ON_SCREEN
  void displayStateText();
#endif

  void drawDotsIfRequired();

  void chooseGoomLine(float* param1,
                      float* param2,
                      Pixel* couleur,
                      LineType* mode,
                      float* amplitude,
                      const int far);

  // si on est dans un goom : afficher les lignes
  void displayLinesIfInAGoom(const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN]);
  void displayLines(const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN]);

  // arret demande
  void stopRequest();
  void stopIfRequested();

  // arret aleatore.. changement de mode de ligne..
  void stopRandomLineChangeMode();

  // Permet de forcer un effet.
  void forceFilterModeIfSet(ZoomFilterData**, const int forceMode);
  void forceFilterMode(const int forceMode, ZoomFilterData**);

  // arreter de decrémenter au bout d'un certain temps
  void stopDecrementingAfterAWhile(ZoomFilterData**);
  void stopDecrementing(ZoomFilterData**);

  // tout ceci ne sera fait qu'en cas de non-blocage
  void bigUpdateIfNotLocked(ZoomFilterData**);
  void bigUpdate(ZoomFilterData**);

  // gros frein si la musique est calme
  void bigBreakIfMusicIsCalm(ZoomFilterData**);
  void bigBreak(ZoomFilterData**);

  void updateMessage(const char* message);
};


GoomControl::GoomControl(const uint16_t resx, const uint16_t resy, const int seed)
  : controller{new GoomControlImp{resx, resy, seed}}
{
}

GoomControl::~GoomControl() noexcept
{
}

void GoomControl::setScreenBuffer(uint32_t* buffer)
{
  controller->setScreenBuffer(buffer);
}

void GoomControl::start()
{
  controller->start();
}

void GoomControl::finish()
{
  controller->finish();
}

void GoomControl::update(const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN],
                         const int forceMode,
                         const float fps,
                         const char* songTitle,
                         const char* message)
{
  controller->update(data, forceMode, fps, songTitle, message);
}

static const Pixel lRed = getRedLineColor();
static const Pixel lGreen = getGreenLineColor();
static const Pixel lBlack = getBlackLineColor();

GoomControl::GoomControlImp::GoomControlImp(const uint16_t resx,
                                            const uint16_t resy,
                                            const int seed)
  : goomInfo{new PluginInfo{resx, resy}},
    imageBuffers{resx, resy},
    visualFx{goomInfo.get()},
    gmline1{goomInfo.get(),
            resx,
            resy,
            LineType::hline,
            static_cast<float>(resy),
            lBlack,
            LineType::circle,
            0.4f * static_cast<float>(resy),
            lGreen},
    gmline2{goomInfo.get(),
            resx,
            resy,
            LineType::hline,
            0,
            lBlack,
            LineType::circle,
            0.2f * static_cast<float>(resy),
            lRed}
{
  logDebug("Initialize goom: resx = {}, resy = {}, seed = {}.", resx, resy, seed);

  if (seed > 0)
  {
    setRandSeed(static_cast<uint64_t>(seed));
  }

  gfont_load();
}

GoomControl::GoomControlImp::~GoomControlImp()
{
}

void GoomControl::GoomControlImp::setScreenBuffer(uint32_t* buffer)
{
  imageBuffers.setOutputBuff(buffer);
}

inline bool GoomControl::GoomControlImp::changeFilterModeEventHappens()
{
  // If we're in amulette mode and the state contains tentacles,
  // then get out with a different probability.
  // (Rationale: get tentacles going earlier with another mode.)
  if ((goomInfo->update.zoomFilterData.mode == ZoomFilterMode::amuletteMode) &&
      states.getCurrentDrawables().contains(GoomDrawable::tentacles))
  {
    return goomEvent.happens(GoomEvent::changeFilterFromAmuletteMode);
  }

  return goomEvent.happens(GoomEvent::changeFilterMode);
}

void GoomControl::GoomControlImp::start()
{
  timeInState = 0;
  changeState();
  changeFilterMode();
  goomEvent.setGoomInfo(goomInfo.get());

  curGDrawables = states.getCurrentDrawables();
  setNextFilterMode();
  goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width;
  goomInfo->update.zoomFilterData.middleY = goomInfo->screen.height;

  stats.reset();
  stats.setStateStartValue(states.getCurrentStateIndex());
  stats.setZoomFilterStartValue(goomInfo->update.zoomFilterData.mode);
  stats.setSeedStartValue(getRandSeed());

  for (auto& v : visualFx.list)
  {
    v->start();
  }
}

static void logStatsValue(const std::string& module,
                          const std::string& name,
                          const StatsLogValue& value)
{
  std::visit(LogStatsVisitor{module, name}, value);
}

void GoomControl::GoomControlImp::finish()
{
  for (auto& v : visualFx.list)
  {
    v->log(logStatsValue);
  }

  stats.setStateLastValue(states.getCurrentStateIndex());
  stats.setZoomFilterLastValue(&goomInfo->update.zoomFilterData);
  stats.setSeedLastValue(getRandSeed());

  stats.log(logStatsValue);

  for (auto& v : visualFx.list)
  {
    v->finish();
  }
}

void GoomControl::GoomControlImp::update(const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN],
                                         const int forceMode,
                                         const float fps,
                                         const char* songTitle,
                                         const char* message)
{
  stats.updateChange(states.getCurrentStateIndex(), goomInfo->update.zoomFilterData.mode);

  timeInState++;

  // elargissement de l'intervalle d'évolution des points
  // ! calcul du deplacement des petits points ...

  logDebug("sound getTimeSinceLastGoom() = {}", goomInfo->getSoundInfo().getTimeSinceLastGoom());

  /* ! etude du signal ... */
  goomInfo->processSoundSample(data);

  updateDecayRecay();

  applyIfsIfRequired();

  drawDotsIfRequired();

  /* par défaut pas de changement de zoom */
  ZoomFilterData* pzfd = nullptr;
  if (forceMode != 0)
  {
    logDebug("forceMode = {}\n", forceMode);
  }

  goomInfo->update.lockvar--;
  if (goomInfo->update.lockvar < 0)
  {
    goomInfo->update.lockvar = 0;
  }
  stats.lockChange();
  /* note pour ceux qui n'ont pas suivis : le lockvar permet d'empecher un */
  /* changement d'etat du plugin juste apres un autre changement d'etat. oki */
  // -- Note for those who have not followed: the lockvar prevents a change
  // of state of the plugin just after another change of state.

  changeFilterModeIfMusicChanges(forceMode);

  bigUpdateIfNotLocked(&pzfd);

  bigBreakIfMusicIsCalm(&pzfd);

  regularlyLowerTheSpeed(&pzfd);

  stopDecrementingAfterAWhile(&pzfd);

  forceFilterModeIfSet(&pzfd, forceMode);

  changeZoomEffect(pzfd, forceMode);

  // Zoom here!
  visualFx.zoomFilter_fx->zoomFilterFastRGB(imageBuffers.getP1(), imageBuffers.getP2(), pzfd,
                                            goomInfo->update.switchIncr,
                                            goomInfo->update.switchMult);

  applyTentaclesIfRequired();

  applyStarsIfRequired();

#ifdef SHOW_STATE_TEXT_ON_SCREEN
  displayStateText();
#endif
  displayText(songTitle, message, fps);

  // Gestion du Scope - Scope management
  stopIfRequested();
  stopRandomLineChangeMode();
  displayLinesIfInAGoom(data);

  // affichage et swappage des buffers...
  visualFx.convolve_fx->convolve(imageBuffers.getP1(), imageBuffers.getOutputBuff());
  imageBuffers.rotateBuffers();

  cycle++;

  logDebug("About to return.");
}

void GoomControl::GoomControlImp::chooseGoomLine(
    float* param1, float* param2, Pixel* couleur, LineType* mode, float* amplitude, const int far)
{
  *amplitude = 1.0f;
  *mode = goomEvent.getRandomLineTypeEvent();

  switch (*mode)
  {
    case LineType::circle:
      if (far)
      {
        *param1 = *param2 = 0.47f;
        *amplitude = 0.8f;
        break;
      }
      if (goomEvent.happens(GoomEvent::changeLineCircleAmplitude))
      {
        *param1 = *param2 = 0;
        *amplitude = 3.0f;
      }
      else if (goomEvent.happens(GoomEvent::changeLineCircleParams))
      {
        *param1 = 0.40f * goomInfo->screen.height;
        *param2 = 0.22f * goomInfo->screen.height;
      }
      else
      {
        *param1 = *param2 = goomInfo->screen.height * 0.35F;
      }
      break;
    case LineType::hline:
      if (goomEvent.happens(GoomEvent::changeHLineParams) || far)
      {
        *param1 = goomInfo->screen.height / 7.0F;
        *param2 = 6.0f * goomInfo->screen.height / 7.0F;
      }
      else
      {
        *param1 = *param2 = goomInfo->screen.height / 2.0f;
        *amplitude = 2.0f;
      }
      break;
    case LineType::vline:
      if (goomEvent.happens(GoomEvent::changeVLineParams) || far)
      {
        *param1 = goomInfo->screen.width / 7.0f;
        *param2 = 6.0f * goomInfo->screen.width / 7.0f;
      }
      else
      {
        *param1 = *param2 = goomInfo->screen.width / 2.0f;
        *amplitude = 1.5f;
      }
      break;
    default:
      throw std::logic_error("Unknown LineTypes enum.");
  }

  stats.changeLineColor();
  *couleur = gmline1.getRandomLineColor();
}

void GoomControl::GoomControlImp::changeFilterModeIfMusicChanges(const int forceMode)
{
  logDebug("forceMode = {}", forceMode);
  if (forceMode == -1)
  {
    return;
  }

  logDebug("sound getTimeSinceLastGoom() = {}, goomInfo->update.cyclesSinceLastChange = {}",
           goomInfo->getSoundInfo().getTimeSinceLastGoom(), goomInfo->update.cyclesSinceLastChange);
  if ((goomInfo->getSoundInfo().getTimeSinceLastGoom() == 0) ||
      (goomInfo->update.cyclesSinceLastChange > timeBetweenChange) || (forceMode > 0))
  {
    logDebug("Try to change the filter mode.");
    if (changeFilterModeEventHappens())
    {
      changeFilterMode();
    }
  }
  logDebug("sound getTimeSinceLastGoom() = {}", goomInfo->getSoundInfo().getTimeSinceLastGoom());
}

void GoomControl::GoomControlImp::setNextFilterMode()
{
  //  goomInfo->update.zoomFilterData.vitesse = 127;
  //  goomInfo->update.zoomFilterData.middleX = 16;
  //  goomInfo->update.zoomFilterData.middleY = 1;
  goomInfo->update.zoomFilterData.reverse = true;
  //  goomInfo->update.zoomFilterData.hPlaneEffect = 0;
  //  goomInfo->update.zoomFilterData.vPlaneEffect = 0;
  goomInfo->update.zoomFilterData.waveEffect = false;
  goomInfo->update.zoomFilterData.hypercosEffect = false;
  //  goomInfo->update.zoomFilterData.noisify = false;
  //  goomInfo->update.zoomFilterData.noiseFactor = 1;
  //  goomInfo->update.zoomFilterData.blockyWavy = false;
  goomInfo->update.zoomFilterData.waveFreqFactor = ZoomFilterData::defaultWaveFreqFactor;
  goomInfo->update.zoomFilterData.waveAmplitude = ZoomFilterData::defaultWaveAmplitude;
  goomInfo->update.zoomFilterData.waveEffectType = ZoomFilterData::defaultWaveEffectType;
  goomInfo->update.zoomFilterData.scrunchAmplitude = ZoomFilterData::defaultScrunchAmplitude;
  goomInfo->update.zoomFilterData.speedwayAmplitude = ZoomFilterData::defaultSpeedwayAmplitude;
  goomInfo->update.zoomFilterData.amuletteAmplitude = ZoomFilterData::defaultAmuletteAmplitude;
  goomInfo->update.zoomFilterData.crystalBallAmplitude =
      ZoomFilterData::defaultCrystalBallAmplitude;
  goomInfo->update.zoomFilterData.hypercosFreq = ZoomFilterData::defaultHypercosFreq;
  goomInfo->update.zoomFilterData.hypercosAmplitude = ZoomFilterData::defaultHypercosAmplitude;
  goomInfo->update.zoomFilterData.hPlaneEffectAmplitude =
      ZoomFilterData::defaultHPlaneEffectAmplitude;
  goomInfo->update.zoomFilterData.vPlaneEffectAmplitude =
      ZoomFilterData::defaultVPlaneEffectAmplitude;

  switch (goomEvent.getRandomFilterEvent())
  {
    case GoomFilterEvent::yOnlyMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::yOnlyMode;
      break;
    case GoomFilterEvent::speedwayMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::speedwayMode;
      goomInfo->update.zoomFilterData.speedwayAmplitude = getRandInRange(
          ZoomFilterData::minSpeedwayAmplitude, ZoomFilterData::maxSpeedwayAmplitude);
      break;
    case GoomFilterEvent::normalMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::normalMode;
      break;
    case GoomFilterEvent::waveModeWithHyperCosEffect:
      goomInfo->update.zoomFilterData.hypercosEffect =
          goomEvent.happens(GoomEvent::hypercosEffectOnWithWaveMode);
      [[fallthrough]];
    case GoomFilterEvent::waveMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::waveMode;
      goomInfo->update.zoomFilterData.reverse = false;
      goomInfo->update.zoomFilterData.waveEffect =
          goomEvent.happens(GoomEvent::waveEffectOnWithWaveMode);
      if (goomEvent.happens(GoomEvent::changeVitesseWithWaveMode))
      {
        goomInfo->update.zoomFilterData.vitesse =
            (goomInfo->update.zoomFilterData.vitesse + 127) >> 1;
      }
      goomInfo->update.zoomFilterData.waveEffectType =
          static_cast<ZoomFilterData::WaveEffect>(getRandInRange(0, 2));
      if (goomEvent.happens(GoomEvent::allowStrangeWaveValues))
      {
        // BUG HERE - wrong ranges - BUT GIVES GOOD AFFECT
        goomInfo->update.zoomFilterData.waveAmplitude = getRandInRange(
            ZoomFilterData::minLargeWaveAmplitude, ZoomFilterData::maxLargeWaveAmplitude);
        goomInfo->update.zoomFilterData.waveFreqFactor = getRandInRange(
            ZoomFilterData::minWaveSmallFreqFactor, ZoomFilterData::maxWaveSmallFreqFactor);
      }
      else
      {
        goomInfo->update.zoomFilterData.waveAmplitude =
            getRandInRange(ZoomFilterData::minWaveAmplitude, ZoomFilterData::maxWaveAmplitude);
        goomInfo->update.zoomFilterData.waveFreqFactor =
            getRandInRange(ZoomFilterData::minWaveFreqFactor, ZoomFilterData::maxWaveFreqFactor);
      }
      break;
    case GoomFilterEvent::crystalBallMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::crystalBallMode;
      goomInfo->update.zoomFilterData.crystalBallAmplitude = getRandInRange(
          ZoomFilterData::minCrystalBallAmplitude, ZoomFilterData::maxCrystalBallAmplitude);
      break;
    case GoomFilterEvent::crystalBallModeWithEffects:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::crystalBallMode;
      goomInfo->update.zoomFilterData.waveEffect =
          goomEvent.happens(GoomEvent::waveEffectOnWithCrystalBallMode);
      goomInfo->update.zoomFilterData.hypercosEffect =
          goomEvent.happens(GoomEvent::hypercosEffectOnWithCrystalBallMode);
      goomInfo->update.zoomFilterData.crystalBallAmplitude = getRandInRange(
          ZoomFilterData::minCrystalBallAmplitude, ZoomFilterData::maxCrystalBallAmplitude);
      break;
    case GoomFilterEvent::amuletteMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::amuletteMode;
      goomInfo->update.zoomFilterData.amuletteAmplitude = getRandInRange(
          ZoomFilterData::minAmuletteAmplitude, ZoomFilterData::maxAmuletteAmplitude);
      break;
    case GoomFilterEvent::waterMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::waterMode;
      break;
    case GoomFilterEvent::scrunchMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::scrunchMode;
      goomInfo->update.zoomFilterData.scrunchAmplitude =
          getRandInRange(ZoomFilterData::minScrunchAmplitude, ZoomFilterData::maxScrunchAmplitude);
      break;
    case GoomFilterEvent::scrunchModeWithEffects:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::scrunchMode;
      goomInfo->update.zoomFilterData.waveEffect = true;
      goomInfo->update.zoomFilterData.hypercosEffect = true;
      goomInfo->update.zoomFilterData.scrunchAmplitude =
          getRandInRange(ZoomFilterData::minScrunchAmplitude, ZoomFilterData::maxScrunchAmplitude);
      break;
    case GoomFilterEvent::hyperCos1Mode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::hyperCos1Mode;
      goomInfo->update.zoomFilterData.hypercosEffect =
          goomEvent.happens(GoomEvent::hypercosEffectOnWithHyperCos1Mode);
      break;
    case GoomFilterEvent::hyperCos2Mode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::hyperCos2Mode;
      goomInfo->update.zoomFilterData.hypercosEffect =
          goomEvent.happens(GoomEvent::hypercosEffectOnWithHyperCos2Mode);
      break;
    default:
      throw std::logic_error("GoomFilterEvent not implemented.");
  }

  if (goomInfo->update.zoomFilterData.hypercosEffect)
  {
    goomInfo->update.zoomFilterData.hypercosFreq =
        getRandInRange(ZoomFilterData::minHypercosFreq, ZoomFilterData::maxHypercosFreq);
    goomInfo->update.zoomFilterData.hypercosAmplitude =
        getRandInRange(ZoomFilterData::minHypercosAmplitude, ZoomFilterData::maxHypercosAmplitude);
  }

  if (goomInfo->update.zoomFilterData.mode == ZoomFilterMode::amuletteMode)
  {
    curGDrawables.erase(GoomDrawable::tentacles);
    stats.tentaclesDisabled();
  }
  else
  {
    curGDrawables = states.getCurrentDrawables();
  }
}

void GoomControl::GoomControlImp::changeFilterMode()
{
  logDebug("Time to change the filter mode.");

  stats.filterModeChange();

  setNextFilterMode();

  stats.filterModeChange(goomInfo->update.zoomFilterData.mode);

  visualFx.ifs_fx->renew();
  stats.ifsRenew();
}

void GoomControl::GoomControlImp::changeState()
{
  const size_t oldStateIndex = states.getCurrentStateIndex();
  for (size_t numTry = 0; numTry < 10; numTry++)
  {
    states.doRandomStateChange();
    if (oldStateIndex != states.getCurrentStateIndex())
    {
      // Only pick a different state.
      break;
    }
  }

  curGDrawables = states.getCurrentDrawables();
  logDebug("Changed goom state to {}", states.getCurrentStateIndex());
  stats.stateChange(timeInState);
  stats.stateChange(states.getCurrentStateIndex(), timeInState);
  timeInState = 0;

  if (states.isCurrentlyDrawable(GoomDrawable::IFS))
  {
    if (goomEvent.happens(GoomEvent::ifsRenew))
    {
      visualFx.ifs_fx->renew();
      stats.ifsRenew();
    }
    if (goomInfo->update.ifs_incr <= 0)
    {
      goomInfo->update.recay_ifs = 5;
      goomInfo->update.ifs_incr = 11;
      stats.ifsIncrLessThanEqualZero();
      visualFx.ifs_fx->renew();
      stats.ifsRenew();
    }
  }
  else if ((goomInfo->update.ifs_incr > 0) && (goomInfo->update.decay_ifs <= 0))
  {
    goomInfo->update.decay_ifs = 100;
    stats.ifsIncrGreaterThanZero();
  }

  if (!states.isCurrentlyDrawable(GoomDrawable::scope))
  {
    goomInfo->update.stop_lines = 0xf000 & 5;
  }
  if (!states.isCurrentlyDrawable(GoomDrawable::farScope))
  {
    goomInfo->update.stop_lines = 0;
    goomInfo->update.lineMode = goomInfo->update.drawLinesDuration;
  }

  // Tentacles and amulette don't look so good together.
  if (states.isCurrentlyDrawable(GoomDrawable::tentacles) &&
      (goomInfo->update.zoomFilterData.mode == ZoomFilterMode::amuletteMode))
  {
    changeFilterMode();
  }
}

void GoomControl::GoomControlImp::changeMilieu()
{

  if ((goomInfo->update.zoomFilterData.mode == ZoomFilterMode::waterMode) ||
      (goomInfo->update.zoomFilterData.mode == ZoomFilterMode::yOnlyMode) ||
      (goomInfo->update.zoomFilterData.mode == ZoomFilterMode::amuletteMode))
  {
    goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width / 2;
    goomInfo->update.zoomFilterData.middleY = goomInfo->screen.height / 2;
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
        goomInfo->update.zoomFilterData.middleY = goomInfo->screen.height - 1;
        goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width / 2;
        break;
      case MiddlePointEvents::event2:
        goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width - 1;
        break;
      case MiddlePointEvents::event3:
        goomInfo->update.zoomFilterData.middleX = 1;
        break;
      case MiddlePointEvents::event4:
        goomInfo->update.zoomFilterData.middleY = goomInfo->screen.height / 2;
        goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width / 2;
        break;
      default:
        throw std::logic_error("Unknown MiddlePointEvents enum.");
    }
  }

  // clang-format off
  // @formatter:off
  enum class PlaneEffectEvents { event1, event2, event3, event4, event5, event6, event7, event8 };
  static const Weights<PlaneEffectEvents> planeEffectWeights{{
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

  switch (planeEffectWeights.getRandomWeighted())
  {
    case PlaneEffectEvents::event1:
      goomInfo->update.zoomFilterData.vPlaneEffect = getRandInRange(-2, +3);
      goomInfo->update.zoomFilterData.hPlaneEffect = getRandInRange(-2, +3);
      break;
    case PlaneEffectEvents::event2:
      goomInfo->update.zoomFilterData.vPlaneEffect = 0;
      goomInfo->update.zoomFilterData.hPlaneEffect = getRandInRange(-7, +8);
      break;
    case PlaneEffectEvents::event3:
      goomInfo->update.zoomFilterData.vPlaneEffect = getRandInRange(-5, +6);
      goomInfo->update.zoomFilterData.hPlaneEffect = -goomInfo->update.zoomFilterData.vPlaneEffect;
      break;
    case PlaneEffectEvents::event4:
      goomInfo->update.zoomFilterData.hPlaneEffect = static_cast<int>(getRandInRange(5u, 13u));
      goomInfo->update.zoomFilterData.vPlaneEffect = -goomInfo->update.zoomFilterData.hPlaneEffect;
      break;
    case PlaneEffectEvents::event5:
      goomInfo->update.zoomFilterData.vPlaneEffect = static_cast<int>(getRandInRange(5u, 13u));
      goomInfo->update.zoomFilterData.hPlaneEffect = -goomInfo->update.zoomFilterData.hPlaneEffect;
      break;
    case PlaneEffectEvents::event6:
      goomInfo->update.zoomFilterData.hPlaneEffect = 0;
      goomInfo->update.zoomFilterData.vPlaneEffect = getRandInRange(-9, +10);
      break;
    case PlaneEffectEvents::event7:
      goomInfo->update.zoomFilterData.hPlaneEffect = getRandInRange(-9, +10);
      goomInfo->update.zoomFilterData.vPlaneEffect = getRandInRange(-9, +10);
      break;
    case PlaneEffectEvents::event8:
      goomInfo->update.zoomFilterData.vPlaneEffect = 0;
      goomInfo->update.zoomFilterData.hPlaneEffect = 0;
      break;
    default:
      throw std::logic_error("Unknown MiddlePointEvents enum.");
  }
  goomInfo->update.zoomFilterData.hPlaneEffectAmplitude = getRandInRange(
      ZoomFilterData::minHPlaneEffectAmplitude, ZoomFilterData::maxHPlaneEffectAmplitude);
  // I think 'vPlaneEffectAmplitude' has to be the same as 'hPlaneEffectAmplitude' otherwise
  //   buffer breaking effects occur.
  goomInfo->update.zoomFilterData.vPlaneEffectAmplitude =
      goomInfo->update.zoomFilterData.hPlaneEffectAmplitude + getRandInRange(-0.0009F, 0.0009F);
  //  goomInfo->update.zoomFilterData.vPlaneEffectAmplitude = getRandInRange(
  //      ZoomFilterData::minVPlaneEffectAmplitude, ZoomFilterData::maxVPlaneEffectAmplitude);
}

void GoomControl::GoomControlImp::bigNormalUpdate(ZoomFilterData** pzfd)
{
  if (goomInfo->update.stateSelectionBlocker)
  {
    goomInfo->update.stateSelectionBlocker--;
  }
  else if (goomEvent.happens(GoomEvent::changeState))
  {
    goomInfo->update.stateSelectionBlocker = 3;
    changeState();
  }

  goomInfo->update.lockvar = 50;
  stats.lockChange();
  const int32_t newvit =
      stopSpeed + 1 -
      static_cast<int32_t>(3.5f * log10(goomInfo->getSoundInfo().getSpeed() * 60 + 1));
  // retablir le zoom avant..
  if ((goomInfo->update.zoomFilterData.reverse) && (!(cycle % 13)) &&
      goomEvent.happens(GoomEvent::filterReverseOffAndStopSpeed))
  {
    goomInfo->update.zoomFilterData.reverse = false;
    goomInfo->update.zoomFilterData.vitesse = stopSpeed - 2;
    goomInfo->update.lockvar = 75;
    stats.lockChange();
  }
  if (goomEvent.happens(GoomEvent::filterReverseOn))
  {
    goomInfo->update.zoomFilterData.reverse = true;
    goomInfo->update.lockvar = 100;
    stats.lockChange();
  }

  if (goomEvent.happens(GoomEvent::filterVitesseStopSpeedMinus1))
  {
    goomInfo->update.zoomFilterData.vitesse = stopSpeed - 1;
  }
  if (goomEvent.happens(GoomEvent::filterVitesseStopSpeedPlus1))
  {
    goomInfo->update.zoomFilterData.vitesse = stopSpeed + 1;
  }

  changeMilieu();

  if (goomEvent.happens(GoomEvent::turnOffNoise))
  {
    goomInfo->update.zoomFilterData.noisify = false;
  }
  else
  {
    goomInfo->update.zoomFilterData.noisify = true;
    goomInfo->update.zoomFilterData.noiseFactor = 1;
    goomInfo->update.lockvar *= 2;
    stats.lockChange();
    stats.doNoise();
  }

  if (!goomEvent.happens(GoomEvent::changeBlockyWavyToOn))
  {
    goomInfo->update.zoomFilterData.blockyWavy = false;
  }
  else
  {
    stats.doBlockyWavy();
    goomInfo->update.zoomFilterData.blockyWavy = true;
  }

  if (!goomEvent.happens(GoomEvent::changeZoomFilterAllowOverexposedToOn))
  {
    visualFx.zoomFilter_fx->setBuffSettings({.buffIntensity = 0.5, .allowOverexposed = false});
  }
  else
  {
    stats.doZoomFilterAlloOverexposed();
    visualFx.zoomFilter_fx->setBuffSettings({.buffIntensity = 0.5, .allowOverexposed = true});
  }

  if (goomInfo->update.zoomFilterData.mode == ZoomFilterMode::amuletteMode)
  {
    goomInfo->update.zoomFilterData.vPlaneEffect = 0;
    goomInfo->update.zoomFilterData.hPlaneEffect = 0;
    //    goomInfo->update.zoomFilterData.noisify = false;
  }

  if ((goomInfo->update.zoomFilterData.middleX == 1) ||
      (goomInfo->update.zoomFilterData.middleX == goomInfo->screen.width - 1))
  {
    goomInfo->update.zoomFilterData.vPlaneEffect = 0;
    if (goomEvent.happens(GoomEvent::filterZeroHPlaneEffect))
    {
      goomInfo->update.zoomFilterData.hPlaneEffect = 0;
    }
  }

  logDebug("newvit = {}, goomInfo->update.zoomFilterData.vitesse = {}", newvit,
           goomInfo->update.zoomFilterData.vitesse);
  if (newvit < goomInfo->update.zoomFilterData.vitesse)
  {
    // on accelere
    logDebug("newvit = {} < {} = goomInfo->update.zoomFilterData.vitesse", newvit,
             goomInfo->update.zoomFilterData.vitesse);
    *pzfd = &goomInfo->update.zoomFilterData;
    if (((newvit < (stopSpeed - 7)) && (goomInfo->update.zoomFilterData.vitesse < stopSpeed - 6) &&
         (cycle % 3 == 0)) ||
        goomEvent.happens(GoomEvent::filterChangeVitesseAndToggleReverse))
    {
      goomInfo->update.zoomFilterData.vitesse =
          stopSpeed - static_cast<int32_t>(getNRand(2)) + static_cast<int32_t>(getNRand(2));
      goomInfo->update.zoomFilterData.reverse = !goomInfo->update.zoomFilterData.reverse;
    }
    else
    {
      goomInfo->update.zoomFilterData.vitesse =
          (newvit + goomInfo->update.zoomFilterData.vitesse * 7) / 8;
    }
    goomInfo->update.lockvar += 50;
    stats.lockChange();
  }

  if (goomInfo->update.lockvar > 150)
  {
    goomInfo->update.switchIncr = goomInfo->update.switchIncrAmount;
    goomInfo->update.switchMult = 1.0f;
  }
}

void GoomControl::GoomControlImp::megaLentUpdate(ZoomFilterData** pzfd)
{
  logDebug("mega lent change");
  *pzfd = &goomInfo->update.zoomFilterData;
  goomInfo->update.zoomFilterData.vitesse = stopSpeed - 1;
  goomInfo->update.lockvar += 50;
  stats.lockChange();
  goomInfo->update.switchIncr = goomInfo->update.switchIncrAmount;
  goomInfo->update.switchMult = 1.0f;
}

void GoomControl::GoomControlImp::bigUpdate(ZoomFilterData** pzfd)
{
  stats.doBigUpdate();

  // reperage de goom (acceleration forte de l'acceleration du volume)
  //   -> coup de boost de la vitesse si besoin..
  logDebug("sound getTimeSinceLastGoom() = {}", goomInfo->getSoundInfo().getTimeSinceLastGoom());
  if (goomInfo->getSoundInfo().getTimeSinceLastGoom() == 0)
  {
    logDebug("sound getTimeSinceLastGoom() = 0.");
    stats.lastTimeGoomChange();
    bigNormalUpdate(pzfd);
  }

  // mode mega-lent
  if (goomEvent.happens(GoomEvent::changeToMegaLentMode))
  {
    stats.megaLentChange();
    megaLentUpdate(pzfd);
  }
}

/* Changement d'effet de zoom !
 */
void GoomControl::GoomControlImp::changeZoomEffect(ZoomFilterData* pzfd, const int forceMode)
{
  if (!goomEvent.happens(GoomEvent::changeBlockyWavyToOn))
  {
    goomInfo->update.zoomFilterData.blockyWavy = false;
  }
  else
  {
    goomInfo->update.zoomFilterData.blockyWavy = true;
    stats.doBlockyWavy();
  }

  if (!goomEvent.happens(GoomEvent::changeZoomFilterAllowOverexposedToOn))
  {
    visualFx.zoomFilter_fx->setBuffSettings({.buffIntensity = 0.5, .allowOverexposed = false});
  }
  else
  {
    stats.doZoomFilterAlloOverexposed();
    visualFx.zoomFilter_fx->setBuffSettings({.buffIntensity = 0.5, .allowOverexposed = true});
  }

  if (pzfd)
  {
    logDebug("pzfd != nullptr");

    goomInfo->update.cyclesSinceLastChange = 0;
    goomInfo->update.switchIncr = goomInfo->update.switchIncrAmount;

    int diff = goomInfo->update.zoomFilterData.vitesse - goomInfo->update.previousZoomSpeed;
    if (diff < 0)
    {
      diff = -diff;
    }

    if (diff > 2)
    {
      goomInfo->update.switchIncr *= (diff + 2) / 2;
    }
    goomInfo->update.previousZoomSpeed = goomInfo->update.zoomFilterData.vitesse;
    goomInfo->update.switchMult = 1.0f;

    if (((goomInfo->getSoundInfo().getTimeSinceLastGoom() == 0) &&
         (goomInfo->getSoundInfo().getTotalGoom() < 2)) ||
        (forceMode > 0))
    {
      goomInfo->update.switchIncr = 0;
      goomInfo->update.switchMult = goomInfo->update.switchMultAmount;

      visualFx.ifs_fx->renew();
      stats.ifsRenew();
    }
  }
  else
  {
    logDebug("pzfd = nullptr");
    logDebug("goomInfo->update.cyclesSinceLastChange = {}", goomInfo->update.cyclesSinceLastChange);
    if (goomInfo->update.cyclesSinceLastChange > timeBetweenChange)
    {
      logDebug("goomInfo->update.cyclesSinceLastChange = {} > {} = timeBetweenChange",
               goomInfo->update.cyclesSinceLastChange, timeBetweenChange);
      pzfd = &goomInfo->update.zoomFilterData;
      goomInfo->update.cyclesSinceLastChange = 0;
      visualFx.ifs_fx->renew();
      stats.ifsRenew();
    }
    else
    {
      goomInfo->update.cyclesSinceLastChange++;
    }
  }

  if (pzfd)
  {
    logDebug("pzfd->mode = {}", pzfd->mode);
  }
}

void GoomControl::GoomControlImp::applyTentaclesIfRequired()
{
  if (!curGDrawables.contains(GoomDrawable::tentacles))
  {
    visualFx.tentacles_fx->applyNoDraw();
    return;
  }

  logDebug("curGDrawables tentacles is set.");
  stats.doTentacles();
  visualFx.tentacles_fx->setBuffSettings(states.getCurrentBuffSettings(GoomDrawable::tentacles));
  visualFx.tentacles_fx->apply(imageBuffers.getP2(), imageBuffers.getP1());
}

void GoomControl::GoomControlImp::applyStarsIfRequired()
{
  if (!curGDrawables.contains(GoomDrawable::stars))
  {
    return;
  }

  logDebug("curGDrawables stars is set.");
  stats.doStars();
  visualFx.star_fx->setBuffSettings(states.getCurrentBuffSettings(GoomDrawable::stars));
  visualFx.star_fx->apply(imageBuffers.getP2(), imageBuffers.getP1());
}

#ifdef SHOW_STATE_TEXT_ON_SCREEN

void GoomControl::GoomControlImp::displayStateText()
{
  std::string message = "";

  message += std20::format("State: {}\n", states.getCurrentStateIndex());
  message += std20::format("Filter: {}\n", enumToString(goomInfo->update.zoomFilterData.mode));
  message += std20::format("switchIncr: {}\n", goomInfo->update.switchIncr);
  message += std20::format("switchIncrAmount: {}\n", goomInfo->update.switchIncrAmount);
  message += std20::format("switchMult: {}\n", goomInfo->update.switchMult);
  message += std20::format("switchMultAmount: {}\n", goomInfo->update.switchMultAmount);
  message += std20::format("previousZoomSpeed: {}\n", goomInfo->update.previousZoomSpeed);
  message += std20::format("vitesse: {}\n", goomInfo->update.zoomFilterData.vitesse);
  message += std20::format("pertedec: {}\n", goomInfo->update.zoomFilterData.pertedec);
  message += std20::format("reverse: {}\n", goomInfo->update.zoomFilterData.reverse);
  message += std20::format("hPlaneEffect: {}\n", goomInfo->update.zoomFilterData.hPlaneEffect);
  message += std20::format("vPlaneEffect: {}\n", goomInfo->update.zoomFilterData.vPlaneEffect);
  message += std20::format("hypercosEffect: {}\n", goomInfo->update.zoomFilterData.hypercosEffect);
  message += std20::format("middleX: {}\n", goomInfo->update.zoomFilterData.middleX);
  message += std20::format("middleY: {}\n", goomInfo->update.zoomFilterData.middleY);
  message += std20::format("noisify: {}\n", goomInfo->update.zoomFilterData.noisify);
  message += std20::format("noiseFactor: {}\n", goomInfo->update.zoomFilterData.noiseFactor);
  message += std20::format("cyclesSinceLastChange: {}\n", goomInfo->update.cyclesSinceLastChange);
  message += std20::format("lineMode: {}\n", goomInfo->update.lineMode);
  message += std20::format("lockvar: {}\n", goomInfo->update.lockvar);
  message += std20::format("stop_lines: {}\n", goomInfo->update.stop_lines);

  updateMessage(message.c_str());
}

#endif

void GoomControl::GoomControlImp::displayText(const char* songTitle,
                                              const char* message,
                                              const float fps)
{
  updateMessage(message);

  if (fps > 0)
  {
    char text[256];
    sprintf(text, "%2.0f fps", fps);
    goom_draw_text(imageBuffers.getP1(), goomInfo->screen.width, goomInfo->screen.height, 10, 24,
                   text, 1, 0);
  }

  if (songTitle)
  {
    stats.setSongTitle(songTitle);
    strncpy(goomInfo->update.titleText, songTitle, 1023);
    goomInfo->update.titleText[1023] = 0;
    goomInfo->update.timeOfTitleDisplay = 200;
  }

  if (goomInfo->update.timeOfTitleDisplay)
  {
    goom_draw_text(imageBuffers.getP1(), goomInfo->screen.width, goomInfo->screen.height,
                   static_cast<int>(goomInfo->screen.width / 2),
                   static_cast<int>(goomInfo->screen.height / 2 + 7), goomInfo->update.titleText,
                   static_cast<float>(190 - goomInfo->update.timeOfTitleDisplay) / 10.0f, 1);
    goomInfo->update.timeOfTitleDisplay--;
    if (goomInfo->update.timeOfTitleDisplay < 4)
    {
      goom_draw_text(imageBuffers.getP2(), goomInfo->screen.width, goomInfo->screen.height,
                     static_cast<int>(goomInfo->screen.width / 2),
                     static_cast<int>(goomInfo->screen.height / 2 + 7), goomInfo->update.titleText,
                     static_cast<float>(190 - goomInfo->update.timeOfTitleDisplay) / 10.0f, 1);
    }
  }
}

/*
 * Met a jour l'affichage du message defilant
 */
void GoomControl::GoomControlImp::updateMessage(const char* message)
{
  if (message)
  {
    messageData.message = message;
    const std::vector<std::string> msgLines = splitString(messageData.message, "\n");
    messageData.numberOfLinesInMessage = msgLines.size();
    messageData.affiche = 100 + 25 * messageData.numberOfLinesInMessage;
  }
  if (messageData.affiche)
  {
    const std::vector<std::string> msgLines = splitString(messageData.message, "\n");
    for (size_t i = 0; i < msgLines.size(); i++)
    {
      const uint32_t ypos =
          10 + messageData.affiche - (messageData.numberOfLinesInMessage - i) * 25;

      goom_draw_text(imageBuffers.getP1(), goomInfo->screen.width, goomInfo->screen.height, 50,
                     static_cast<int>(ypos), msgLines[i].c_str(), 1, 0);
    }
    messageData.affiche--;
  }
}

void GoomControl::GoomControlImp::stopRequest()
{
  logDebug("goomInfo->update.stop_lines = {},"
           " curGDrawables.contains(GoomDrawable::scope) = {}",
           goomInfo->update.stop_lines, curGDrawables.contains(GoomDrawable::scope));

  float param1 = 0;
  float param2 = 0;
  float amplitude = 0;
  Pixel couleur{};
  LineType mode;
  chooseGoomLine(&param1, &param2, &couleur, &mode, &amplitude, 1);
  couleur = getBlackLineColor();

  gmline1.switchGoomLines(mode, param1, amplitude, couleur);
  gmline2.switchGoomLines(mode, param2, amplitude, couleur);
  stats.switchLines();
  goomInfo->update.stop_lines &= 0x0fff;
}

/* arret aleatore.. changement de mode de ligne..
  */
void GoomControl::GoomControlImp::stopRandomLineChangeMode()
{
  if (goomInfo->update.lineMode != goomInfo->update.drawLinesDuration)
  {
    goomInfo->update.lineMode--;
    if (goomInfo->update.lineMode == -1)
    {
      goomInfo->update.lineMode = 0;
    }
  }
  else if ((cycle % 80 == 0) && goomEvent.happens(GoomEvent::reduceLineMode) &&
           goomInfo->update.lineMode)
  {
    goomInfo->update.lineMode--;
  }

  if ((cycle % 120 == 0) && goomEvent.happens(GoomEvent::updateLineMode) &&
      curGDrawables.contains(GoomDrawable::scope))
  {
    if (goomInfo->update.lineMode == 0)
    {
      goomInfo->update.lineMode = goomInfo->update.drawLinesDuration;
    }
    else if (goomInfo->update.lineMode == goomInfo->update.drawLinesDuration)
    {
      goomInfo->update.lineMode--;

      float param1 = 0;
      float param2 = 0;
      float amplitude = 0;
      Pixel couleur1{};
      LineType mode;
      chooseGoomLine(&param1, &param2, &couleur1, &mode, &amplitude, goomInfo->update.stop_lines);

      Pixel couleur2 = gmline2.getRandomLineColor();
      if (goomInfo->update.stop_lines)
      {
        goomInfo->update.stop_lines--;
        if (goomEvent.happens(GoomEvent::changeLineToBlack))
        {
          couleur2 = couleur1 = getBlackLineColor();
        }
      }

      logDebug("goomInfo->update.lineMode = {} == {} = goomInfo->update.drawLinesDuration",
               goomInfo->update.lineMode, goomInfo->update.drawLinesDuration);
      gmline1.switchGoomLines(mode, param1, amplitude, couleur1);
      gmline2.switchGoomLines(mode, param2, amplitude, couleur2);
      stats.switchLines();
    }
  }
}

void GoomControl::GoomControlImp::displayLines(
    const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN])
{
  if (!curGDrawables.contains(GoomDrawable::lines))
  {
    return;
  }

  logDebug("curGDrawables lines is set.");

  stats.doLines();

  gmline2.power = gmline1.power;

  gmline1.drawGoomLines(data[0], imageBuffers.getP1(), imageBuffers.getP2());
  gmline2.drawGoomLines(data[1], imageBuffers.getP1(), imageBuffers.getP2());

  if (((cycle % 121) == 9) && goomEvent.happens(GoomEvent::changeGoomLine) &&
      ((goomInfo->update.lineMode == 0) ||
       (goomInfo->update.lineMode == goomInfo->update.drawLinesDuration)))
  {
    logDebug("cycle % 121 etc.: goomInfo->cycle = {}, rand1_3 = ?", cycle);
    float param1 = 0;
    float param2 = 0;
    float amplitude = 0;
    Pixel couleur1{};
    LineType mode;
    chooseGoomLine(&param1, &param2, &couleur1, &mode, &amplitude, goomInfo->update.stop_lines);

    Pixel couleur2 = gmline2.getRandomLineColor();
    if (goomInfo->update.stop_lines)
    {
      goomInfo->update.stop_lines--;
      if (goomEvent.happens(GoomEvent::changeLineToBlack))
      {
        couleur2 = couleur1 = getBlackLineColor();
      }
    }
    gmline1.switchGoomLines(mode, param1, amplitude, couleur1);
    gmline2.switchGoomLines(mode, param2, amplitude, couleur2);
  }
}

void GoomControl::GoomControlImp::bigBreakIfMusicIsCalm(ZoomFilterData** pzfd)
{
  logDebug("sound getSpeed() = {:.2}, goomInfo->update.zoomFilterData.vitesse = {}, "
           "cycle = {}",
           goomInfo->getSoundInfo().getSpeed(), goomInfo->update.zoomFilterData.vitesse, cycle);
  if ((goomInfo->getSoundInfo().getSpeed() < 0.01f) &&
      (goomInfo->update.zoomFilterData.vitesse < (stopSpeed - 4)) && (cycle % 16 == 0))
  {
    logDebug("sound getSpeed() = {:.2}", goomInfo->getSoundInfo().getSpeed());
    bigBreak(pzfd);
  }
}

void GoomControl::GoomControlImp::bigBreak(ZoomFilterData** pzfd)
{
  *pzfd = &goomInfo->update.zoomFilterData;
  goomInfo->update.zoomFilterData.vitesse += 3;
}

void GoomControl::GoomControlImp::forceFilterMode(const int forceMode, ZoomFilterData** pzfd)
{
  *pzfd = &goomInfo->update.zoomFilterData;
  (*pzfd)->mode = static_cast<ZoomFilterMode>(forceMode - 1);
}

void GoomControl::GoomControlImp::lowerSpeed(ZoomFilterData** pzfd)
{
  *pzfd = &goomInfo->update.zoomFilterData;
  goomInfo->update.zoomFilterData.vitesse++;
}

void GoomControl::GoomControlImp::stopDecrementing(ZoomFilterData** pzfd)
{
  *pzfd = &goomInfo->update.zoomFilterData;
}

void GoomControl::GoomControlImp::updateDecayRecay()
{
  goomInfo->update.decay_ifs--;
  if (goomInfo->update.decay_ifs > 0)
  {
    goomInfo->update.ifs_incr += 2;
  }
  if (goomInfo->update.decay_ifs == 0)
  {
    goomInfo->update.ifs_incr = 0;
  }

  if (goomInfo->update.recay_ifs)
  {
    goomInfo->update.ifs_incr -= 2;
    goomInfo->update.recay_ifs--;
    if ((goomInfo->update.recay_ifs == 0) && (goomInfo->update.ifs_incr <= 0))
    {
      goomInfo->update.ifs_incr = 1;
    }
  }
}

void GoomControl::GoomControlImp::bigUpdateIfNotLocked(ZoomFilterData** pzfd)
{
  logDebug("goomInfo->update.lockvar = {}", goomInfo->update.lockvar);
  if (goomInfo->update.lockvar == 0)
  {
    logDebug("goomInfo->update.lockvar = 0");
    bigUpdate(pzfd);
  }
  logDebug("sound getTimeSinceLastGoom() = {}", goomInfo->getSoundInfo().getTimeSinceLastGoom());
}

void GoomControl::GoomControlImp::forceFilterModeIfSet(ZoomFilterData** pzfd, const int forceMode)
{
  constexpr size_t numFilterFx = static_cast<size_t>(ZoomFilterMode::_size);

  logDebug("forceMode = {}", forceMode);
  if ((forceMode > 0) && (size_t(forceMode) <= numFilterFx))
  {
    logDebug("forceMode = {} <= numFilterFx = {}.", forceMode, numFilterFx);
    forceFilterMode(forceMode, pzfd);
  }
  if (forceMode == -1)
  {
    pzfd = nullptr;
  }
}

void GoomControl::GoomControlImp::stopIfRequested()
{
  logDebug("goomInfo->update.stop_lines = {}, curGState->scope = {}", goomInfo->update.stop_lines,
           states.isCurrentlyDrawable(GoomDrawable::scope));
  if ((goomInfo->update.stop_lines & 0xf000) || !states.isCurrentlyDrawable(GoomDrawable::scope))
  {
    stopRequest();
  }
}

void GoomControl::GoomControlImp::displayLinesIfInAGoom(
    const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN])
{
  logDebug("goomInfo->update.lineMode = {} != 0 || sound getTimeSinceLastGoom() = {}",
           goomInfo->update.lineMode, goomInfo->getSoundInfo().getTimeSinceLastGoom());
  if ((goomInfo->update.lineMode != 0) || (goomInfo->getSoundInfo().getTimeSinceLastGoom() < 5))
  {
    logDebug("goomInfo->update.lineMode = {} != 0 || sound getTimeSinceLastGoom() = {} < 5",
             goomInfo->update.lineMode, goomInfo->getSoundInfo().getTimeSinceLastGoom());

    displayLines(data);
  }
}

void GoomControl::GoomControlImp::applyIfsIfRequired()
{
  logDebug("goomInfo->update.ifs_incr = {}", goomInfo->update.ifs_incr);
  if ((goomInfo->update.ifs_incr <= 0) || !curGDrawables.contains(GoomDrawable::IFS))
  {
    return;
  }

  logDebug("goomInfo->update.ifs_incr = {} > 0 and curGDrawables IFS is set",
           goomInfo->update.ifs_incr);
  stats.doIFS();
  visualFx.ifs_fx->setBuffSettings(states.getCurrentBuffSettings(GoomDrawable::IFS));
  visualFx.ifs_fx->apply(imageBuffers.getP2(), imageBuffers.getP1());
}

void GoomControl::GoomControlImp::regularlyLowerTheSpeed(ZoomFilterData** pzfd)
{
  logDebug("goomInfo->update.zoomFilterData.vitesse = {}, cycle = {}",
           goomInfo->update.zoomFilterData.vitesse, cycle);
  if ((cycle % 73 == 0) && (goomInfo->update.zoomFilterData.vitesse < (stopSpeed - 5)))
  {
    logDebug("cycle % 73 = 0 && dgoomInfo->update.zoomFilterData.vitesse = {} < {} - 5, ", cycle,
             goomInfo->update.zoomFilterData.vitesse, stopSpeed);
    lowerSpeed(pzfd);
  }
}

void GoomControl::GoomControlImp::stopDecrementingAfterAWhile(ZoomFilterData** pzfd)
{
  logDebug("cycle = {}, goomInfo->update.zoomFilterData.pertedec = {}", cycle,
           goomInfo->update.zoomFilterData.pertedec);
  if ((cycle % 101 == 0) && (goomInfo->update.zoomFilterData.pertedec == 7))
  {
    logDebug("cycle % 101 = 0 && dgoomInfo->update.zoomFilterData.pertedec = 7, ", cycle,
             goomInfo->update.zoomFilterData.vitesse);
    stopDecrementing(pzfd);
  }
}

void GoomControl::GoomControlImp::drawDotsIfRequired()
{
  if (!curGDrawables.contains(GoomDrawable::dots))
  {
    return;
  }

  logDebug("goomInfo->curGDrawables points is set.");
  stats.doDots();
  visualFx.goomDots->apply(imageBuffers.getP2(), imageBuffers.getP1());
  logDebug("sound getTimeSinceLastGoom() = {}", goomInfo->getSoundInfo().getTimeSinceLastGoom());
}

GoomEvents::GoomEvents() noexcept
  : filterWeights{{weightedFilterEvents.begin(), weightedFilterEvents.end()}},
    lineTypeWeights{{weightedLineEvents.begin(), weightedLineEvents.end()}}
{
}

void GoomEvents::setGoomInfo(PluginInfo* info)
{
  goomInfo = info;
}

inline bool GoomEvents::happens(const GoomEvent event) const
{
  const WeightedEvent& weightedEvent = weightedEvents[static_cast<size_t>(event)];
  return probabilityOfMInN(weightedEvent.m, weightedEvent.outOf);
}

inline GoomEvents::GoomFilterEvent GoomEvents::getRandomFilterEvent() const
{
  //////////////////return GoomFilterEvent::amuletteMode;
  //////////////////return GoomFilterEvent::waveModeWithHyperCosEffect;

  GoomEvents::GoomFilterEvent nextEvent = filterWeights.getRandomWeighted();
  for (size_t i = 0; i < 10; i++)
  {
    if (nextEvent != lastReturnedFilterEvent)
    {
      break;
    }
    nextEvent = filterWeights.getRandomWeighted();
  }
  lastReturnedFilterEvent = nextEvent;

  return nextEvent;
}

inline LineType GoomEvents::getRandomLineTypeEvent() const
{
  return lineTypeWeights.getRandomWeighted();
}

GoomStates::GoomStates() : weightedStates{getWeightedStates(states)}, currentStateIndex{0}
{
  doRandomStateChange();
}

inline bool GoomStates::isCurrentlyDrawable(const GoomDrawable drawable) const
{
  return getCurrentDrawables().contains(drawable);
}

inline size_t GoomStates::getCurrentStateIndex() const
{
  return currentStateIndex;
}

inline GoomStates::DrawablesState GoomStates::getCurrentDrawables() const
{
  GoomStates::DrawablesState currentDrawables{};
  for (const auto d : states[currentStateIndex].drawables)
  {
    currentDrawables.insert(d.fx);
  }
  return currentDrawables;
}

const FXBuffSettings GoomStates::getCurrentBuffSettings(const GoomDrawable theFx) const
{
  for (const auto& d : states[currentStateIndex].drawables)
  {
    if (d.fx == theFx)
    {
      return d.buffSettings;
    }
  }
  return FXBuffSettings{};
}

inline void GoomStates::doRandomStateChange()
{
  currentStateIndex = static_cast<size_t>(weightedStates.getRandomWeighted());
}

std::vector<std::pair<uint16_t, size_t>> GoomStates::getWeightedStates(
    const GoomStates::WeightedStatesArray& states)
{
  std::vector<std::pair<uint16_t, size_t>> weightedVals(states.size());
  for (size_t i = 0; i < states.size(); i++)
  {
    weightedVals[i] = std::make_pair(i, states[i].weight);
  }
  return weightedVals;
}

} // namespace goom
