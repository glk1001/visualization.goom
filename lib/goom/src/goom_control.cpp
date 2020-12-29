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
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/parallel_utils.h"
#include "goomutils/strutils.h"
#include "ifs_fx.h"
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

CEREAL_REGISTER_TYPE(goom::WritablePluginInfo);
CEREAL_REGISTER_POLYMORPHIC_RELATION(goom::PluginInfo, goom::WritablePluginInfo);

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
  GoomEvents(const GoomEvents&) = delete;
  GoomEvents& operator=(const GoomEvents&) = delete;

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

  bool happens(GoomEvent) const;
  GoomFilterEvent getRandomFilterEvent() const;
  LinesFx::LineType getRandomLineTypeEvent() const;

private:
  PluginInfo* goomInfo{};
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

  static constexpr std::array<std::pair<GoomFilterEvent, size_t>, numGoomFilterEvents> weightedFilterEvents{{
    { GoomFilterEvent::waveModeWithHyperCosEffect,  3 },
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

  static constexpr
  std::array<std::pair<LinesFx::LineType, size_t>, LinesFx::numLineTypes> weightedLineEvents{{
    { LinesFx::LineType::circle, 8 },
    { LinesFx::LineType::hline,  2 },
    { LinesFx::LineType::vline,  2 },
  }};
  // clang-format on
  const Weights<GoomFilterEvent> filterWeights;
  const Weights<LinesFx::LineType> lineTypeWeights;
};

using GoomEvent = GoomEvents::GoomEvent;
using GoomFilterEvent = GoomEvents::GoomFilterEvent;

class GoomStates
{
public:
  using DrawablesState = std::unordered_set<GoomDrawable>;

  GoomStates();

  [[nodiscard]] bool isCurrentlyDrawable(GoomDrawable) const;
  [[nodiscard]] size_t getCurrentStateIndex() const;
  [[nodiscard]] DrawablesState getCurrentDrawables() const;
  [[nodiscard]] FXBuffSettings getCurrentBuffSettings(GoomDrawable) const;

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
    .weight = 20,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
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
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.1, .allowOverexposed = true  } },
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
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.1, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 20,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = false } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.1, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 60,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 70,
    .drawables {{
      { .fx = GoomDrawable::IFS,       .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true } },
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
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = true } },
      { .fx = GoomDrawable::tentacles, .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true } },
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
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
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
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = true  } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 60,
    .drawables {{
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.2, .allowOverexposed = true  } },
      { .fx = GoomDrawable::lines,     .buffSettings = { .buffIntensity = 0.7, .allowOverexposed = true  } },
      { .fx = GoomDrawable::scope,     .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::farScope,  .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
    }},
  },
  {
    .weight = 60,
    .drawables {{
      { .fx = GoomDrawable::dots,      .buffSettings = { .buffIntensity = 0.5, .allowOverexposed = true  } },
      { .fx = GoomDrawable::stars,     .buffSettings = { .buffIntensity = 0.3, .allowOverexposed = true  } },
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
  GoomStats& operator=(const GoomStats&) = delete;

  void setSongTitle(const std::string& songTitle);
  void setStateStartValue(uint32_t stateIndex);
  void setZoomFilterStartValue(ZoomFilterMode filterMode);
  void setSeedStartValue(uint64_t seed);
  void setStateLastValue(uint32_t stateIndex);
  void setZoomFilterLastValue(const ZoomFilterData* filterData);
  void setSeedLastValue(uint64_t seed);
  void setNumThreadsUsedValue(size_t numThreads);
  void reset();
  void log(const StatsLogValueFunc&) const;
  void updateChange(size_t currentState, ZoomFilterMode currentFilterMode);
  void stateChange(uint32_t timeInState);
  void stateChange(size_t index, uint32_t timeInState);
  void filterModeChange();
  void filterModeChange(ZoomFilterMode);
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
  void setLastNoiseFactor(float val);
  void ifsRenew();
  void changeLineColor();
  void doBlockyWavy();
  void doZoomFilterAlloOverexposed();

private:
  std::string songTitle{};
  uint32_t startingState = 0;
  ZoomFilterMode startingFilterMode = ZoomFilterMode::_size;
  uint64_t startingSeed = 0;
  uint32_t lastState = 0;
  const ZoomFilterData* lastZoomFilterData = nullptr;
  uint64_t lastSeed = 0;
  size_t numThreadsUsed = 0;

  uint64_t totalTimeInUpdatesMs = 0;
  uint32_t minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  uint32_t maxTimeInUpdatesMs = 0;
  std::chrono::high_resolution_clock::time_point timeNowHiRes{};
  size_t stateAtMin = 0;
  size_t stateAtMax = 0;
  ZoomFilterMode filterModeAtMin = ZoomFilterMode::_null;
  ZoomFilterMode filterModeAtMax = ZoomFilterMode::_null;

  uint32_t numUpdates = 0;
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
  uint32_t numChangeLineColor = 0;
  uint32_t numSwitchLines = 0;
  uint32_t numBlockyWavy = 0;
  uint32_t numZoomFilterAllowOverexposed = 0;
  std::array<uint32_t, static_cast<size_t>(ZoomFilterMode::_size)> numFilterModeChanges{0};
  std::vector<uint32_t> numStateChanges{};
  std::vector<uint64_t> stateDurations{};
};

void GoomStats::reset()
{
  startingState = 0;
  startingFilterMode = ZoomFilterMode::_size;
  startingSeed = 0;
  lastState = 0;
  lastZoomFilterData = nullptr;
  lastSeed = 0;
  numThreadsUsed = 0;

  stateAtMin = 0;
  stateAtMax = 0;
  filterModeAtMin = ZoomFilterMode::_null;
  filterModeAtMax = ZoomFilterMode::_null;

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
  numChangeLineColor = 0;
  numSwitchLines = 0;
  numBlockyWavy = 0;
  numZoomFilterAllowOverexposed = 0;
}

void GoomStats::log(const StatsLogValueFunc& logVal) const
{
  const constexpr char* module = "goom_core";

  logVal(module, "songTitle", songTitle);
  logVal(module, "startingState", startingState);
  logVal(module, "startingFilterMode", enumToString(startingFilterMode));
  logVal(module, "startingSeed", startingSeed);
  logVal(module, "lastState", lastState);
  logVal(module, "lastSeed", lastSeed);
  logVal(module, "numThreadsUsed", numThreadsUsed);

  if (!lastZoomFilterData)
  {
    logVal(module, "lastZoomFilterData", 0u);
  }
  else
  {
    logVal(module, "lastZoomFilterData->mode", enumToString(lastZoomFilterData->mode));
    logVal(module, "lastZoomFilterData->vitesse", lastZoomFilterData->vitesse);
    logVal(module, "lastZoomFilterData->pertedec", static_cast<uint32_t>(ZoomFilterData::pertedec));
    logVal(module, "lastZoomFilterData->middleX", lastZoomFilterData->middleX);
    logVal(module, "lastZoomFilterData->middleY", lastZoomFilterData->middleY);
    logVal(module, "lastZoomFilterData->reverse",
           static_cast<uint32_t>(lastZoomFilterData->reverse));
    logVal(module, "lastZoomFilterData->hPlaneEffect", lastZoomFilterData->hPlaneEffect);
    logVal(module, "lastZoomFilterData->vPlaneEffect", lastZoomFilterData->vPlaneEffect);
    logVal(module, "lastZoomFilterData->waveEffect",
           static_cast<uint32_t>(lastZoomFilterData->waveEffect));
    logVal(module, "lastZoomFilterData->hypercosEffect",
           enumToString(lastZoomFilterData->hypercosEffect));
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
    const float avStateTime = numStateChanges[i] == 0 ? -1.0F
                                                      : static_cast<float>(stateDurations[i]) /
                                                            static_cast<float>(numStateChanges[i]);
    logVal(module, "averageState_" + std::to_string(i) + "_Duration", avStateTime);
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

void GoomStats::setNumThreadsUsedValue(const size_t n)
{
  numThreadsUsed = n;
}

inline void GoomStats::updateChange(const size_t currentState,
                                    const ZoomFilterMode currentFilterMode)
{
  const auto timeNow = std::chrono::high_resolution_clock::now();
  if (numUpdates > 0)
  {
    using ms = std::chrono::milliseconds;
    const ms diff = std::chrono::duration_cast<ms>(timeNow - timeNowHiRes);
    const auto timeInUpdateMs = static_cast<uint32_t>(diff.count());
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
  GoomImageBuffers() noexcept = default;
  explicit GoomImageBuffers(uint32_t resx, uint32_t resy) noexcept;
  ~GoomImageBuffers() noexcept;
  GoomImageBuffers(const GoomImageBuffers&) = delete;
  GoomImageBuffers& operator=(const GoomImageBuffers&) = delete;

  void setResolution(uint32_t resx, uint32_t resy);

  [[nodiscard]] PixelBuffer& getP1() const { return *p1; }
  [[nodiscard]] PixelBuffer& getP2() const { return *p2; }

  [[nodiscard]] PixelBuffer& getOutputBuff() const { return *outputBuff; }
  void setOutputBuff(PixelBuffer& val) { outputBuff = &val; }

  static constexpr size_t maxNumBuffs = 10;
  static constexpr size_t maxBuffInc = maxNumBuffs / 2;
  void setBuffInc(size_t i);
  void rotateBuffers();

private:
  std::vector<std::unique_ptr<PixelBuffer>> buffs{};
  PixelBuffer* p1{};
  PixelBuffer* p2{};
  PixelBuffer* outputBuff{};
  size_t nextBuff = 0;
  size_t buffInc = 1;
  static std::vector<std::unique_ptr<PixelBuffer>> getPixelBuffs(uint32_t resx, uint32_t resy);
};

std::vector<std::unique_ptr<PixelBuffer>> GoomImageBuffers::getPixelBuffs(const uint32_t resx,
                                                                          const uint32_t resy)
{
  std::vector<std::unique_ptr<PixelBuffer>> newBuffs(maxNumBuffs);
  for (auto& b : newBuffs)
  {
    // Allocate one extra row and column to help the filter process handle edges.
    b = std::make_unique<PixelBuffer>(resx + 1, resy + 1);
  }
  return newBuffs;
}

GoomImageBuffers::GoomImageBuffers(const uint32_t resx, const uint32_t resy) noexcept
  : buffs{getPixelBuffs(resx, resy)},
    p1{buffs[0].get()},
    p2{buffs[1].get()},
    nextBuff{maxNumBuffs == 2 ? 0 : 2},
    buffInc{1}
{
}

GoomImageBuffers::~GoomImageBuffers() noexcept = default;

void GoomImageBuffers::setResolution(const uint32_t resx, const uint32_t resy)
{
  buffs = getPixelBuffs(resx, resy);
  p1 = buffs[0].get();
  p2 = buffs[1].get();
  nextBuff = maxNumBuffs == 2 ? 0 : 2;
  buffInc = 1;
}

void GoomImageBuffers::setBuffInc(const size_t i)
{
  if (i < 1 || i > maxBuffInc)
  {
    throw std::logic_error("Cannot set buff inc: i out of range.");
  }
  buffInc = i;
}

void GoomImageBuffers::rotateBuffers()
{
  p1 = p2;
  p2 = buffs[nextBuff].get();
  nextBuff += +buffInc;
  if (nextBuff >= buffs.size())
  {
    nextBuff = 0;
  }
}

struct GoomMessage
{
  std::string message;
  uint32_t numberOfLinesInMessage = 0;
  uint32_t affiche = 0;

  bool operator==(const GoomMessage&) const = default;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(CEREAL_NVP(message), CEREAL_NVP(numberOfLinesInMessage), CEREAL_NVP(affiche));
  };
};

struct GoomVisualFx
{
  GoomVisualFx() noexcept = default;
  explicit GoomVisualFx(Parallel&, const std::shared_ptr<const PluginInfo>&) noexcept;

  std::shared_ptr<ConvolveFx> convolve_fx{};
  std::shared_ptr<ZoomFilterFx> zoomFilter_fx{};
  std::shared_ptr<FlyingStarsFx> star_fx{};
  std::shared_ptr<GoomDotsFx> goomDots_fx{};
  std::shared_ptr<IfsFx> ifs_fx{};
  std::shared_ptr<TentaclesFx> tentacles_fx{};

  std::vector<std::shared_ptr<VisualFx>> list{};

  bool operator==(const GoomVisualFx&) const;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(CEREAL_NVP(zoomFilter_fx), CEREAL_NVP(ifs_fx), CEREAL_NVP(star_fx), CEREAL_NVP(convolve_fx),
       CEREAL_NVP(tentacles_fx), CEREAL_NVP(goomDots_fx), CEREAL_NVP(list));
  };
};

GoomVisualFx::GoomVisualFx(Parallel& p, const std::shared_ptr<const PluginInfo>& goomInfo) noexcept
  : convolve_fx{new ConvolveFx{p, goomInfo}},
    zoomFilter_fx{new ZoomFilterFx{p, goomInfo}},
    star_fx{new FlyingStarsFx{goomInfo}},
    goomDots_fx{new GoomDotsFx{goomInfo}},
    ifs_fx{new IfsFx{goomInfo}},
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

bool GoomVisualFx::operator==(const GoomVisualFx& f) const
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

  static constexpr int maxTitleDisplayTime = 300;
  static constexpr int timeToFadeTitleDisplay = 50;
  int timeOfTitleDisplay = 0;
  std::string title{};

  bool operator==(const GoomData&) const;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(CEREAL_NVP(lockVar), CEREAL_NVP(stopLines), CEREAL_NVP(cyclesSinceLastChange),
       CEREAL_NVP(drawLinesDuration), CEREAL_NVP(lineMode), CEREAL_NVP(switchMult),
       CEREAL_NVP(switchIncr), CEREAL_NVP(stateSelectionBlocker), CEREAL_NVP(previousZoomSpeed),
       CEREAL_NVP(timeOfTitleDisplay), CEREAL_NVP(title), CEREAL_NVP(zoomFilterData));
  };
};

bool GoomData::operator==(const GoomData& d) const
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
  void swap(GoomControl::GoomControlImpl& other) noexcept = delete;

  PixelBuffer& getScreenBuffer() const;
  void setScreenBuffer(PixelBuffer&);
  uint32_t getScreenWidth() const;
  uint32_t getScreenHeight() const;

  void start();
  void finish();

  void update(
      const AudioSamples&, int forceMode, float fps, const char* songTitle, const char* message);

  bool operator==(const GoomControlImpl&) const;

private:
  Parallel parallel;
  std::shared_ptr<WritablePluginInfo> goomInfo{};
  GoomImageBuffers imageBuffers{};
  GoomVisualFx visualFx{};
  GoomStats stats{};
  GoomStates states{};
  GoomEvents goomEvent{};
  uint32_t timeInState = 0;
  uint32_t cycle = 0;
  std::unordered_set<GoomDrawable> curGDrawables{};
  GoomMessage messageData{};
  GoomData goomData{};
  GoomDraw draw{};
  TextDraw text{};

  // Line Fx
  LinesFx gmline1{};
  LinesFx gmline2{};

  bool changeFilterModeEventHappens();
  void setNextFilterMode();
  void changeState();
  void changeFilterMode();
  void changeMilieu();
  void bigNormalUpdate(ZoomFilterData** pzfd);
  void megaLentUpdate(ZoomFilterData** pzfd);

  // baisser regulierement la vitesse
  void regularlyLowerTheSpeed(ZoomFilterData** pzfd);
  void lowerSpeed(ZoomFilterData** pzfd);

  // on verifie qu'il ne se pas un truc interressant avec le son.
  void changeFilterModeIfMusicChanges(int forceMode);

  // Changement d'effet de zoom !
  void changeZoomEffect(ZoomFilterData*, int forceMode);

  void applyIfsIfRequired();
  void applyTentaclesIfRequired();
  void applyStarsIfRequired();

  void displayText(const char* songTitle, const char* message, float fps);

#ifdef SHOW_STATE_TEXT_ON_SCREEN
  void displayStateText();
#endif

  void drawDotsIfRequired();

  void chooseGoomLine(float* param1,
                      float* param2,
                      Pixel* couleur,
                      LinesFx::LineType* mode,
                      float* amplitude,
                      int far);

  // si on est dans un goom : afficher les lignes
  void displayLinesIfInAGoom(const AudioSamples&);
  void displayLines(const AudioSamples&);

  // arret demande
  void stopRequest();
  void stopIfRequested();

  // arret aleatore.. changement de mode de ligne..
  void stopRandomLineChangeMode();

  // Permet de forcer un effet.
  void forceFilterModeIfSet(ZoomFilterData**, int forceMode);
  void forceFilterMode(int forceMode, ZoomFilterData**);

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

  friend class cereal::access;
  template<class Archive>
  void save(Archive&) const;
  template<class Archive>
  void load(Archive&);
};

