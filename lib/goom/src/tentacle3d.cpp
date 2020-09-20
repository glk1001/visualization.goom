#include "tentacle3d.h"

#include "colorutils.h"
#include "goom_config.h"
#include "goom_core.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "goomutils/colormap.h"
#include "goomutils/logging_control.h"
#include "goomutils/mathutils.h"
//#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "tentacle_driver.h"
#include "tentacles_new.h"
#include "v3d.h"

#include <cmath>
#include <cstdint>
#include <tuple>

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
  void happensReset();
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
  uint32_t numHappensResets = 0;
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
  logVal(module, "numHappensResets", numHappensResets);
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
  numHappensResets = 0;
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

inline void TentacleStats::happensReset()
{
  numHappensResets++;
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
  static constexpr float distt2Max = 500;
  float distt2Offset = 0;
  float rot = 0; // entre 0 et m_2pi
  int32_t isPrettyMoveHappening = 0;
  static constexpr size_t prettyMoveHappeningMin = 100;
  static constexpr size_t prettyMoveHappeningMax = 160;
  int32_t postPrettyMoveLock = 0;
  float prettyMoveLerpMix = 1.0 / 16.0; // original goom value
  static constexpr size_t changePrettyLerpMixMark = 1000000; // big number means never change
  void isPrettyMoveHappeningUpdate(PluginInfo*, const float accelVar);
  void prettyMove(PluginInfo*, const float accelVar);

  size_t countSincePrettyLerpMixMarked = 0;
  size_t countSinceHighAccelLastMarked = 0;
  size_t countSinceColorChangeLastMarked = 0;
  void incCounters();

  static constexpr float highAcceleration = 0.7;
  TentacleDriver driver;
  TentacleStats stats;
  std::tuple<uint32_t, uint32_t> getModColors(PluginInfo*);
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
  stats.setLastPrettyLerpMixValue(prettyMoveLerpMix);
  stats.log(logVal);
}

inline float randFactor(PluginInfo* goomInfo, const float min)
{
  return min + (1.0 - min) * static_cast<float>(goomInfo->getNRand(101)) / 100.0;
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
    prettyMove(goomInfo, accelVar);

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
    prettyMove(goomInfo, accelVar);

    const auto [modColor, modColorLow] = getModColors(goomInfo);

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

void TentaclesWrapper::isPrettyMoveHappeningUpdate(PluginInfo* goomInfo, const float accelVar)
{
  if (isPrettyMoveHappening)
  {
    isPrettyMoveHappening -= 1;
  }
  else if (postPrettyMoveLock != 0)
  {
    postPrettyMoveLock--;
    distt2Offset = 0;
  }
  else
  {
    stats.happensReset();
    if (probabilityOfMInN(goomInfo, 199, 200))
    {
      isPrettyMoveHappening = 0;
      postPrettyMoveLock = 0;
      distt2Offset = 0;
    }
    else
    {
      isPrettyMoveHappening = static_cast<int>(
          goomInfo->getRandInRange(prettyMoveHappeningMin, prettyMoveHappeningMax));
      postPrettyMoveLock = 3 * isPrettyMoveHappening / 2;
      distt2Offset = (1.0 / (1.10 - accelVar)) * goomInfo->getRandInRange(distt2Min, distt2Max);
    }
  }
}

// TODO - Make this prettier
void TentaclesWrapper::prettyMove(PluginInfo* goomInfo, const float accelVar)
{
  /* many magic numbers here... I don't really like that. */
  isPrettyMoveHappeningUpdate(goomInfo, accelVar);

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
    rotOffset = (1.5 + sin(cycle) / 32.0) * m_pi;
  }
  else
  {
    float currentCycle = cycle;
    if (probabilityOfMInN(goomInfo, 1, 1000))
    {
      // do rotation
      currentCycle *= m_two_pi;
    }
    else
    {
      currentCycle *= -m_pi;
    }
    rotOffset = m_two_pi * getFractPart(currentCycle / m_two_pi);
  }

  const float rotLerpMix = prettyMoveLerpMix;
  if (std::fabs(rot - rotOffset) > std::fabs(rot + m_two_pi - rotOffset))
  {
    rot = std::lerp(rot + m_two_pi, rotOffset, rotLerpMix);
    if (rot > m_two_pi)
    {
      rot -= m_two_pi;
    }
  }
  else if (std::fabs(rot - rotOffset) > std::fabs(rot - m_two_pi - rotOffset))
  {
    rot = std::lerp(rot - m_two_pi, rotOffset, rotLerpMix);
    if (rot < 0.0f)
    {
      rot += m_two_pi;
    }
  }
  else
  {
    rot = std::lerp(rot, rotOffset, rotLerpMix);
  }
}

inline std::tuple<uint32_t, uint32_t> TentaclesWrapper::getModColors(PluginInfo* goomInfo)
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
  if ((lig < 6.3f) && probabilityOfMInN(goomInfo, 1, 30))
  {
    stats.changeDominantColor();
    // TODO - To abrupt color change here may cause flicker - maybe compare 'luma'
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
