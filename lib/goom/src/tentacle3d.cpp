#include "tentacle3d.h"

#include "colorutils.h"
#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goomutils/colormap.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
#include "goomutils/mathutils.h"
//#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "tentacles.h"
#include "tentacles_driver.h"
#include "v3d.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <numeric>
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
  void setLastCycleValue(const float val);
  void prettyMoveHappens();
  void changePrettyLerpMixLower();
  void changePrettyLerpMixHigher();
  void setLastPrettyLerpMixValue(const float value);
  void setNumTentacleDrivers(const std::vector<std::unique_ptr<TentacleDriver>>&);
  void changeTentacleDriver(const uint32_t driverIndex);

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
  float lastCycleValue = 0;
  uint32_t numPrettyMoveHappens = 0;
  float lastPrettyLerpMixValue = 0;
  uint32_t numTentacleDrivers = 0;
  std::vector<uint32_t> numDriverTentacles{};
  std::vector<uint32_t> numDriverChanges{};
};

void TentacleStats::log(const StatsLogValueFunc logVal) const
{
  const constexpr char* module = "Tentacles";

  logVal(module, "numDominantColorMapChanges", numDominantColorMapChanges);
  logVal(module, "numDominantColorChanges", numDominantColorChanges);
  logVal(module, "numUpdatesWithDraw", numUpdatesWithDraw);
  logVal(module, "numUpdatesWithNoDraw", numUpdatesWithNoDraw);
  logVal(module, "numUpdatesWithPrettyMoveNoDraw", numUpdatesWithPrettyMoveNoDraw);
  logVal(module, "numUpdatesWithPrettyMoveDraw", numUpdatesWithPrettyMoveDraw);
  logVal(module, "numLowToMediumAcceleration", numLowToMediumAcceleration);
  logVal(module, "numHighAcceleration", numHighAcceleration);
  logVal(module, "numCycleResets", numCycleResets);
  logVal(module, "lastCycleValue", lastCycleValue);
  logVal(module, "numPrettyMoveHappens", numPrettyMoveHappens);
  logVal(module, "lastPrettyLerpMixValue", lastPrettyLerpMixValue);
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
  lastCycleValue = 0;
  numPrettyMoveHappens = 0;
  lastPrettyLerpMixValue = 0;
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

inline void TentacleStats::setLastCycleValue(const float val)
{
  lastCycleValue = val;
}

inline void TentacleStats::prettyMoveHappens()
{
  numPrettyMoveHappens++;
}

inline void TentacleStats::setLastPrettyLerpMixValue(const float value)
{
  lastPrettyLerpMixValue = value;
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

class TentaclesWrapper
{
public:
  explicit TentaclesWrapper(const uint32_t screenWidth, const uint32_t screenHeight);
  ~TentaclesWrapper() noexcept = default;
  void update(PluginInfo*,
              Pixel* frontBuff,
              Pixel* backBuff,
              const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN]);
  void updateWithNoDraw(PluginInfo*);
  void logStats(const StatsLogValueFunc logVal);

private:
  WeightedColorMaps colorMaps;
  const ColorMap* dominantColorMap;
  uint32_t dominantColor = 0;

  bool updatingWithDraw = true;
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
  void isPrettyMoveHappeningUpdate(const float accelVar);
  void prettyMovePreStart();
  void prettyMoveStart(const float accelVar);
  void prettyMoveFinish();
  void prettyMove(const float accelVar);
  void prettyMoveWithNoDraw(PluginInfo*);
  std::tuple<uint32_t, uint32_t> getModColors();

  size_t countSinceHighAccelLastMarked = 0;
  size_t countSinceColorChangeLastMarked = 0;
  void incCounters();

  static constexpr size_t numDrivers = 4;
  std::vector<std::unique_ptr<TentacleDriver>> drivers;
  TentacleDriver* currentDriver = nullptr;
  TentacleDriver* getNextDriver() const;
  const Weights<size_t> driverWeights;

  mutable TentacleStats stats;
};

TentaclesWrapper::TentaclesWrapper(const uint32_t screenWidth, const uint32_t screenHeight)
  : colorMaps{Weights<ColorMapGroup>{{
        {ColorMapGroup::perceptuallyUniformSequential, 10},
        {ColorMapGroup::sequential, 5},
        {ColorMapGroup::sequential2, 5},
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
        {3, 5},
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
    drivers.emplace_back(new TentacleDriver{&colorMaps, screenWidth, screenHeight});
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
}

inline void TentaclesWrapper::incCounters()
{
  countSinceHighAccelLastMarked++;
  countSinceColorChangeLastMarked++;
}

void TentaclesWrapper::logStats(const StatsLogValueFunc logVal)
{
  stats.setLastCycleValue(cycle);
  stats.setLastPrettyLerpMixValue(prettyMoveLerpMix);
  stats.log(logVal);
}

void TentaclesWrapper::updateWithNoDraw(PluginInfo*)
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

  stats.changeDominantColor();
  dominantColor = ColorMap::getRandomColor(*dominantColorMap);
}

void TentaclesWrapper::update(PluginInfo* goomInfo,
                              Pixel* frontBuff,
                              Pixel* backBuff,
                              const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN])
{
  logDebug("Starting update.");

  incCounters();

  stats.updateWithDraw();
  lig += ligs;

  if (!updatingWithDraw)
  {
    updatingWithDraw = true;
    distt = std::midpoint(disttMin, disttMax);
    distt2 = 0;
    distt2Offset = 0;
    rot = getStableRotationOffset(0);
    prettyMoveReadyToStart = false;
    prePrettyMoveLock = 0;
    postPrettyMoveLock = 0;
    isPrettyMoveHappening = false;
    prettyMoveHappeningTimer = 0;
    currentDriver->setRoughTentacles(false);
    currentDriver->freshStart();
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
    prettyMove(goomInfo->sound.accelvar);

    cycle += 10.0 * cycleInc;
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
    prettyMove(goomInfo->sound.accelvar);
    cycle += cycleInc;

    if (isPrettyMoveHappening || changeDominantColorMapEvent())
    {
      // IMPORTANT. Very delicate here - seems the right time to change maps.
      stats.changeDominantColorMap();
      dominantColorMap = &colorMaps.getRandomColorMap();
    }

    if ((isPrettyMoveHappening || (lig < 6.3f)) && changeDominantColorEvent())
    {
      stats.changeDominantColor();
      dominantColor = ColorMap::getRandomColor(*dominantColorMap);
      countSinceColorChangeLastMarked = 0;
    }

    const auto [modColor, modColorLow] = getModColors();

    if (goomInfo->sound.timeSinceLastGoom != 0)
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
          stats.changeDominantColor();
          dominantColor = ColorMap::getRandomColor(*dominantColorMap);
          countSinceColorChangeLastMarked = 0;
        }
      }
    }

    // Higher sound acceleration increases tentacle wave frequency.
    currentDriver->multiplyIterZeroYValWaveFreq(1.0 / (1.10 - goomInfo->sound.accelvar));

    currentDriver->update(m_half_pi - rot, distt, distt2, modColor, modColorLow, frontBuff,
                          backBuff);
  }
}