uint64_t GoomControl::getRandSeed()
{
  return goom::utils::getRandSeed();
}

void GoomControl::setRandSeed(const uint64_t seed)
{
  logDebug("Set goom seed = {}.", seed);
  goom::utils::setRandSeed(seed);
}

GoomControl::GoomControl() noexcept : controller{new GoomControlImpl{}}
{
}

GoomControl::GoomControl(const uint32_t resx, const uint32_t resy) noexcept
  : controller{new GoomControlImpl{resx, resy}}
{
}

GoomControl::~GoomControl() noexcept = default;

bool GoomControl::operator==(const GoomControl& c) const
{
  return controller->operator==(*c.controller);
}

void GoomControl::saveState(std::ostream& f) const
{
  cereal::JSONOutputArchive archive(f);
  archive(*this);
}

void GoomControl::restoreState(std::istream& f)
{
  PixelBuffer& outputBuffer = controller->getScreenBuffer();
  cereal::JSONInputArchive archive(f);
  archive(*this);
  controller->setScreenBuffer(outputBuffer);
}

void GoomControl::setScreenBuffer(PixelBuffer& buffer)
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

void GoomControl::update(const AudioSamples& soundData,
                         const int forceMode,
                         const float fps,
                         const char* songTitle,
                         const char* message)
{
  controller->update(soundData, forceMode, fps, songTitle, message);
}

