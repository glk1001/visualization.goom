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
#include <stdexcept>
#include <tuple>

namespace goom
{

using namespace goom::utils;

class TentacleStats
{
public:
  TentacleStats() noexcept = default;

  void reset();
  void log(const StatsLogValueFunc) const;
  void changeDominantColorMap();
  void changeDominantColor();
  void updateWithDraw();
  void updateWithPrettyMove1();
  void updateWithPrettyMove2();
  void lowToMediumAcceleration();
  void highAcceleration();
  void cycleReset();
  void setLastCycleValue(const float val);
  void prettyMoveHappensReset();
  void changePrettyLerpMixLower();
  void changePrettyLerpMixHigher();
  void setLastPrettyLerpMixValue(const float value);

private:
  uint32_t numDominantColorMapChanges = 0;
  uint32_t numDominantColorChanges = 0;
  uint32_t numUpdatesWithDraw = 0;
  uint32_t numUpdatesWithPrettyMove1 = 0;
  uint32_t numUpdatesWithPrettyMove2 = 0;
  uint32_t numLowToMediumAcceleration = 0;
  uint32_t numHighAcceleration = 0;
  uint32_t numCycleResets = 0;
  float lastCycleValue = 0;
  uint32_t numPrettyMoveHappensResets = 0;
  uint32_t numLowerPrettyLerpMixChanges = 0;
  uint32_t numHigherPrettyLerpMixChanges = 0;
  float lastPrettyLerpMixValue = 0;
};

void TentacleStats::log(const StatsLogValueFunc logVal) const
{
  const constexpr char* module = "Tentacles";

  logVal(module, "numDominantColorMapChanges", numDominantColorMapChanges);
  logVal(module, "numDominantColorChanges", numDominantColorChanges);
  logVal(module, "numUpdatesWithDraw", numUpdatesWithDraw);
  logVal(module, "numUpdatesWithPrettyMove1", numUpdatesWithPrettyMove1);
  logVal(module, "numUpdatesWithPrettyMove2", numUpdatesWithPrettyMove2);
  logVal(module, "numLowToMediumAcceleration", numLowToMediumAcceleration);
  logVal(module, "numHighAcceleration", numHighAcceleration);
  logVal(module, "numCycleResets", numCycleResets);
  logVal(module, "lastCycleValue", lastCycleValue);
  logVal(module, "numPrettyMoveHappensResets", numPrettyMoveHappensResets);
  logVal(module, "numLowerPrettyLerpMixChanges", numLowerPrettyLerpMixChanges);
  logVal(module, "numHigherPrettyLerpMixChanges", numHigherPrettyLerpMixChanges);
  logVal(module, "lastPrettyLerpMixValue", lastPrettyLerpMixValue);
}

void TentacleStats::reset()
{
  numDominantColorMapChanges = 0;
  numDominantColorChanges = 0;
  numUpdatesWithDraw = 0;
  numUpdatesWithPrettyMove1 = 0;
  numUpdatesWithPrettyMove2 = 0;
  numLowToMediumAcceleration = 0;
  numHighAcceleration = 0;
  numCycleResets = 0;
  lastCycleValue = 0;
  numPrettyMoveHappensResets = 0;
  numLowerPrettyLerpMixChanges = 0;
  numHigherPrettyLerpMixChanges = 0;
  lastPrettyLerpMixValue = 0;
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

inline void TentacleStats::updateWithPrettyMove1()
{
  numUpdatesWithPrettyMove1++;
}

inline void TentacleStats::updateWithPrettyMove2()
{
  numUpdatesWithPrettyMove2++;
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

inline void TentacleStats::prettyMoveHappensReset()
{
  numPrettyMoveHappensResets++;
}

inline void TentacleStats::changePrettyLerpMixLower()
{
  numLowerPrettyLerpMixChanges++;
}

inline void TentacleStats::changePrettyLerpMixHigher()
{
  numHigherPrettyLerpMixChanges++;
}

inline void TentacleStats::setLastPrettyLerpMixValue(const float value)
{
  lastPrettyLerpMixValue = value;
}

class TentaclesWrapper
{
public:
  explicit TentaclesWrapper(const uint32_t screenWidth, const uint32_t screenHeight);
  ~TentaclesWrapper() noexcept = default;
  void update(PluginInfo*,
              Pixel* frontBuff,
              Pixel* backBuff,
              const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN],
              const float accelVar,
              const bool doDraw);
  void logStats(const StatsLogValueFunc logVal);

private:
  WeightedColorMaps colorMaps;
  const ColorMap* dominantColorMap;
  uint32_t dominantColor = 0;

  float cycle = 0;
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
  static constexpr uint32_t prettyMoveHappeningMin = 130;
  static constexpr uint32_t prettyMoveHappeningMax = 200;
  int32_t postPrettyMoveLock = 0;
  float prettyMoveLerpMix = 1.0 / 16.0; // original goom value
  static constexpr size_t changePrettyLerpMixMark = 1000000; // big number means never change
  void isPrettyMoveHappeningUpdate(const float accelVar);
  void prettyMoveStarted(const float accelVar);
  void prettyMoveFinished();
  void prettyMoveUnlocked();
  void prettyMove(const float accelVar);
  std::tuple<uint32_t, uint32_t> getModColors();

  size_t countSincePrettyLerpMixMarked = 0;
  size_t countSinceHighAccelLastMarked = 0;
  size_t countSinceColorChangeLastMarked = 0;
  void incCounters();

  static constexpr float highAcceleration = 0.7;
  TentacleDriver driver;
  TentacleStats stats;
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
    rot{TentaclesWrapper::getStableRotationOffset(0)},
    driver{&colorMaps, screenWidth, screenHeight},
    stats{}
{
  /**
// Temp hack of weights
Weights<ColorMapGroup> colorGroupWeights = colorMaps.getWeights();
colorGroupWeights.clearWeights(1);
colorGroupWeights.setWeight(ColorMapGroup::misc, 30000);
colorMaps.setWeights(colorGroupWeights);
***/

  driver.init();
  driver.startIterating();
}

inline void TentaclesWrapper::incCounters()
{
  countSincePrettyLerpMixMarked++;
  countSinceHighAccelLastMarked++;
  countSinceColorChangeLastMarked++;
}

void TentaclesWrapper::logStats(const StatsLogValueFunc logVal)
{
  stats.setLastCycleValue(cycle);
  stats.setLastPrettyLerpMixValue(prettyMoveLerpMix);
  stats.log(logVal);
}

void TentaclesWrapper::update(PluginInfo* goomInfo,
                              Pixel* frontBuff,
                              Pixel* backBuff,
                              const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN],
                              const float accelVar,
                              const bool doDraw)
{
  logDebug("Starting update.");

  incCounters();

  if (!doDraw && (ligs > 0.0f))
  {
    ligs = -ligs;
  }
  lig += ligs;

  if (lig <= 1.01f)
  {
    lig = 1.05f;
    if (ligs < 0.0f)
    {
      ligs = -ligs;
    }

    logDebug("Starting pretty_move 1.");
    stats.updateWithPrettyMove1();
    prettyMove(accelVar);

    cycle += 0.1f;
    if (cycle > 1000)
    {
      stats.cycleReset();
      cycle = 0;
    }
  }
  else
  {
    logDebug("Starting pretty_move 2.");
    stats.updateWithPrettyMove2();
    prettyMove(accelVar);

    const auto [modColor, modColorLow] = getModColors();

    if (goomInfo->sound.timeSinceLastBigGoom != 0)
    {
      stats.lowToMediumAcceleration();
      if (countSincePrettyLerpMixMarked > changePrettyLerpMixMark)
      {
        // TODO Make random?
        prettyMoveLerpMix = 1.0 / 16.0; // original goom value
        stats.changePrettyLerpMixLower();
        countSincePrettyLerpMixMarked = 0;
      }
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
      if (countSincePrettyLerpMixMarked > changePrettyLerpMixMark)
      {
        // TODO Make random?
        // 0.5 is magic - gives star mode - not sure why.
        prettyMoveLerpMix = 0.5;
        stats.changePrettyLerpMixHigher();
        countSincePrettyLerpMixMarked = 0;
      }
    }

    // Higher sound acceleration increases tentacle wave frequency.
    driver.multiplyIterZeroYValWaveFreq(1.0 / (1.10 - accelVar));

    cycle += 0.01f;

    if (doDraw)
    {
      stats.updateWithDraw();
    }

    driver.update(doDraw, m_half_pi - rot, distt, distt2, modColor, modColorLow, frontBuff,
                  backBuff);
  }
}

void TentaclesWrapper::prettyMoveStarted(const float accelVar)
{
  prettyMoveHappeningTimer =
      static_cast<int>(getRandInRange(prettyMoveHappeningMin, prettyMoveHappeningMax));
  prettyMoveCheckStopMark = prettyMoveHappeningTimer / 4;
  postPrettyMoveLock = 3 * prettyMoveHappeningTimer / 2;

  distt2Offset = (1.0 / (1.10 - accelVar)) * getRandInRange(distt2Min, distt2Max);
  rotAtStartOfPrettyMove = rot;
  driver.setRoughTentacles(true);
}

void TentaclesWrapper::prettyMoveFinished()
{
  stats.prettyMoveHappensReset();
  prettyMoveHappeningTimer = 0;
  distt2Offset = 0;
  driver.setRoughTentacles(false);
}

void TentaclesWrapper::isPrettyMoveHappeningUpdate(const float accelVar)
{
  if (prettyMoveHappeningTimer > 0)
  {
    prettyMoveHappeningTimer -= 1;
    return;
  }

  // Not in a pretty move. Have we just finished?
  if (isPrettyMoveHappening)
  {
    isPrettyMoveHappening = false;
    prettyMoveFinished();
  }
  else if (postPrettyMoveLock != 0)
  {
    postPrettyMoveLock--;
  }
  else if (probabilityOfMInN(1, 200))
  {
    isPrettyMoveHappening = true;
    prettyMoveStarted(accelVar);
  }
  else
  {
    postPrettyMoveLock = 0;
  }
}

inline float TentaclesWrapper::getStableRotationOffset(const float cycleVal)
{
  return (1.5 + sin(cycleVal) / 32.0) * m_pi;
}

// TODO - Make this prettier
void TentaclesWrapper::prettyMove(const float accelVar)
{
  /* many magic numbers here... I don't really like that. */
  isPrettyMoveHappeningUpdate(accelVar);

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
    if (probabilityOfMInN(1, 100))
    {
      doRotation = probabilityOfMInN(1, 2);
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
        prettyMoveFinished();
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
  if (isPrettyMoveHappening)
  {
    // IMPORTANT. Very delicate here - seems the right time to change maps.
    stats.changeDominantColorMap();
    dominantColorMap = &colorMaps.getRandomColorMap();
  }

  if ((lig > 10.0f) || (lig < 1.1f))
  {
    ligs = -ligs;
  }

  // IMPORTANT. Very delicate here - 1 in 30 seems just right
  if ((lig < 6.3f) && probabilityOfMInN(1, 30))
  {
    stats.changeDominantColor();
    // TODO - Too abrupt color change here may cause flicker - maybe compare 'luma'
    //   Tried below but somehow its make color change way less interesting.
    /***
    Pixel oldColor{ .val = dominantColor };
    const float oldLuma = 0.299*oldColor.channels.r + 0.587*oldColor.channels.g + 0.114*oldColor.channels.b;
    dominantColor = ColorMap::getRandomColor(*dominantColorMap);
    Pixel pix{ .val = dominantColor };
    const float luma = 0.299*pix.channels.r + 0.587*pix.channels.g + 0.114*pix.channels.b;
    pix.channels.r = (oldLuma/luma)*pix.channels.r;
    pix.channels.g = (oldLuma/luma)*pix.channels.g;
    pix.channels.b = (oldLuma/luma)*pix.channels.b;
    dominantColor = pix.val;
    **/
    dominantColor = ColorMap::getRandomColor(*dominantColorMap);
    countSinceColorChangeLastMarked = 0;
  }

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
    data->tentacles->update(goomInfo, dest, src, goomInfo->sound.samples, goomInfo->sound.accelvar,
                            goomInfo->curGDrawables.count(GoomDrawable::tentacles));
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
