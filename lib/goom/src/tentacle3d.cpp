#include "tentacle3d.h"

#include "colorutils.h"
#include "goom_config.h"
#include "goom_core.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "goomutils/colormap.h"
#include "goomutils/mathutils.h"
#include "goomutils/logging_control.h"
#undef NO_LOGGING
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
  void changeColorMap();
  void changeTentacleColor();
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
  uint32_t numColorMapChanges = 0;
  uint32_t numTentacleColorChanges = 0;
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

  logVal(module, "numColorMapChanges", numColorMapChanges);
  logVal(module, "numTentacleColorChanges", numTentacleColorChanges);
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
  numColorMapChanges = 0;
  numTentacleColorChanges = 0;
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

inline void TentacleStats::changeColorMap()
{
  numColorMapChanges++;
}

inline void TentacleStats::changeTentacleColor()
{
  numTentacleColorChanges++;
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
  ~TentaclesWrapper();
  void update(PluginInfo*,
              Pixel* frontBuff,
              Pixel* backBuff,
              const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN],
              const float accelVar,
              const bool doDraw);
  void logStats(const StatsLogValueFunc logVal);

private:
  WeightedColorMaps colorMaps;
  const ColorMap* dominantColorGroup;

  float cycle = 0;
  uint32_t color = 0;
  float lig = 1.15;
  float ligs = 0.1;
  float distt = 10;
  float distt2 = 0;
  float rot = 0; // entre 0 et m_2pi
  int happens = 0;
  bool doRotation = false;
  int lock = 0;
  float prettyMoveLerpMix = 1.0 / 16.0;

  size_t countSincePrettyLerpMixMarked = 0;
  size_t countSinceHighAccelLastMarked = 0;
  void incCounters();

  static constexpr float highAcceleration = 0.7;
  TentacleDriver driver;
  TentacleStats stats;
  void happensUpdate(PluginInfo*);
  void prettyMove(PluginInfo*);
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
    dominantColorGroup{&colorMaps.getRandomColorMap()},
    color{ColorMap::getRandomColor(*dominantColorGroup)},
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

TentaclesWrapper::~TentaclesWrapper()
{
}

inline void TentaclesWrapper::incCounters()
{
  countSincePrettyLerpMixMarked++;
  countSinceHighAccelLastMarked++;
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

  if (lig > 1.01f)
  {
    logDebug("Starting pretty_move 1.");
    stats.updateWithPrettyMove1();
    prettyMove(goomInfo);

    if (accelVar < highAcceleration)
    {
      stats.lowToMediumAcceleration();
      if (countSincePrettyLerpMixMarked > 100)
      {
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
        stats.changeTentacleColor();
        color = ColorMap::getRandomColor(*dominantColorGroup);
      }
      if (countSincePrettyLerpMixMarked > 100)
      {
        // 0.5 is magic - gives star mode - not sure why.
        prettyMoveLerpMix = 0.5;
        stats.changePrettyLerpMixHigher();
        countSincePrettyLerpMixMarked = 0;
      }
    }

    // Higher sound acceleration increases tentacle wave frequency.
    driver.multiplyIterZeroYValWaveFreq(1.0 / (1.10 - accelVar));

    const auto [modColor, modColorLow] = getModColors(goomInfo);
    cycle += 0.01f;

    if (doDraw)
    {
      stats.updateWithDraw();
    }

    driver.update(doDraw, m_half_pi - rot, distt, distt2, modColor, modColorLow, frontBuff,
                  backBuff);
  }
  else
  {
    lig = 1.05f;
    if (ligs < 0.0f)
    {
      ligs = -ligs;
    }

    logDebug("Starting pretty_move 2.");
    stats.updateWithPrettyMove2();
    prettyMove(goomInfo);

    cycle += 0.1f;
    if (cycle > 1000)
    {
      stats.cycleReset();
      cycle = 0;
    }
  }
}

void TentaclesWrapper::happensUpdate(PluginInfo* goomInfo)
{
  if (happens)
  {
    happens -= 1;
  }
  else if (lock == 0)
  {
    stats.happensReset();
    happens = goomInfo->getNRand(200) ? 0 : static_cast<int>(100 + goomInfo->getNRand(60));
    lock = happens * 3 / 2;
  }
  else
  {
    lock--;
  }
}

void TentaclesWrapper::prettyMove(PluginInfo* goomInfo)
{
  /* many magic numbers here... I don't really like that. */
  happensUpdate(goomInfo);

  const float distt2Offset = happens ? 8.0 : 0.0;
  distt2 = std::lerp(distt2, distt2Offset, prettyMoveLerpMix);

  float currentCycle = cycle;

  float disttOffset = 286.0 - 90.0 * (1.0 + sin(currentCycle * 19.0 / 20.0));
  if (happens)
  {
    disttOffset *= 0.6f;
  }
  distt = std::lerp(distt, disttOffset, 4.0f * prettyMoveLerpMix);

  float rotOffset = 0;
  if (!happens)
  {
    rotOffset = (1.5 + sin(currentCycle) / 32.0) * m_pi;
  }
  else
  {
    if (probabilityOfMInN(goomInfo, 1, 500))
    {
      doRotation = probabilityOfMInN(goomInfo, 1, 2);
    }
    if (doRotation)
    {
      currentCycle *= m_two_pi;
    }
    else
    {
      currentCycle *= -m_pi;
    }
    rotOffset = m_two_pi * getFractPart(currentCycle / m_two_pi);
  }
  if (std::fabs(rot - rotOffset) > std::fabs(rot + m_two_pi - rotOffset))
  {
    rot = std::lerp(rot + m_two_pi, rotOffset, prettyMoveLerpMix);
    if (rot > m_two_pi)
    {
      rot -= m_two_pi;
    }
  }
  else if (std::fabs(rot - rotOffset) > std::fabs(rot - m_two_pi - rotOffset))
  {
    rot = std::lerp(rot - m_two_pi, rotOffset, prettyMoveLerpMix);
    if (rot < 0.0f)
    {
      rot += m_two_pi;
    }
  }
  else
  {
    rot = std::lerp(rot, rotOffset, prettyMoveLerpMix);
  }
}

inline std::tuple<uint32_t, uint32_t> TentaclesWrapper::getModColors(PluginInfo* goomInfo)
{
  if (happens)
  {
    stats.changeColorMap();
    dominantColorGroup = &colorMaps.getRandomColorMap();
  }

  if ((lig > 10.0f) || (lig < 1.1f))
  {
    ligs = -ligs;
  }

  if ((lig < 6.3f) && probabilityOfMInN(goomInfo, 1, 5))
  {
    stats.changeTentacleColor();
    color = ColorMap::getRandomColor(*dominantColorGroup);
  }

  color = getEvolvedColor(color);

  const uint32_t modColor = getLightenedColor(color, lig * 2.0f + 2.0f);
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
  fx.save = NULL;
  fx.restore = NULL;
  return fx;
}

void tentacle_free(TentacleFXData* data)
{
  delete data->tentacles;
}