template<class Archive>
void GoomControl::serialize(Archive& ar)
{
  ar(CEREAL_NVP(controller));
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
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(timeInState), CEREAL_NVP(cycle), CEREAL_NVP(visualFx),
     CEREAL_NVP(curGDrawables), CEREAL_NVP(messageData), CEREAL_NVP(goomData), CEREAL_NVP(gmline1),
     CEREAL_NVP(gmline2));
}

template<class Archive>
void GoomControl::GoomControlImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(timeInState), CEREAL_NVP(cycle), CEREAL_NVP(visualFx),
     CEREAL_NVP(curGDrawables), CEREAL_NVP(messageData), CEREAL_NVP(goomData), CEREAL_NVP(gmline1),
     CEREAL_NVP(gmline2));

  imageBuffers.setResolution(goomInfo->getScreenInfo().width, goomInfo->getScreenInfo().height);
}

bool GoomControl::GoomControlImpl::operator==(const GoomControlImpl& c) const
{
  if (goomInfo == nullptr && c.goomInfo != nullptr)
  {
    return false;
  }
  if (goomInfo != nullptr && c.goomInfo == nullptr)
  {
    return false;
  }

  bool result = ((goomInfo == nullptr && c.goomInfo == nullptr) || (*goomInfo == *c.goomInfo)) &&
                timeInState == c.timeInState && cycle == c.cycle && visualFx == c.visualFx &&
                curGDrawables == c.curGDrawables && messageData == c.messageData &&
                goomData == c.goomData && gmline1 == c.gmline1 && gmline2 == c.gmline2;

  if (!result)
  {
    logDebug("result == {}", result);
    logDebug("goomInfo->getScreenInfo().width = {}", goomInfo->getScreenInfo().width);
    logDebug("c.goomInfo->getScreenInfo().width = {}", c.goomInfo->getScreenInfo().width);
    logDebug("goomInfo->getScreenInfo().height = {}", goomInfo->getScreenInfo().height);
    logDebug("c.goomInfo->getScreenInfo().height = {}", c.goomInfo->getScreenInfo().height);
    logDebug("timeInState = {}, c.timeInState = {}", timeInState, c.timeInState);
    logDebug("cycle = {}, c.cycle = {}", cycle, c.cycle);
    logDebug("visualFx == c.visualFx = {}", visualFx == c.visualFx);
    logDebug("curGDrawables == c.curGDrawables = {}", curGDrawables == c.curGDrawables);
    logDebug("messageData == c.messageData = {}", messageData == c.messageData);
    logDebug("goomData == c.goomData = {}", goomData == c.goomData);
    logDebug("gmline1 == c.gmline1 = {}", gmline1 == c.gmline1);
    logDebug("gmline2 == c.gmline2 = {}", gmline2 == c.gmline2);
  }
  return result;
}

static const Pixel lRed = getRedLineColor();
static const Pixel lGreen = getGreenLineColor();
static const Pixel lBlack = getBlackLineColor();

GoomControl::GoomControlImpl::GoomControlImpl() noexcept : parallel{}
{
}

GoomControl::GoomControlImpl::GoomControlImpl(const uint32_t screenWidth,
                                              const uint32_t screenHeight) noexcept
  : parallel{-1}, // max cores - 1
    goomInfo{new WritablePluginInfo{screenWidth, screenHeight}},
    imageBuffers{screenWidth, screenHeight},
    visualFx{parallel, std::const_pointer_cast<const PluginInfo>(
                           std::dynamic_pointer_cast<PluginInfo>(goomInfo))},
    draw{screenWidth, screenHeight},
    text{screenWidth, screenHeight},
    gmline1{
        std::const_pointer_cast<const PluginInfo>(std::dynamic_pointer_cast<PluginInfo>(goomInfo)),
        LinesFx::LineType::hline,
        static_cast<float>(screenHeight),
        lBlack,
        LinesFx::LineType::circle,
        0.4f * static_cast<float>(screenHeight),
        lGreen},
    gmline2{
        std::const_pointer_cast<const PluginInfo>(std::dynamic_pointer_cast<PluginInfo>(goomInfo)),
        LinesFx::LineType::hline,
        0,
        lBlack,
        LinesFx::LineType::circle,
        0.2f * static_cast<float>(screenHeight),
        lRed}
{
  logDebug("Initialize goom: screenWidth = {}, screenHeight = {}.", screenWidth, screenHeight);
}

GoomControl::GoomControlImpl::~GoomControlImpl() noexcept = default;

PixelBuffer& GoomControl::GoomControlImpl::getScreenBuffer() const
{
  return imageBuffers.getOutputBuff();
}

void GoomControl::GoomControlImpl::setScreenBuffer(PixelBuffer& buffer)
{
  imageBuffers.setOutputBuff(buffer);
}

uint32_t GoomControl::GoomControlImpl::getScreenWidth() const
{
  return goomInfo->getScreenInfo().width;
}

uint32_t GoomControl::GoomControlImpl::getScreenHeight() const
{
  return goomInfo->getScreenInfo().height;
}

inline bool GoomControl::GoomControlImpl::changeFilterModeEventHappens()
{
  // If we're in amulette mode and the state contains tentacles,
  // then get out with a different probability.
  // (Rationale: get tentacles going earlier with another mode.)
  if ((goomData.zoomFilterData.mode == ZoomFilterMode::amuletteMode) &&
      states.getCurrentDrawables().contains(GoomDrawable::tentacles))
  {
    return goomEvent.happens(GoomEvent::changeFilterFromAmuletteMode);
  }

  return goomEvent.happens(GoomEvent::changeFilterMode);
}

