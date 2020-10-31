#include "tentacles_fx.h"

#include "colorutils.h"
#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
#include "goomutils/mathutils.h"
//#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "tentacle_driver.h"
#include "tentacles.h"
#include "v3d.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace goom
{

using namespace goom::utils;

inline bool startPrettyMoveEvent()
{
  return probabilityOfMInN(1, 400);
}

inline bool changeRotationEvent()
{
  return probabilityOfMInN(1, 100);
}

inline bool turnRotationOnEvent()
{
  return probabilityOfMInN(1, 2);
}

inline bool changeDominantColorMapEvent()
{
  return probabilityOfMInN(1, 50);
}

// IMPORTANT. Very delicate here - 1 in 30 seems just right
inline bool changeDominantColorEvent()
{
  return probabilityOfMInN(1, 30);
}

class TentacleStats
{
public:
  TentacleStats() noexcept = default;

  void reset();
  void log(const StatsLogValueFunc) const;
  void changeDominantColorMap();
  void changeDominantColor();
  void updateWithDraw();
  void updateWithNoDraw();
  void updateWithPrettyMoveNoDraw();
  void updateWithPrettyMoveDraw();
  void lowToMediumAcceleration();
  void highAcceleration();
  void cycleReset();
  void prettyMoveHappens();
  void changePrettyLerpMixLower();
  void changePrettyLerpMixHigher();
  void setNumTentacleDrivers(const std::vector<std::unique_ptr<TentacleDriver>>&);
  void changeTentacleDriver(const uint32_t driverIndex);

  void setLastNumTentacles(const size_t val);
  void setLastUpdatingWithDraw(const bool val);
  void setLastCycle(const float val);
  void setLastCycleInc(const float val);
  void setLastLig(const float val);
  void setLastLigs(const float val);
  void setLastDistt(const float val);
  void setLastDistt2(const float val);
  void setLastDistt2Offset(const float val);
  void setLastRot(const float val);
  void setLastRotAtStartOfPrettyMove(const float val);
  void setLastDoRotation(const bool val);
  void setLastIsPrettyMoveHappening(const bool val);
  void setLastPrettyMoveHappeningTimer(const int32_t val);
  void setLastPrettyMoveCheckStopMark(const int32_t val);
  void setLastPrePrettyMoveLock(const int32_t val);
  void setLastDistt2OffsetPreStep(const float val);
  void setLastPrettyMoveReadyToStart(const bool val);
  void setLastPostPrettyMoveLock(const int32_t val);
  void setLastPrettyLerpMixValue(const float val);

private:
  uint32_t numDominantColorMapChanges = 0;
  uint32_t numDominantColorChanges = 0;
  uint32_t numUpdatesWithDraw = 0;
  uint32_t numUpdatesWithNoDraw = 0;
  uint32_t numUpdatesWithPrettyMoveNoDraw = 0;
  uint32_t numUpdatesWithPrettyMoveDraw = 0;
  uint32_t numLowToMediumAcceleration = 0;
  uint32_t numHighAcceleration = 0;
  uint32_t numCycleResets = 0;
  uint32_t numPrettyMoveHappens = 0;
  uint32_t numTentacleDrivers = 0;
  std::vector<uint32_t> numDriverTentacles{};
  std::vector<uint32_t> numDriverChanges{};

  size_t lastNumTentacles = 0;
  bool lastUpdatingWithDraw = false;
  float lastCycle = 0;
  float lastCycleInc = 0;
  float lastLig = 0;
  float lastLigs = 0;
  float lastDistt = 0;
  float lastDistt2 = 0;
  float lastDistt2Offset = 0;
  float lastRot = 0;
  float lastRotAtStartOfPrettyMove = 0;
  bool lastDoRotation = false;
  bool lastIsPrettyMoveHappening = false;
  int32_t lastPrettyMoveHappeningTimer = 0;
  int32_t lastPrettyMoveCheckStopMark = 0;
  int32_t lastPrePrettyMoveLock = 0;
  float lastDistt2OffsetPreStep = 0;
  bool lastPrettyMoveReadyToStart = false;
  int32_t lastPostPrettyMoveLock = 0;
  float lastPrettyLerpMixValue = 0;
};

void TentacleStats::log(const StatsLogValueFunc logVal) const
{
  const constexpr char* module = "Tentacles";

  logVal(module, "lastNumTentacles", lastNumTentacles);
  logVal(module, "lastUpdatingWithDraw", lastUpdatingWithDraw);
  logVal(module, "lastCycle", lastCycle);
  logVal(module, "lastCycleInc", lastCycleInc);
  logVal(module, "lastLig", lastLig);
  logVal(module, "lastLigs", lastLigs);
  logVal(module, "lastDistt", lastDistt);
  logVal(module, "lastDistt2", lastDistt2);
  logVal(module, "lastDistt2Offset", lastDistt2Offset);
  logVal(module, "lastRot", lastRot);
  logVal(module, "lastRotAtStartOfPrettyMove", lastRotAtStartOfPrettyMove);
  logVal(module, "lastDoRotation", lastDoRotation);
  logVal(module, "lastIsPrettyMoveHappening", lastIsPrettyMoveHappening);
  logVal(module, "lastPrettyMoveHappeningTimer", lastPrettyMoveHappeningTimer);
  logVal(module, "lastPrettyMoveCheckStopMark", lastPrettyMoveCheckStopMark);
  logVal(module, "lastPrePrettyMoveLock", lastPrePrettyMoveLock);
  logVal(module, "lastDistt2OffsetPreStep", lastDistt2OffsetPreStep);
  logVal(module, "lastPrettyMoveReadyToStart", lastPrettyMoveReadyToStart);
  logVal(module, "lastPostPrettyMoveLock", lastPostPrettyMoveLock);
  logVal(module, "lastPrettyLerpMixValue", lastPrettyLerpMixValue);

  logVal(module, "numDominantColorMapChanges", numDominantColorMapChanges);
  logVal(module, "numDominantColorChanges", numDominantColorChanges);
  logVal(module, "numUpdatesWithDraw", numUpdatesWithDraw);
  logVal(module, "numUpdatesWithNoDraw", numUpdatesWithNoDraw);
  logVal(module, "numUpdatesWithPrettyMoveNoDraw", numUpdatesWithPrettyMoveNoDraw);
  logVal(module, "numUpdatesWithPrettyMoveDraw", numUpdatesWithPrettyMoveDraw);
  logVal(module, "numLowToMediumAcceleration", numLowToMediumAcceleration);
  logVal(module, "numHighAcceleration", numHighAcceleration);
  logVal(module, "numCycleResets", numCycleResets);
  logVal(module, "numPrettyMoveHappens", numPrettyMoveHappens);
  logVal(module, "numTentacleDrivers", numTentacleDrivers);
  // TODO Make this a string util function
  std::string numTentaclesStr = "";
  std::string numDriverChangesStr = "";
  for (size_t i = 0; i < numDriverTentacles.size(); i++)
  {
    if (i)
    {
      numTentaclesStr += ", ";
      numDriverChangesStr += ", ";
    }
    numTentaclesStr += std::to_string(numDriverTentacles[i]);
    numDriverChangesStr += std::to_string(numDriverChanges[i]);
  }
  logVal(module, "numDriverTentacles", numTentaclesStr);
  logVal(module, "numDriverChangesStr", numDriverChangesStr);
}

void TentacleStats::reset()
{
  numDominantColorMapChanges = 0;
  numDominantColorChanges = 0;
  numUpdatesWithDraw = 0;
  numUpdatesWithNoDraw = 0;
  numUpdatesWithPrettyMoveNoDraw = 0;
  numUpdatesWithPrettyMoveDraw = 0;
  numLowToMediumAcceleration = 0;
  numHighAcceleration = 0;
  numCycleResets = 0;
  numPrettyMoveHappens = 0;
  std::fill(numDriverChanges.begin(), numDriverChanges.end(), 0);
}

inline void TentacleStats::changeDominantColorMap()
{
  numDominantColorMapChanges++;
}

inline void TentacleStats::changeDominantColor()
{
  numDominantColorChanges++;
}

inline void TentacleStats::updateWithDraw()
{
  numUpdatesWithDraw++;
}

inline void TentacleStats::updateWithNoDraw()
{
  numUpdatesWithNoDraw++;
}

inline void TentacleStats::updateWithPrettyMoveNoDraw()
{
  numUpdatesWithPrettyMoveNoDraw++;
}

inline void TentacleStats::updateWithPrettyMoveDraw()
{
  numUpdatesWithPrettyMoveDraw++;
}

inline void TentacleStats::lowToMediumAcceleration()
{
  numLowToMediumAcceleration++;
}

inline void TentacleStats::highAcceleration()
{
  numHighAcceleration++;
}

inline void TentacleStats::cycleReset()
{
  numCycleResets++;
}

inline void TentacleStats::prettyMoveHappens()
{
  numPrettyMoveHappens++;
}

inline void TentacleStats::setLastNumTentacles(const size_t val)
{
  lastNumTentacles = val;
}

inline void TentacleStats::setLastUpdatingWithDraw(const bool val)
{
  lastUpdatingWithDraw = val;
}

inline void TentacleStats::setLastCycle(const float val)
{
  lastCycle = val;
}

inline void TentacleStats::setLastCycleInc(const float val)
{
  lastCycleInc = val;
}

inline void TentacleStats::setLastLig(const float val)
{
  lastLig = val;
}

inline void TentacleStats::setLastLigs(const float val)
{
  lastLigs = val;
}

inline void TentacleStats::setLastDistt(const float val)
{
  lastDistt = val;
}

inline void TentacleStats::setLastDistt2(const float val)
{
  lastDistt2 = val;
}

inline void TentacleStats::setLastDistt2Offset(const float val)
{
  lastDistt2Offset = val;
}

inline void TentacleStats::setLastRot(const float val)
{
  lastRot = val;
}

inline void TentacleStats::setLastRotAtStartOfPrettyMove(const float val)
{
  lastRotAtStartOfPrettyMove = val;
}

inline void TentacleStats::setLastDoRotation(const bool val)
{
  lastDoRotation = val;
}

inline void TentacleStats::setLastIsPrettyMoveHappening(const bool val)
{
  lastIsPrettyMoveHappening = val;
}

inline void TentacleStats::setLastPrettyMoveHappeningTimer(const int32_t val)
{
  lastPrettyMoveHappeningTimer = val;
}

inline void TentacleStats::setLastPrettyMoveCheckStopMark(const int32_t val)
{
  lastPrettyMoveCheckStopMark = val;
}

inline void TentacleStats::setLastPrePrettyMoveLock(const int32_t val)
{
  lastPrePrettyMoveLock = val;
}

inline void TentacleStats::setLastDistt2OffsetPreStep(const float val)
{
  lastDistt2OffsetPreStep = val;
}

inline void TentacleStats::setLastPrettyMoveReadyToStart(const bool val)
{
  lastPrettyMoveReadyToStart = val;
}

inline void TentacleStats::setLastPostPrettyMoveLock(const int32_t val)
{
  lastPostPrettyMoveLock = val;
}

inline void TentacleStats::setLastPrettyLerpMixValue(const float val)
{
  lastPrettyLerpMixValue = val;
}

void TentacleStats::setNumTentacleDrivers(
    const std::vector<std::unique_ptr<TentacleDriver>>& drivers)
{
  numTentacleDrivers = drivers.size();
  numDriverTentacles.resize(numTentacleDrivers);
  numDriverChanges.resize(numTentacleDrivers);
  for (size_t i = 0; i < numTentacleDrivers; i++)
  {
    numDriverTentacles[i] = drivers[i]->getNumTentacles();
    numDriverChanges[i] = 0;
  }
}

void TentacleStats::changeTentacleDriver(const uint32_t driverIndex)
{
  numDriverChanges.at(driverIndex)++;
}

class TentaclesFx::TentaclesImpl
{
public:
  explicit TentaclesImpl(const PluginInfo*);
  ~TentaclesImpl() noexcept = default;
  TentaclesImpl(const TentaclesImpl&) = delete;
  TentaclesImpl& operator=(const TentaclesImpl&) = delete;

  void setBuffSettings(const FXBuffSettings&);

  void update(Pixel* prevBuff, Pixel* currentBuff);
  void updateWithNoDraw();

  void logStats(const StatsLogValueFunc logVal);

private:
  const PluginInfo* const goomInfo;
  WeightedColorMaps colorMaps;
  const ColorMap* dominantColorMap;
  Pixel dominantColor;
  void changeDominantColor();

  bool updatingWithDraw = false;
  float cycle = 0;
  static constexpr float cycleIncMin = 0.01;
  static constexpr float cycleIncMax = 0.05;
  float cycleInc = cycleIncMin;
  float lig = 1.15;
  float ligs = 0.1;
  float distt = 10;
  static constexpr double disttMin = 106.0;
  static constexpr double disttMax = 286.0;
  float distt2 = 0;
  static constexpr float distt2Min = 8.0;
  static constexpr float distt2Max = 1000;
  float distt2Offset = 0;
  float rot; // entre 0 et m_two_pi
  float rotAtStartOfPrettyMove = 0;
  bool doRotation = false;
  static float getStableRotationOffset(const float cycleVal);
  bool isPrettyMoveHappening = false;
  int32_t prettyMoveHappeningTimer = 0;
  int32_t prettyMoveCheckStopMark = 0;
  static constexpr uint32_t prettyMoveHappeningMin = 100;
  static constexpr uint32_t prettyMoveHappeningMax = 200;
  int32_t prePrettyMoveLock = 0;
  float distt2OffsetPreStep = 0;
  bool prettyMoveReadyToStart = false;
  static constexpr int32_t minPrePrettyMoveLock = 200;
  static constexpr int32_t maxPrePrettyMoveLock = 500;
  int32_t postPrettyMoveLock = 0;
  float prettyMoveLerpMix = 1.0 / 16.0; // original goom value
  void isPrettyMoveHappeningUpdate(const float acceleration);
  void prettyMovePreStart();
  void prettyMoveStart(const float acceleration, const int32_t timerVal = -1);
  void prettyMoveFinish();
  void prettyMove(const float acceleration);
  void prettyMoveWithNoDraw();
  std::tuple<Pixel, Pixel> getModColors();

  size_t countSinceHighAccelLastMarked = 0;
  size_t countSinceColorChangeLastMarked = 0;
  void incCounters();

  static constexpr size_t numDrivers = 4;
  std::vector<std::unique_ptr<TentacleDriver>> drivers;
  TentacleDriver* currentDriver = nullptr;
  TentacleDriver* getNextDriver() const;
  const Weights<size_t> driverWeights;

  void init();
  mutable TentacleStats stats;
};

// TODO PASS GoomINFO to wrapper
TentaclesFx::TentaclesFx(const PluginInfo* info) : fxImpl{new TentaclesImpl{info}}
{
}

void TentaclesFx::setBuffSettings(const FXBuffSettings& settings)
{
  fxImpl->setBuffSettings(settings);
}

void TentaclesFx::start()
{
}

void TentaclesFx::finish()
{
}

void TentaclesFx::log(const StatsLogValueFunc& logVal) const
{
  fxImpl->logStats(logVal);
}

void TentaclesFx::apply(Pixel* prevBuff, Pixel* currentBuff)
{
  if (!enabled)
  {
    return;
  }

  fxImpl->update(prevBuff, currentBuff);
}

void TentaclesFx::applyNoDraw()
{
  if (!enabled)
  {
    return;
  }

  fxImpl->updateWithNoDraw();
}

std::string TentaclesFx::getFxName() const
{
  return "Tentacles FX";
}

void TentaclesFx::saveState(std::ostream&) const
{
}

void TentaclesFx::loadState(std::istream&)
{
}

TentaclesFx::TentaclesImpl::TentaclesImpl(const PluginInfo* info)
  : goomInfo{info},
    colorMaps{Weights<ColorMapGroup>{{
        {ColorMapGroup::perceptuallyUniformSequential, 10},
        {ColorMapGroup::sequential, 10},
        {ColorMapGroup::sequential2, 10},
        {ColorMapGroup::cyclic, 10},
        {ColorMapGroup::diverging, 20},
        {ColorMapGroup::diverging_black, 20},
        {ColorMapGroup::qualitative, 10},
        {ColorMapGroup::misc, 20},
    }}},
    dominantColorMap{&colorMaps.getRandomColorMap()},
    dominantColor{ColorMap::getRandomColor(*dominantColorMap)},
    rot{getStableRotationOffset(0)},
    drivers{},
    driverWeights{{
        {0, 5},
        {1, 15},
        {2, 15},
        {3, 15},
    }},
    stats{}
{
  // clang-format off
  const std::vector<CirclesTentacleLayout> layouts{
      {10,  60, {16, 10,  6,  4, 2}, 0},
      {10,  60, {20, 16, 10,  4, 2}, 0},
      {10,  80, {30, 20, 14,  6, 4}, 0},
      {10,  90, {40, 26, 20, 12, 6}, 0},
  };
  // clang-format on
  // const GridTentacleLayout layout{ -100, 100, xRowLen, -100, 100, numXRows, 0 };
  if (numDrivers != layouts.size())
  {
    throw std::logic_error("numDrivers != layouts.size()");
  }

  /**
// Temp hack of weights
Weights<ColorMapGroup> colorGroupWeights = colorMaps.getWeights();
colorGroupWeights.clearWeights(1);
colorGroupWeights.setWeight(ColorMapGroup::misc, 30000);
colorMaps.setWeights(colorGroupWeights);
***/

  for (size_t i = 0; i < numDrivers; i++)
  {
    drivers.emplace_back(new TentacleDriver{&colorMaps, goomInfo->getScreenInfo().width,
                                            goomInfo->getScreenInfo().height});
  }

  if (numDrivers != driverWeights.getNumElements())
  {
    throw std::logic_error("numDrivers != driverWeights.getNumElements()");
  }

  for (size_t i = 0; i < numDrivers; i++)
  {
    drivers[i]->init(layouts[i]);
    drivers[i]->startIterating();
  }
  stats.setNumTentacleDrivers(drivers);

  currentDriver = getNextDriver();
  currentDriver->startIterating();
  init();
}

inline void TentaclesFx::TentaclesImpl::setBuffSettings(const FXBuffSettings& settings)
{
  currentDriver->setBuffSettings(settings);
}

inline void TentaclesFx::TentaclesImpl::incCounters()
{
  countSinceHighAccelLastMarked++;
  countSinceColorChangeLastMarked++;
}

void TentaclesFx::TentaclesImpl::logStats(const StatsLogValueFunc logVal)
{
  stats.setLastNumTentacles(currentDriver->getNumTentacles());
  stats.setLastUpdatingWithDraw(updatingWithDraw);
  stats.setLastCycle(cycle);
  stats.setLastCycleInc(cycleInc);
  stats.setLastLig(lig);
  stats.setLastLigs(ligs);
  stats.setLastDistt(distt);
  stats.setLastDistt2(distt2);
  stats.setLastDistt2Offset(distt2Offset);
  stats.setLastRot(rot);
  stats.setLastRotAtStartOfPrettyMove(rotAtStartOfPrettyMove);
  stats.setLastDoRotation(doRotation);
  stats.setLastIsPrettyMoveHappening(isPrettyMoveHappening);
  stats.setLastPrettyMoveHappeningTimer(prettyMoveHappeningTimer);
  stats.setLastPrettyMoveCheckStopMark(prettyMoveCheckStopMark);
  stats.setLastPrePrettyMoveLock(prePrettyMoveLock);
  stats.setLastDistt2OffsetPreStep(distt2OffsetPreStep);
  stats.setLastPrettyMoveReadyToStart(prettyMoveReadyToStart);
  stats.setLastPostPrettyMoveLock(postPrettyMoveLock);
  stats.setLastPrettyLerpMixValue(prettyMoveLerpMix);

  stats.log(logVal);
}

void TentaclesFx::TentaclesImpl::updateWithNoDraw()
{
  stats.updateWithNoDraw();

  if (!updatingWithDraw)
  {
    // already in tentacle no draw state
    return;
  }

  updatingWithDraw = false;

  if (ligs > 0.0f)
  {
    ligs = -ligs;
  }
  lig += ligs;

  changeDominantColor();
}

void TentaclesFx::TentaclesImpl::init()
{
  currentDriver->setRoughTentacles(false);
  currentDriver->freshStart();
  //  currentDriver->setColorMode(static_cast<TentacleDriver::ColorModes>(getRandInRange(0u, 2u)));
  currentDriver->setColorMode(static_cast<TentacleDriver::ColorModes>(getRandInRange(1u, 2u)));
  //  currentDriver->setColorMode(TentacleDriver::ColorModes::oneGroupForAll);
  currentDriver->setReverseColorMix(probabilityOfMInN(1, 2));

  distt = std::lerp(disttMin, disttMax, 0.3);
  distt2 = distt2Min;
  distt2Offset = 0;
  rot = getStableRotationOffset(0);

  prePrettyMoveLock = 0;
  postPrettyMoveLock = 0;
  prettyMoveReadyToStart = false;
  if (probabilityOfMInN(1, 5))
  {
    isPrettyMoveHappening = false;
    prettyMoveHappeningTimer = 0;
  }
  else
  {
    isPrettyMoveHappening = true;
    prettyMoveStart(1.0, prettyMoveHappeningMin / 2);
  }
}

void TentaclesFx::TentaclesImpl::update(Pixel* prevBuff, Pixel* currentBuff)
{
  logDebug("Starting update.");

  incCounters();

  stats.updateWithDraw();
  lig += ligs;

  if (!updatingWithDraw)
  {
    updatingWithDraw = true;
    init();
  }

  if (lig <= 1.01f)
  {
    lig = 1.05f;
    if (ligs < 0.0f)
    {
      ligs = -ligs;
    }

    logDebug("Starting pretty_move without draw.");
    stats.updateWithPrettyMoveNoDraw();
    prettyMove(goomInfo->getSoundInfo().getAcceleration());

    cycle += 10.0F * cycleInc;
    if (cycle > 1000)
    {
      stats.cycleReset();
      cycle = 0;
    }
  }
  else
  {
    if ((lig > 10.0f) || (lig < 1.1f))
    {
      ligs = -ligs;
    }

    logDebug("Starting pretty_move and draw.");
    stats.updateWithPrettyMoveDraw();
    prettyMove(goomInfo->getSoundInfo().getAcceleration());
    cycle += cycleInc;

    if (isPrettyMoveHappening || changeDominantColorMapEvent())
    {
      // IMPORTANT. Very delicate here - seems the right time to change maps.
      stats.changeDominantColorMap();
      dominantColorMap = &colorMaps.getRandomColorMap();
    }

    if ((isPrettyMoveHappening || (lig < 6.3f)) && changeDominantColorEvent())
    {
      changeDominantColor();
      countSinceColorChangeLastMarked = 0;
    }

    const auto [modColor, modColorLow] = getModColors();

    if (goomInfo->getSoundInfo().getTimeSinceLastGoom() != 0)
    {
      stats.lowToMediumAcceleration();
    }
    else
    {
      stats.highAcceleration();
      if (countSinceHighAccelLastMarked > 100)
      {
        countSinceHighAccelLastMarked = 0;
        if (countSinceColorChangeLastMarked > 100)
        {
          changeDominantColor();
          countSinceColorChangeLastMarked = 0;
        }
      }
    }

    // Higher sound acceleration increases tentacle wave frequency.
    const float tentacleWaveFreq =
        goomInfo->getSoundInfo().getAcceleration() < 0.3
            ? 1.25F
            : 1.0F / (1.10F - goomInfo->getSoundInfo().getAcceleration());
    currentDriver->multiplyIterZeroYValWaveFreq(tentacleWaveFreq);

    currentDriver->update(m_half_pi - rot, distt, distt2, modColor, modColorLow, prevBuff,
                          currentBuff);
  }
}

void TentaclesFx::TentaclesImpl::changeDominantColor()
{
  stats.changeDominantColor();
  const Pixel newColor = ColorMap::getRandomColor(*dominantColorMap);
  dominantColor = ColorMap::colorMix(dominantColor, newColor, 0.7);
}

inline std::tuple<Pixel, Pixel> TentaclesFx::TentaclesImpl::getModColors()
{
  // IMPORTANT. getEvolvedColor works just right - not sure why
  dominantColor = getEvolvedColor(dominantColor);

  const Pixel modColor = getLightenedColor(dominantColor, lig * 2.0f + 2.0f);
  const Pixel modColorLow = getLightenedColor(modColor, (lig / 2.0f) + 0.67f);

  return std::make_tuple(modColor, modColorLow);
}

void TentaclesFx::TentaclesImpl::prettyMovePreStart()
{
  prePrettyMoveLock = getRandInRange(minPrePrettyMoveLock, maxPrePrettyMoveLock);
  distt2OffsetPreStep =
      std::lerp(distt2Min, distt2Max, 0.2f) / static_cast<float>(prePrettyMoveLock);
  distt2Offset = 0;
}

void TentaclesFx::TentaclesImpl::prettyMoveStart(const float acceleration, const int32_t timerVal)
{
  stats.prettyMoveHappens();

  if (timerVal != -1)
  {
    prettyMoveHappeningTimer = timerVal;
  }
  else
  {
    prettyMoveHappeningTimer =
        static_cast<int>(getRandInRange(prettyMoveHappeningMin, prettyMoveHappeningMax));
  }
  prettyMoveCheckStopMark = prettyMoveHappeningTimer / 4;
  postPrettyMoveLock = 3 * prettyMoveHappeningTimer / 2;

  distt2Offset = (1.0F / (1.10F - acceleration)) * getRandInRange(distt2Min, distt2Max);
  rotAtStartOfPrettyMove = rot;
  cycleInc = getRandInRange(cycleIncMin, cycleIncMax);
  currentDriver->setRoughTentacles(true);
}

/****
inline float getRapport(const float accelvar)
{
  return std::min(1.12f, 1.2f * (1.0f + 2.0f * (accelvar - 1.0f)));
}
****/

void TentaclesFx::TentaclesImpl::prettyMoveFinish()
{
  prettyMoveHappeningTimer = 0;
  distt2Offset = 0;
  currentDriver->setRoughTentacles(false);
}

void TentaclesFx::TentaclesImpl::isPrettyMoveHappeningUpdate(const float acceleration)
{
  // Are we in a prettyMove?
  if (prettyMoveHappeningTimer > 0)
  {
    prettyMoveHappeningTimer -= 1;
    return;
  }

  // Not in a pretty move. Have we just finished?
  if (isPrettyMoveHappening)
  {
    isPrettyMoveHappening = false;
    prettyMoveFinish();
    return;
  }

  // Are we locked after prettyMove finished?
  if (postPrettyMoveLock != 0)
  {
    postPrettyMoveLock--;
    return;
  }

  // Are we locked after prettyMove start signal?
  if (prePrettyMoveLock != 0)
  {
    prePrettyMoveLock--;
    distt2Offset += distt2OffsetPreStep;
    return;
  }

  // Are we ready to start a prettyMove?
  if (prettyMoveReadyToStart)
  {
    prettyMoveReadyToStart = false;
    isPrettyMoveHappening = true;
    prettyMoveStart(acceleration);
    return;
  }

  // Are we ready to signal a prettyMove start?
  if (startPrettyMoveEvent())
  {
    prettyMoveReadyToStart = true;
    prettyMovePreStart();
    return;
  }
}

inline TentacleDriver* TentaclesFx::TentaclesImpl::getNextDriver() const
{
  const size_t driverIndex = driverWeights.getRandomWeighted();
  stats.changeTentacleDriver(driverIndex);
  return drivers[driverIndex].get();
}

inline float TentaclesFx::TentaclesImpl::getStableRotationOffset(const float cycleVal)
{
  return (1.5F + sin(cycleVal) / 32.0F) * m_pi;
}

void TentaclesFx::TentaclesImpl::prettyMove(const float acceleration)
{
  isPrettyMoveHappeningUpdate(acceleration);

  if (isPrettyMoveHappening && (prettyMoveHappeningTimer == prettyMoveCheckStopMark))
  {
    currentDriver = getNextDriver();
  }

  distt2 = std::lerp(distt2, distt2Offset, prettyMoveLerpMix);

  // Bigger offset here means tentacles start further back behind screen.
  float disttOffset = std::lerp(disttMin, disttMax, 0.5F * (1.0F - sin(cycle * 19.0 / 20.0)));
  if (isPrettyMoveHappening)
  {
    disttOffset *= 0.6F;
  }
  distt = std::lerp(distt, disttOffset, 4.0f * prettyMoveLerpMix);

  float rotOffset = 0;
  if (!isPrettyMoveHappening)
  {
    rotOffset = getStableRotationOffset(cycle);
  }
  else
  {
    float currentCycle = cycle;
    if (changeRotationEvent())
    {
      doRotation = turnRotationOnEvent();
    }
    if (doRotation)
    {
      currentCycle *= m_two_pi;
    }
    else
    {
      currentCycle *= -m_pi;
    }
    rotOffset = std::fmod(currentCycle, m_two_pi);
    if (rotOffset < 0.0)
    {
      rotOffset += m_two_pi;
    }

    if (prettyMoveHappeningTimer < prettyMoveCheckStopMark)
    {
      // If close enough to initialStableRotationOffset, then stop the happening.
      if (std::fabs(rot - rotAtStartOfPrettyMove) < 0.1)
      {
        isPrettyMoveHappening = false;
        prettyMoveFinish();
      }
    }

    if (!(0.0 <= rotOffset && rotOffset <= m_two_pi))
    {
      throw std::logic_error(std20::format(
          "rotOffset {:.3f} not in [0, 2pi]: currentCycle = {:.3f}", rotOffset, currentCycle));
    }
  }

  rot = std::clamp(std::lerp(rot, rotOffset, prettyMoveLerpMix), 0.0f, m_two_pi);

  logInfo("happening = {}, lock = {}, oldRot = {:.03f}, rot = {:.03f}, rotOffset = {:.03f}, "
          "lerpMix = {:.03f}, cycle = {:.03f}, doRotation = {}",
          prettyMoveHappeningTimer, postPrettyMoveLock, oldRot, rot, rotOffset, prettyMoveLerpMix,
          cycle, doRotation);
}

} // namespace goom