void TentaclesWrapper::prettyMovePreStart()
{
  prePrettyMoveLock = getRandInRange(minPrePrettyMoveLock, maxPrePrettyMoveLock);
  distt2OffsetPreStep =
      std::lerp(distt2Min, distt2Max, 0.2f) / static_cast<float>(prePrettyMoveLock);
  distt2Offset = 0;
}

void TentaclesWrapper::prettyMoveStart(const float accelVar)
{
  stats.prettyMoveHappens();

  prettyMoveHappeningTimer =
      static_cast<int>(getRandInRange(prettyMoveHappeningMin, prettyMoveHappeningMax));
  prettyMoveCheckStopMark = prettyMoveHappeningTimer / 4;
  postPrettyMoveLock = 3 * prettyMoveHappeningTimer / 2;

  distt2Offset = (1.0 / (1.10 - accelVar)) * getRandInRange(distt2Min, distt2Max);
  rotAtStartOfPrettyMove = rot;
  cycleInc = getRandInRange(cycleIncMin, cycleIncMax);
  currentDriver->setRoughTentacles(true);
}

void TentaclesWrapper::prettyMoveFinish()
{
  prettyMoveHappeningTimer = 0;
  distt2Offset = 0;
  currentDriver->setRoughTentacles(false);
}

void TentaclesWrapper::isPrettyMoveHappeningUpdate(const float accelVar)
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
    prettyMoveStart(accelVar);
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