void GoomControl::GoomControlImpl::start()
{
  timeInState = 0;
  changeState();
  changeFilterMode();
  goomEvent.setGoomInfo(goomInfo.get());

  curGDrawables = states.getCurrentDrawables();
  setNextFilterMode();
  goomData.zoomFilterData.middleX = getScreenWidth();
  goomData.zoomFilterData.middleY = getScreenHeight();

  stats.reset();
  stats.setStateStartValue(states.getCurrentStateIndex());
  stats.setZoomFilterStartValue(goomData.zoomFilterData.mode);
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

void GoomControl::GoomControlImpl::finish()
{
  for (auto& v : visualFx.list)
  {
    v->log(logStatsValue);
  }

  stats.setStateLastValue(states.getCurrentStateIndex());
  stats.setZoomFilterLastValue(&goomData.zoomFilterData);
  stats.setSeedLastValue(getRandSeed());
  stats.setNumThreadsUsedValue(parallel.getNumThreadsUsed());

  stats.log(logStatsValue);

  for (auto& v : visualFx.list)
  {
    v->finish();
  }
}

void GoomControl::GoomControlImpl::update(const AudioSamples& soundData,
                                          const int forceMode,
                                          const float fps,
                                          const char* songTitle,
                                          const char* message)
{
  stats.updateChange(states.getCurrentStateIndex(), goomData.zoomFilterData.mode);

  timeInState++;

  // elargissement de l'intervalle d'évolution des points
  // ! calcul du deplacement des petits points ...

  logDebug("sound getTimeSinceLastGoom() = {}", goomInfo->getSoundInfo().getTimeSinceLastGoom());

  /* ! etude du signal ... */
  goomInfo->processSoundSample(soundData);

  // applyIfsIfRequired();

  drawDotsIfRequired();

  /* par défaut pas de changement de zoom */
  if (forceMode != 0)
  {
    logDebug("forceMode = {}\n", forceMode);
  }
  goomData.lockVar--;
  if (goomData.lockVar < 0)
  {
    goomData.lockVar = 0;
  }
  stats.lockChange();
  /* note pour ceux qui n'ont pas suivis : le lockVar permet d'empecher un */
  /* changement d'etat du plugin juste apres un autre changement d'etat. oki */
  // -- Note for those who have not followed: the lockVar prevents a change
  // of state of the plugin just after another change of state.

  changeFilterModeIfMusicChanges(forceMode);

  ZoomFilterData* pzfd{};

  bigUpdateIfNotLocked(&pzfd);

  bigBreakIfMusicIsCalm(&pzfd);

  regularlyLowerTheSpeed(&pzfd);

  stopDecrementingAfterAWhile(&pzfd);

  forceFilterModeIfSet(&pzfd, forceMode);

  changeZoomEffect(pzfd, forceMode);

  // Zoom here!
  visualFx.zoomFilter_fx->zoomFilterFastRGB(imageBuffers.getP1(), imageBuffers.getP2(), pzfd,
                                            goomData.switchIncr, goomData.switchMult);

  // drawDotsIfRequired();
  applyIfsIfRequired();
  applyTentaclesIfRequired();
  applyStarsIfRequired();

  /**
#ifdef SHOW_STATE_TEXT_ON_SCREEN
  displayStateText();
#endif
  displayText(songTitle, message, fps);
**/

  // Gestion du Scope - Scope management
  stopIfRequested();
  stopRandomLineChangeMode();
  displayLinesIfInAGoom(soundData);

  // affichage et swappage des buffers...
  visualFx.convolve_fx->convolve(imageBuffers.getP2(), imageBuffers.getOutputBuff());

  imageBuffers.setBuffInc(getRandInRange(1U, GoomImageBuffers::maxBuffInc + 1));
  imageBuffers.rotateBuffers();

#ifdef SHOW_STATE_TEXT_ON_SCREEN
  displayStateText();
#endif
  displayText(songTitle, message, fps);

  cycle++;

  logDebug("About to return.");
}

void GoomControl::GoomControlImpl::chooseGoomLine(float* param1,
                                                  float* param2,
                                                  Pixel* couleur,
                                                  LinesFx::LineType* mode,
                                                  float* amplitude,
                                                  const int far)
{
  *amplitude = 1.0f;
  *mode = goomEvent.getRandomLineTypeEvent();

  switch (*mode)
  {
    case LinesFx::LineType::circle:
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
        *param1 = 0.40f * getScreenHeight();
        *param2 = 0.22f * getScreenHeight();
      }
      else
      {
        *param1 = *param2 = getScreenHeight() * 0.35F;
      }
      break;
    case LinesFx::LineType::hline:
      if (goomEvent.happens(GoomEvent::changeHLineParams) || far)
      {
        *param1 = getScreenHeight() / 7.0F;
        *param2 = 6.0f * getScreenHeight() / 7.0F;
      }
      else
      {
        *param1 = *param2 = getScreenHeight() / 2.0f;
        *amplitude = 2.0f;
      }
      break;
    case LinesFx::LineType::vline:
      if (goomEvent.happens(GoomEvent::changeVLineParams) || far)
      {
        *param1 = getScreenWidth() / 7.0f;
        *param2 = 6.0f * getScreenWidth() / 7.0f;
      }
      else
      {
        *param1 = *param2 = getScreenWidth() / 2.0f;
        *amplitude = 1.5f;
      }
      break;
    default:
      throw std::logic_error("Unknown LineTypes enum.");
  }

  stats.changeLineColor();
  *couleur = gmline1.getRandomLineColor();
}

void GoomControl::GoomControlImpl::changeFilterModeIfMusicChanges(const int forceMode)
{
  logDebug("forceMode = {}", forceMode);
  if (forceMode == -1)
  {
    return;
  }

  logDebug("sound getTimeSinceLastGoom() = {}, goomData.cyclesSinceLastChange = {}",
           goomInfo->getSoundInfo().getTimeSinceLastGoom(), goomData.cyclesSinceLastChange);
  if ((goomInfo->getSoundInfo().getTimeSinceLastGoom() == 0) ||
      (goomData.cyclesSinceLastChange > timeBetweenChange) || (forceMode > 0))
  {
    logDebug("Try to change the filter mode.");
    if (changeFilterModeEventHappens())
    {
      changeFilterMode();
    }
  }
  logDebug("sound getTimeSinceLastGoom() = {}", goomInfo->getSoundInfo().getTimeSinceLastGoom());
}

inline ZoomFilterData::HypercosEffect getHypercosEffect(const bool active)
{
  if (!active)
  {
    return ZoomFilterData::HypercosEffect::none;
  }

  return static_cast<ZoomFilterData::HypercosEffect>(
      getRandInRange(static_cast<uint32_t>(ZoomFilterData::HypercosEffect::none) + 1,
                     static_cast<uint32_t>(ZoomFilterData::HypercosEffect::_size)));
}

void GoomControl::GoomControlImpl::setNextFilterMode()
{
  //  goomData.zoomFilterData.vitesse = 127;
  //  goomData.zoomFilterData.middleX = 16;
  //  goomData.zoomFilterData.middleY = 1;
  goomData.zoomFilterData.reverse = true;
  //  goomData.zoomFilterData.hPlaneEffect = 0;
  //  goomData.zoomFilterData.vPlaneEffect = 0;
  goomData.zoomFilterData.waveEffect = false;
  goomData.zoomFilterData.hypercosEffect = ZoomFilterData::HypercosEffect::none;
  //  goomData.zoomFilterData.noisify = false;
  //  goomData.zoomFilterData.noiseFactor = 1;
  //  goomData.zoomFilterData.blockyWavy = false;
  goomData.zoomFilterData.waveFreqFactor = ZoomFilterData::defaultWaveFreqFactor;
  goomData.zoomFilterData.waveAmplitude = ZoomFilterData::defaultWaveAmplitude;
  goomData.zoomFilterData.waveEffectType = ZoomFilterData::defaultWaveEffectType;
  goomData.zoomFilterData.scrunchAmplitude = ZoomFilterData::defaultScrunchAmplitude;
  goomData.zoomFilterData.speedwayAmplitude = ZoomFilterData::defaultSpeedwayAmplitude;
  goomData.zoomFilterData.amuletteAmplitude = ZoomFilterData::defaultAmuletteAmplitude;
  goomData.zoomFilterData.crystalBallAmplitude = ZoomFilterData::defaultCrystalBallAmplitude;
  goomData.zoomFilterData.hypercosFreq = ZoomFilterData::defaultHypercosFreq;
  goomData.zoomFilterData.hypercosAmplitude = ZoomFilterData::defaultHypercosAmplitude;
  goomData.zoomFilterData.hPlaneEffectAmplitude = ZoomFilterData::defaultHPlaneEffectAmplitude;
  goomData.zoomFilterData.vPlaneEffectAmplitude = ZoomFilterData::defaultVPlaneEffectAmplitude;

  switch (goomEvent.getRandomFilterEvent())
  {
    case GoomFilterEvent::yOnlyMode:
      goomData.zoomFilterData.mode = ZoomFilterMode::yOnlyMode;
      break;
    case GoomFilterEvent::speedwayMode:
      goomData.zoomFilterData.mode = ZoomFilterMode::speedwayMode;
      goomData.zoomFilterData.speedwayAmplitude = getRandInRange(
          ZoomFilterData::minSpeedwayAmplitude, ZoomFilterData::maxSpeedwayAmplitude);
      break;
    case GoomFilterEvent::normalMode:
      goomData.zoomFilterData.mode = ZoomFilterMode::normalMode;
      break;
    case GoomFilterEvent::waveModeWithHyperCosEffect:
      goomData.zoomFilterData.hypercosEffect =
          getHypercosEffect(goomEvent.happens(GoomEvent::hypercosEffectOnWithWaveMode));
      [[fallthrough]];
    case GoomFilterEvent::waveMode:
      goomData.zoomFilterData.mode = ZoomFilterMode::waveMode;
      goomData.zoomFilterData.reverse = false;
      goomData.zoomFilterData.waveEffect = goomEvent.happens(GoomEvent::waveEffectOnWithWaveMode);
      if (goomEvent.happens(GoomEvent::changeVitesseWithWaveMode))
      {
        goomData.zoomFilterData.vitesse = (goomData.zoomFilterData.vitesse + 127) >> 1;
      }
      goomData.zoomFilterData.waveEffectType =
          static_cast<ZoomFilterData::WaveEffect>(getRandInRange(0, 2));
      if (goomEvent.happens(GoomEvent::allowStrangeWaveValues))
      {
        // BUG HERE - wrong ranges - BUT GIVES GOOD AFFECT
        goomData.zoomFilterData.waveAmplitude = getRandInRange(
            ZoomFilterData::minLargeWaveAmplitude, ZoomFilterData::maxLargeWaveAmplitude);
        goomData.zoomFilterData.waveFreqFactor = getRandInRange(
            ZoomFilterData::minWaveSmallFreqFactor, ZoomFilterData::maxWaveSmallFreqFactor);
      }
      else
      {
        goomData.zoomFilterData.waveAmplitude =
            getRandInRange(ZoomFilterData::minWaveAmplitude, ZoomFilterData::maxWaveAmplitude);
        goomData.zoomFilterData.waveFreqFactor =
            getRandInRange(ZoomFilterData::minWaveFreqFactor, ZoomFilterData::maxWaveFreqFactor);
      }
      break;
    case GoomFilterEvent::crystalBallMode:
      goomData.zoomFilterData.mode = ZoomFilterMode::crystalBallMode;
      goomData.zoomFilterData.crystalBallAmplitude = getRandInRange(
          ZoomFilterData::minCrystalBallAmplitude, ZoomFilterData::maxCrystalBallAmplitude);
      break;
    case GoomFilterEvent::crystalBallModeWithEffects:
      goomData.zoomFilterData.mode = ZoomFilterMode::crystalBallMode;
      goomData.zoomFilterData.waveEffect =
          goomEvent.happens(GoomEvent::waveEffectOnWithCrystalBallMode);
      goomData.zoomFilterData.hypercosEffect =
          getHypercosEffect(goomEvent.happens(GoomEvent::hypercosEffectOnWithCrystalBallMode));
      goomData.zoomFilterData.crystalBallAmplitude = getRandInRange(
          ZoomFilterData::minCrystalBallAmplitude, ZoomFilterData::maxCrystalBallAmplitude);
      break;
    case GoomFilterEvent::amuletteMode:
      goomData.zoomFilterData.mode = ZoomFilterMode::amuletteMode;
      goomData.zoomFilterData.amuletteAmplitude = getRandInRange(
          ZoomFilterData::minAmuletteAmplitude, ZoomFilterData::maxAmuletteAmplitude);
      break;
    case GoomFilterEvent::waterMode:
      goomData.zoomFilterData.mode = ZoomFilterMode::waterMode;
      break;
    case GoomFilterEvent::scrunchMode:
      goomData.zoomFilterData.mode = ZoomFilterMode::scrunchMode;
      goomData.zoomFilterData.scrunchAmplitude =
          getRandInRange(ZoomFilterData::minScrunchAmplitude, ZoomFilterData::maxScrunchAmplitude);
      break;
    case GoomFilterEvent::scrunchModeWithEffects:
      goomData.zoomFilterData.mode = ZoomFilterMode::scrunchMode;
      goomData.zoomFilterData.waveEffect = true;
      goomData.zoomFilterData.hypercosEffect = getHypercosEffect(true);
      goomData.zoomFilterData.scrunchAmplitude =
          getRandInRange(ZoomFilterData::minScrunchAmplitude, ZoomFilterData::maxScrunchAmplitude);
      break;
    case GoomFilterEvent::hyperCos1Mode:
      goomData.zoomFilterData.mode = ZoomFilterMode::hyperCos1Mode;
      goomData.zoomFilterData.hypercosEffect =
          getHypercosEffect(goomEvent.happens(GoomEvent::hypercosEffectOnWithHyperCos1Mode));
      break;
    case GoomFilterEvent::hyperCos2Mode:
      goomData.zoomFilterData.mode = ZoomFilterMode::hyperCos2Mode;
      goomData.zoomFilterData.hypercosEffect =
          getHypercosEffect(goomEvent.happens(GoomEvent::hypercosEffectOnWithHyperCos2Mode));
      break;
    default:
      throw std::logic_error("GoomFilterEvent not implemented.");
  }

  if (goomData.zoomFilterData.hypercosEffect != ZoomFilterData::HypercosEffect::none)
  {
    goomData.zoomFilterData.hypercosFreq =
        getRandInRange(ZoomFilterData::minHypercosFreq, ZoomFilterData::maxHypercosFreq);
    goomData.zoomFilterData.hypercosAmplitude =
        getRandInRange(ZoomFilterData::minHypercosAmplitude, ZoomFilterData::maxHypercosAmplitude);
  }

  if (goomData.zoomFilterData.mode == ZoomFilterMode::amuletteMode)
  {
    curGDrawables.erase(GoomDrawable::tentacles);
    stats.tentaclesDisabled();
  }
  else
  {
    curGDrawables = states.getCurrentDrawables();
  }
}

void GoomControl::GoomControlImpl::changeFilterMode()
{
  logDebug("Time to change the filter mode.");

  stats.filterModeChange();

  setNextFilterMode();

  stats.filterModeChange(goomData.zoomFilterData.mode);

  visualFx.ifs_fx->renew();
  stats.ifsRenew();
}