inline TentacleDriver* TentaclesWrapper::getNextDriver() const
{
  const size_t driverIndex = driverWeights.getRandomWeighted();
  stats.changeTentacleDriver(driverIndex);
  return drivers[driverIndex].get();
}

inline float TentaclesWrapper::getStableRotationOffset(const float cycleVal)
{
  return (1.5 + sin(cycleVal) / 32.0) * m_pi;
}

void TentaclesWrapper::prettyMove(const float accelVar)
{
  isPrettyMoveHappeningUpdate(accelVar);

  if (prettyMoveHappeningTimer == prettyMoveCheckStopMark)
  {
    currentDriver = getNextDriver();
  }

  distt2 = std::lerp(distt2, distt2Offset, prettyMoveLerpMix);

  // Bigger offset here means tentacles start further back behind screen.
  float disttOffset = std::lerp(disttMin, disttMax, 0.5 * (1.0 - sin(cycle * 19.0 / 20.0)));
  if (isPrettyMoveHappening)
  {
    disttOffset *= 0.6f;
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

inline std::tuple<uint32_t, uint32_t> TentaclesWrapper::getModColors()
{
  // IMPORTANT. getEvolvedColor works just right - not sure why
  dominantColor = getEvolvedColor(dominantColor);

  const uint32_t modColor = getLightenedColor(dominantColor, lig * 2.0f + 2.0f);
  const uint32_t modColorLow = getLightenedColor(modColor, (lig / 2.0f) + 0.67f);

  return std::make_tuple(modColor, modColorLow);
}

/*
 * VisualFX wrapper for the tentacles
 */

void tentacle_fx_init(VisualFX* _this, PluginInfo* info)
{
  TentacleFXData* data = (TentacleFXData*)malloc(sizeof(TentacleFXData));

  data->tentacles = new TentaclesWrapper{info->screen.width, info->screen.height};

  data->enabled_bp = secure_b_param("Enabled", 1);
  data->params = plugin_parameters("3D Tentacles", 1);
  data->params.params[0] = &data->enabled_bp;

  _this->params = &data->params;
  _this->fx_data = data;
}

void tentacle_fx_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* goomInfo)
{
  TentacleFXData* data = static_cast<TentacleFXData*>(_this->fx_data);
  if (BVAL(data->enabled_bp))
  {
    data->tentacles->update(goomInfo, dest, src, goomInfo->sound.samples);
  }
}

void tentacle_fx_update_no_draw(VisualFX* _this, PluginInfo* goomInfo)
{
  TentacleFXData* data = static_cast<TentacleFXData*>(_this->fx_data);
  if (BVAL(data->enabled_bp))
  {
    data->tentacles->updateWithNoDraw(goomInfo);
  }
}

void tentacle_log_stats(VisualFX* _this, const StatsLogValueFunc logVal)
{
  TentacleFXData* data = static_cast<TentacleFXData*>(_this->fx_data);
  data->tentacles->logStats(logVal);
}

void tentacle_fx_free(VisualFX* _this)
{
  TentacleFXData* data = static_cast<TentacleFXData*>(_this->fx_data);
  free(data->params.params);
  tentacle_free(data);
  free(_this->fx_data);
}

VisualFX tentacle_fx_create(void)
{
  VisualFX fx;
  fx.init = tentacle_fx_init;
  fx.apply = tentacle_fx_apply;
  fx.free = tentacle_fx_free;
  fx.save = nullptr;
  fx.restore = nullptr;
  return fx;
}

void tentacle_free(TentacleFXData* data)
{
  delete data->tentacles;
}

} // namespace goom