void GoomControl::GoomControlImpl::changeState()
{
  const auto oldGDrawables = states.getCurrentDrawables();

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
    if (!oldGDrawables.contains(GoomDrawable::IFS))
    {
      visualFx.ifs_fx->init();
    }
    else if (goomEvent.happens(GoomEvent::ifsRenew))
    {
      visualFx.ifs_fx->renew();
      stats.ifsRenew();
    }
    visualFx.ifs_fx->updateIncr();
  }

  if (!states.isCurrentlyDrawable(GoomDrawable::scope))
  {
    goomData.stopLines = 0xf000 & 5;
  }
  if (!states.isCurrentlyDrawable(GoomDrawable::farScope))
  {
    goomData.stopLines = 0;
    goomData.lineMode = goomData.drawLinesDuration;
  }

  // Tentacles and amulette don't look so good together.
  if (states.isCurrentlyDrawable(GoomDrawable::tentacles) &&
      (goomData.zoomFilterData.mode == ZoomFilterMode::amuletteMode))
  {
    changeFilterMode();
  }
}

void GoomControl::GoomControlImpl::changeMilieu()
{

  if ((goomData.zoomFilterData.mode == ZoomFilterMode::waterMode) ||
      (goomData.zoomFilterData.mode == ZoomFilterMode::yOnlyMode) ||
      (goomData.zoomFilterData.mode == ZoomFilterMode::amuletteMode))
  {
    goomData.zoomFilterData.middleX = getScreenWidth() / 2;
    goomData.zoomFilterData.middleY = getScreenHeight() / 2;
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
        goomData.zoomFilterData.middleX = getScreenWidth() / 2;
        goomData.zoomFilterData.middleY = getScreenHeight() - 1;
        break;
      case MiddlePointEvents::event2:
        goomData.zoomFilterData.middleX = getScreenWidth() - 1;
        break;
      case MiddlePointEvents::event3:
        goomData.zoomFilterData.middleX = 1;
        break;
      case MiddlePointEvents::event4:
        goomData.zoomFilterData.middleX = getScreenWidth() / 2;
        goomData.zoomFilterData.middleY = getScreenHeight() / 2;
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
      goomData.zoomFilterData.vPlaneEffect = getRandInRange(-2, +3);
      goomData.zoomFilterData.hPlaneEffect = getRandInRange(-2, +3);
      break;
    case PlaneEffectEvents::event2:
      goomData.zoomFilterData.vPlaneEffect = 0;
      goomData.zoomFilterData.hPlaneEffect = getRandInRange(-7, +8);
      break;
    case PlaneEffectEvents::event3:
      goomData.zoomFilterData.vPlaneEffect = getRandInRange(-5, +6);
      goomData.zoomFilterData.hPlaneEffect = -goomData.zoomFilterData.vPlaneEffect;
      break;
    case PlaneEffectEvents::event4:
      goomData.zoomFilterData.hPlaneEffect = static_cast<int>(getRandInRange(5u, 13u));
      goomData.zoomFilterData.vPlaneEffect = -goomData.zoomFilterData.hPlaneEffect;
      break;
    case PlaneEffectEvents::event5:
      goomData.zoomFilterData.vPlaneEffect = static_cast<int>(getRandInRange(5u, 13u));
      goomData.zoomFilterData.hPlaneEffect = -goomData.zoomFilterData.hPlaneEffect;
      break;
    case PlaneEffectEvents::event6:
      goomData.zoomFilterData.hPlaneEffect = 0;
      goomData.zoomFilterData.vPlaneEffect = getRandInRange(-9, +10);
      break;
    case PlaneEffectEvents::event7:
      goomData.zoomFilterData.hPlaneEffect = getRandInRange(-9, +10);
      goomData.zoomFilterData.vPlaneEffect = getRandInRange(-9, +10);
      break;
    case PlaneEffectEvents::event8:
      goomData.zoomFilterData.vPlaneEffect = 0;
      goomData.zoomFilterData.hPlaneEffect = 0;
      break;
    default:
      throw std::logic_error("Unknown MiddlePointEvents enum.");
  }
  goomData.zoomFilterData.hPlaneEffectAmplitude = getRandInRange(
      ZoomFilterData::minHPlaneEffectAmplitude, ZoomFilterData::maxHPlaneEffectAmplitude);
  // I think 'vPlaneEffectAmplitude' has to be the same as 'hPlaneEffectAmplitude' otherwise
  //   buffer breaking effects occur.
  goomData.zoomFilterData.vPlaneEffectAmplitude =
      goomData.zoomFilterData.hPlaneEffectAmplitude + getRandInRange(-0.0009F, 0.0009F);
  //  goomData.zoomFilterData.vPlaneEffectAmplitude = getRandInRange(
  //      ZoomFilterData::minVPlaneEffectAmplitude, ZoomFilterData::maxVPlaneEffectAmplitude);
}

void GoomControl::GoomControlImpl::bigNormalUpdate(ZoomFilterData** pzfd)
{
  if (goomData.stateSelectionBlocker)
  {
    goomData.stateSelectionBlocker--;
  }
  else if (goomEvent.happens(GoomEvent::changeState))
  {
    goomData.stateSelectionBlocker = 3;
    changeState();
  }

  goomData.lockVar = 50;
  stats.lockChange();
  const int32_t newvit =
      stopSpeed + 1 -
      static_cast<int32_t>(3.5F * std::log10(goomInfo->getSoundInfo().getSpeed() * 60 + 1));
  // retablir le zoom avant..
  if ((goomData.zoomFilterData.reverse) && (!(cycle % 13)) &&
      goomEvent.happens(GoomEvent::filterReverseOffAndStopSpeed))
  {
    goomData.zoomFilterData.reverse = false;
    goomData.zoomFilterData.vitesse = stopSpeed - 2;
    goomData.lockVar = 75;
    stats.lockChange();
  }
  if (goomEvent.happens(GoomEvent::filterReverseOn))
  {
    goomData.zoomFilterData.reverse = true;
    goomData.lockVar = 100;
    stats.lockChange();
  }

  if (goomEvent.happens(GoomEvent::filterVitesseStopSpeedMinus1))
  {
    goomData.zoomFilterData.vitesse = stopSpeed - 1;
  }
  if (goomEvent.happens(GoomEvent::filterVitesseStopSpeedPlus1))
  {
    goomData.zoomFilterData.vitesse = stopSpeed + 1;
  }

  changeMilieu();

  if (goomEvent.happens(GoomEvent::turnOffNoise))
  {
    goomData.zoomFilterData.noisify = false;
  }
  else
  {
    goomData.zoomFilterData.noisify = true;
    goomData.zoomFilterData.noiseFactor = 1;
    goomData.lockVar *= 2;
    stats.lockChange();
    stats.doNoise();
  }

  if (!goomEvent.happens(GoomEvent::changeBlockyWavyToOn))
  {
    goomData.zoomFilterData.blockyWavy = false;
  }
  else
  {
    stats.doBlockyWavy();
    goomData.zoomFilterData.blockyWavy = true;
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

  if (goomData.zoomFilterData.mode == ZoomFilterMode::amuletteMode)
  {
    goomData.zoomFilterData.vPlaneEffect = 0;
    goomData.zoomFilterData.hPlaneEffect = 0;
    //    goomData.zoomFilterData.noisify = false;
  }

  if ((goomData.zoomFilterData.middleX == 1) ||
      (goomData.zoomFilterData.middleX == getScreenWidth() - 1))
  {
    goomData.zoomFilterData.vPlaneEffect = 0;
    if (goomEvent.happens(GoomEvent::filterZeroHPlaneEffect))
    {
      goomData.zoomFilterData.hPlaneEffect = 0;
    }
  }

  logDebug("newvit = {}, goomData.zoomFilterData.vitesse = {}", newvit,
           goomData.zoomFilterData.vitesse);
  if (newvit < goomData.zoomFilterData.vitesse)
  {
    // on accelere
    logDebug("newvit = {} < {} = goomData.zoomFilterData.vitesse", newvit,
             goomData.zoomFilterData.vitesse);
    *pzfd = &goomData.zoomFilterData;
    if (((newvit < (stopSpeed - 7)) && (goomData.zoomFilterData.vitesse < stopSpeed - 6) &&
         (cycle % 3 == 0)) ||
        goomEvent.happens(GoomEvent::filterChangeVitesseAndToggleReverse))
    {
      goomData.zoomFilterData.vitesse =
          stopSpeed - static_cast<int32_t>(getNRand(2)) + static_cast<int32_t>(getNRand(2));
      goomData.zoomFilterData.reverse = !goomData.zoomFilterData.reverse;
    }
    else
    {
      goomData.zoomFilterData.vitesse = (newvit + goomData.zoomFilterData.vitesse * 7) / 8;
    }
    goomData.lockVar += 50;
    stats.lockChange();
  }

  if (goomData.lockVar > 150)
  {
    goomData.switchIncr = GoomData::switchIncrAmount;
    goomData.switchMult = 1.0F;
  }
}

void GoomControl::GoomControlImpl::megaLentUpdate(ZoomFilterData** pzfd)
{
  logDebug("mega lent change");
  *pzfd = &goomData.zoomFilterData;
  goomData.zoomFilterData.vitesse = stopSpeed - 1;
  goomData.lockVar += 50;
  stats.lockChange();
  goomData.switchIncr = GoomData::switchIncrAmount;
  goomData.switchMult = 1.0f;
}

void GoomControl::GoomControlImpl::bigUpdate(ZoomFilterData** pzfd)
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
void GoomControl::GoomControlImpl::changeZoomEffect(ZoomFilterData* pzfd, const int forceMode)
{
  if (!goomEvent.happens(GoomEvent::changeBlockyWavyToOn))
  {
    goomData.zoomFilterData.blockyWavy = false;
  }
  else
  {
    goomData.zoomFilterData.blockyWavy = true;
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

    goomData.cyclesSinceLastChange = 0;
    goomData.switchIncr = GoomData::switchIncrAmount;

    int diff = goomData.zoomFilterData.vitesse - goomData.previousZoomSpeed;
    if (diff < 0)
    {
      diff = -diff;
    }

    if (diff > 2)
    {
      goomData.switchIncr *= (diff + 2) / 2;
    }
    goomData.previousZoomSpeed = goomData.zoomFilterData.vitesse;
    goomData.switchMult = 1.0f;

    if (((goomInfo->getSoundInfo().getTimeSinceLastGoom() == 0) &&
         (goomInfo->getSoundInfo().getTotalGoom() < 2)) ||
        (forceMode > 0))
    {
      goomData.switchIncr = 0;
      goomData.switchMult = GoomData::switchMultAmount;

      visualFx.ifs_fx->renew();
      stats.ifsRenew();
    }
  }
  else
  {
    logDebug("pzfd = nullptr");
    logDebug("goomData.cyclesSinceLastChange = {}", goomData.cyclesSinceLastChange);
    if (goomData.cyclesSinceLastChange > timeBetweenChange)
    {
      logDebug("goomData.cyclesSinceLastChange = {} > {} = timeBetweenChange",
               goomData.cyclesSinceLastChange, timeBetweenChange);
      pzfd = &goomData.zoomFilterData;
      goomData.cyclesSinceLastChange = 0;
      visualFx.ifs_fx->renew();
      stats.ifsRenew();
    }
    else
    {
      goomData.cyclesSinceLastChange++;
    }
  }

  if (pzfd)
  {
    logDebug("pzfd->mode = {}", pzfd->mode);
  }
}

void GoomControl::GoomControlImpl::applyTentaclesIfRequired()
{
  if (!curGDrawables.contains(GoomDrawable::tentacles))
  {
    visualFx.tentacles_fx->applyNoDraw();
    return;
  }

  logDebug("curGDrawables tentacles is set.");
  stats.doTentacles();
  visualFx.tentacles_fx->setBuffSettings(states.getCurrentBuffSettings(GoomDrawable::tentacles));
  visualFx.tentacles_fx->apply(imageBuffers.getP1(), imageBuffers.getP2());
}

void GoomControl::GoomControlImpl::applyStarsIfRequired()
{
  if (!curGDrawables.contains(GoomDrawable::stars))
  {
    return;
  }

  logDebug("curGDrawables stars is set.");
  stats.doStars();
  visualFx.star_fx->setBuffSettings(states.getCurrentBuffSettings(GoomDrawable::stars));
  //  visualFx.star_fx->apply(imageBuffers.getP2(), imageBuffers.getP1());
  visualFx.star_fx->apply(imageBuffers.getP1(), imageBuffers.getP2());
}

#ifdef SHOW_STATE_TEXT_ON_SCREEN

void GoomControl::GoomControlImpl::displayStateText()
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

void GoomControl::GoomControlImpl::displayText(const char* songTitle,
                                               const char* message,
                                               const float fps)
{
  updateMessage(message);

  if (fps > 0)
  {
    const std::string text = std20::format("{.0f} fps", fps);
    draw.text(imageBuffers.getP1(), 10, 24, text, 1, false);
  }

  if (songTitle)
  {
    stats.setSongTitle(songTitle);
    goomData.title = songTitle;
    goomData.timeOfTitleDisplay = GoomData::maxTitleDisplayTime;
  }

  if (goomData.timeOfTitleDisplay)
  {
    const auto xPos = static_cast<int>(getScreenWidth() / 2);
    const auto yPos = static_cast<int>(getScreenHeight() / 2 + 7);
    const auto spacing =
        static_cast<float>(GoomData::maxTitleDisplayTime - goomData.timeOfTitleDisplay - 10) / 10;
    constexpr bool center = true;

    draw.text(imageBuffers.getOutputBuff(), xPos, yPos, goomData.title, spacing, center);

    goomData.timeOfTitleDisplay--;

    if (goomData.timeOfTitleDisplay < GoomData::timeToFadeTitleDisplay)
    {
      draw.text(imageBuffers.getP1(), xPos, yPos, goomData.title, spacing, center);
    }
  }
}

/*
 * Met a jour l'affichage du message defilant
 */
void GoomControl::GoomControlImpl::updateMessage(const char* message)
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

      draw.text(imageBuffers.getOutputBuff(), 50, static_cast<int>(ypos), msgLines[i], 1, false);
    }
    messageData.affiche--;
  }
}

void GoomControl::GoomControlImpl::stopRequest()
{
  logDebug("goomData.stopLines = {},"
           " curGDrawables.contains(GoomDrawable::scope) = {}",
           goomData.stopLines, curGDrawables.contains(GoomDrawable::scope));

  float param1 = 0;
  float param2 = 0;
  float amplitude = 0;
  Pixel couleur{};
  LinesFx::LineType mode;
  chooseGoomLine(&param1, &param2, &couleur, &mode, &amplitude, 1);
  couleur = getBlackLineColor();

  gmline1.switchLines(mode, param1, amplitude, couleur);
  gmline2.switchLines(mode, param2, amplitude, couleur);
  stats.switchLines();
  goomData.stopLines &= 0x0fff;
}

/* arret aleatore.. changement de mode de ligne..
  */
void GoomControl::GoomControlImpl::stopRandomLineChangeMode()
{
  if (goomData.lineMode != goomData.drawLinesDuration)
  {
    goomData.lineMode--;
    if (goomData.lineMode == -1)
    {
      goomData.lineMode = 0;
    }
  }
  else if ((cycle % 80 == 0) && goomEvent.happens(GoomEvent::reduceLineMode) && goomData.lineMode)
  {
    goomData.lineMode--;
  }

  if ((cycle % 120 == 0) && goomEvent.happens(GoomEvent::updateLineMode) &&
      curGDrawables.contains(GoomDrawable::scope))
  {
    if (goomData.lineMode == 0)
    {
      goomData.lineMode = goomData.drawLinesDuration;
    }
    else if (goomData.lineMode == goomData.drawLinesDuration)
    {
      goomData.lineMode--;

      float param1 = 0;
      float param2 = 0;
      float amplitude = 0;
      Pixel couleur1{};
      LinesFx::LineType mode;
      chooseGoomLine(&param1, &param2, &couleur1, &mode, &amplitude, goomData.stopLines);

      Pixel couleur2 = gmline2.getRandomLineColor();
      if (goomData.stopLines)
      {
        goomData.stopLines--;
        if (goomEvent.happens(GoomEvent::changeLineToBlack))
        {
          couleur2 = couleur1 = getBlackLineColor();
        }
      }

      logDebug("goomData.lineMode = {} == {} = goomData.drawLinesDuration", goomData.lineMode,
               goomData.drawLinesDuration);
      gmline1.switchLines(mode, param1, amplitude, couleur1);
      gmline2.switchLines(mode, param2, amplitude, couleur2);
      stats.switchLines();
    }
  }
}

void GoomControl::GoomControlImpl::displayLines(const AudioSamples& soundData)
{
  if (!curGDrawables.contains(GoomDrawable::lines))
  {
    return;
  }

  logDebug("curGDrawables lines is set.");

  stats.doLines();

  gmline2.setPower(gmline1.getPower());

  gmline1.drawLines(soundData.getSample(0), imageBuffers.getP1(), imageBuffers.getP2());
  gmline2.drawLines(soundData.getSample(1), imageBuffers.getP1(), imageBuffers.getP2());

  if (((cycle % 121) == 9) && goomEvent.happens(GoomEvent::changeGoomLine) &&
      ((goomData.lineMode == 0) || (goomData.lineMode == goomData.drawLinesDuration)))
  {
    logDebug("cycle % 121 etc.: goomInfo->cycle = {}, rand1_3 = ?", cycle);
    float param1 = 0;
    float param2 = 0;
    float amplitude = 0;
    Pixel couleur1{};
    LinesFx::LineType mode;
    chooseGoomLine(&param1, &param2, &couleur1, &mode, &amplitude, goomData.stopLines);

    Pixel couleur2 = gmline2.getRandomLineColor();
    if (goomData.stopLines)
    {
      goomData.stopLines--;
      if (goomEvent.happens(GoomEvent::changeLineToBlack))
      {
        couleur2 = couleur1 = getBlackLineColor();
      }
    }
    gmline1.switchLines(mode, param1, amplitude, couleur1);
    gmline2.switchLines(mode, param2, amplitude, couleur2);
  }
}

void GoomControl::GoomControlImpl::bigBreakIfMusicIsCalm(ZoomFilterData** pzfd)
{
  logDebug("sound getSpeed() = {:.2}, goomData.zoomFilterData.vitesse = {}, "
           "cycle = {}",
           goomInfo->getSoundInfo().getSpeed(), goomData.zoomFilterData.vitesse, cycle);
  if ((goomInfo->getSoundInfo().getSpeed() < 0.01f) &&
      (goomData.zoomFilterData.vitesse < (stopSpeed - 4)) && (cycle % 16 == 0))
  {
    logDebug("sound getSpeed() = {:.2}", goomInfo->getSoundInfo().getSpeed());
    bigBreak(pzfd);
  }
}

void GoomControl::GoomControlImpl::bigBreak(ZoomFilterData** pzfd)
{
  *pzfd = &goomData.zoomFilterData;
  goomData.zoomFilterData.vitesse += 3;
}

void GoomControl::GoomControlImpl::forceFilterMode(const int forceMode, ZoomFilterData** pzfd)
{
  *pzfd = &goomData.zoomFilterData;
  (*pzfd)->mode = static_cast<ZoomFilterMode>(forceMode - 1);
}

void GoomControl::GoomControlImpl::lowerSpeed(ZoomFilterData** pzfd)
{
  *pzfd = &goomData.zoomFilterData;
  goomData.zoomFilterData.vitesse++;
}

void GoomControl::GoomControlImpl::stopDecrementing(ZoomFilterData** pzfd)
{
  *pzfd = &goomData.zoomFilterData;
}

void GoomControl::GoomControlImpl::bigUpdateIfNotLocked(ZoomFilterData** pzfd)
{
  logDebug("goomData.lockVar = {}", goomData.lockVar);
  if (goomData.lockVar == 0)
  {
    logDebug("goomData.lockVar = 0");
    bigUpdate(pzfd);
  }
  logDebug("sound getTimeSinceLastGoom() = {}", goomInfo->getSoundInfo().getTimeSinceLastGoom());
}

void GoomControl::GoomControlImpl::forceFilterModeIfSet(ZoomFilterData** pzfd, const int forceMode)
{
  constexpr auto numFilterFx = static_cast<size_t>(ZoomFilterMode::_size);

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

void GoomControl::GoomControlImpl::stopIfRequested()
{
  logDebug("goomData.stopLines = {}, curGState->scope = {}", goomData.stopLines,
           states.isCurrentlyDrawable(GoomDrawable::scope));
  if ((goomData.stopLines & 0xf000) || !states.isCurrentlyDrawable(GoomDrawable::scope))
  {
    stopRequest();
  }
}

void GoomControl::GoomControlImpl::displayLinesIfInAGoom(const AudioSamples& soundData)
{
  logDebug("goomData.lineMode = {} != 0 || sound getTimeSinceLastGoom() = {}", goomData.lineMode,
           goomInfo->getSoundInfo().getTimeSinceLastGoom());
  if ((goomData.lineMode != 0) || (goomInfo->getSoundInfo().getTimeSinceLastGoom() < 5))
  {
    logDebug("goomData.lineMode = {} != 0 || sound getTimeSinceLastGoom() = {} < 5",
             goomData.lineMode, goomInfo->getSoundInfo().getTimeSinceLastGoom());

    displayLines(soundData);
  }
}

void GoomControl::GoomControlImpl::applyIfsIfRequired()
{
  if (!curGDrawables.contains(GoomDrawable::IFS))
  {
    visualFx.ifs_fx->applyNoDraw();
    return;
  }

  logDebug("curGDrawables IFS is set");
  stats.doIFS();
  visualFx.ifs_fx->setBuffSettings(states.getCurrentBuffSettings(GoomDrawable::IFS));
  visualFx.ifs_fx->apply(imageBuffers.getP1(), imageBuffers.getP2());
}

void GoomControl::GoomControlImpl::regularlyLowerTheSpeed(ZoomFilterData** pzfd)
{
  logDebug("goomData.zoomFilterData.vitesse = {}, cycle = {}", goomData.zoomFilterData.vitesse,
           cycle);
  if ((cycle % 73 == 0) && (goomData.zoomFilterData.vitesse < (stopSpeed - 5)))
  {
    logDebug("cycle % 73 = 0 && dgoomData.zoomFilterData.vitesse = {} < {} - 5, ", cycle,
             goomData.zoomFilterData.vitesse, stopSpeed);
    lowerSpeed(pzfd);
  }
}

void GoomControl::GoomControlImpl::stopDecrementingAfterAWhile(ZoomFilterData** pzfd)
{
  logDebug("cycle = {}, goomData.zoomFilterData.pertedec = {}", cycle,
           goomData.zoomFilterData.pertedec);
  if ((cycle % 101 == 0) && (ZoomFilterData::pertedec == 7))
  {
    logDebug("cycle % 101 = 0 && goomData.zoomFilterData.pertedec = 7, ", cycle,
             goomData.zoomFilterData.vitesse);
    stopDecrementing(pzfd);
  }
}

void GoomControl::GoomControlImpl::drawDotsIfRequired()
{
  if (!curGDrawables.contains(GoomDrawable::dots))
  {
    return;
  }

  logDebug("goomInfo->curGDrawables points is set.");
  stats.doDots();
  visualFx.goomDots_fx->apply(imageBuffers.getP1(), imageBuffers.getP2());
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

inline LinesFx::LineType GoomEvents::getRandomLineTypeEvent() const
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

FXBuffSettings GoomStates::getCurrentBuffSettings(const GoomDrawable theFx) const
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
    const GoomStates::WeightedStatesArray& theStates)
{
  std::vector<std::pair<uint16_t, size_t>> weightedVals(theStates.size());
  for (size_t i = 0; i < theStates.size(); i++)
  {
    weightedVals[i] = std::make_pair(i, theStates[i].weight);
  }
  return weightedVals;
}

} // namespace goom
