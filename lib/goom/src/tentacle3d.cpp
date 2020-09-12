#include "tentacle3d.h"

#include "colorutils.h"
#include "goom_config.h"
#include "goom_core.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "goomutils/colormap.h"
#include "goomutils/logging_control.h"
#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "tentacle_driver.h"
#include "tentacles_new.h"
#include "v3d.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

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
  void resetCycle();
  void resetHappens();
private:
  uint32_t numColorMapChanges = 0;
  uint32_t numTentacleColorChanges = 0;
  uint32_t numUpdatesWithDraw = 0;
  uint32_t numUpdatesWithPrettyMove1 = 0;
  uint32_t numUpdatesWithPrettyMove2 = 0;
  uint32_t numLowToMediumAcceleration = 0;
  uint32_t numHighAcceleration = 0;
  uint32_t numResetCycle = 0;
  uint32_t numResetHappens = 0;
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
  logVal(module, "numResetCycle", numResetCycle);
  logVal(module, "numResetHappens", numResetHappens);
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
  numResetCycle = 0;
  numResetHappens = 0;
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

inline void TentacleStats::resetCycle()
{
  numResetCycle++;
}

inline void TentacleStats::resetHappens()
{
  numResetHappens++;
}

class TentaclesWrapper
{
public:
  explicit TentaclesWrapper(const int screenWidth, const int screenHeight);
  void update(PluginInfo*,
              Pixel* frontBuff,
              Pixel* backBuff,
              const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN],
              const float accelVar,
              const bool doDraw,
              TentacleFXData*);
  void logStats(const StatsLogValueFunc logVal);

private:
  WeightedColorMaps colorMaps;
  const ColorMap* dominantColorGroup;
  TentacleDriver driver;
  TentacleStats stats;
  struct PrettyMoveValues
  {
    float dist;
    float dist2;
    float rotangle;
  };
  PrettyMoveValues prettyMove(PluginInfo*, float cycle, TentacleFXData*);
  std::tuple<uint32_t, uint32_t> getModColors(PluginInfo*, TentacleFXData*);
  std::vector<float> getGridZeroAdditiveValues(PluginInfo*, const float rapport);
};

TentaclesWrapper::TentaclesWrapper(const int screenWidth, const int screenHeight)
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

void TentaclesWrapper::logStats(const StatsLogValueFunc logVal)
{
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
                              const bool doDraw,
                              TentacleFXData* fx_data)
{
  logDebug("Starting update.");

  if (!doDraw && (fx_data->ligs > 0.0f))
  {
    fx_data->ligs = -fx_data->ligs;
  }
  fx_data->lig += fx_data->ligs;

  if (fx_data->lig > 1.01f)
  {
    logDebug("Starting pretty_move 1.");
    stats.updateWithPrettyMove1();
    const PrettyMoveValues movevalues = prettyMove(goomInfo, fx_data->cycle, fx_data);

    if (accelVar < 0.7)
    {
      stats.lowToMediumAcceleration();
      driver.setGlitchValues(0.0, 0.0);
      driver.setReverseColorMix(false);
    }
    else
    {
      stats.highAcceleration();
      const float glitchLength = accelVar * getRandInRange(0.3f, 2.0f);
      driver.setGlitchValues(-0.5 * glitchLength, +0.5 * glitchLength);
      // Reversing seems too jarring
      //      driver.setReverseColorMix(true);
      stats.changeTentacleColor();
      fx_data->col = ColorMap::getRandomColor(*dominantColorGroup);
    }

    const auto [modColor, modColorLow] = getModColors(goomInfo, fx_data);

    // Higher sound acceleration increases tentacle wave frequency.
    driver.multiplyIterZeroYValWaveFreq(1.0 / (1.10 - accelVar));

    fx_data->cycle += 0.01f;
    //    updateColors(goomInfo, fx_data);

    if (doDraw)
    {
      stats.updateWithDraw();
    }

    driver.update(doDraw, 0.5 * M_PI - movevalues.rotangle, movevalues.dist, movevalues.dist2,
                  modColor, modColorLow, frontBuff, backBuff);
  }
  else
  {
    fx_data->lig = 1.05f;
    if (fx_data->ligs < 0.0f)
    {
      fx_data->ligs = -fx_data->ligs;
    }

    logDebug("Starting pretty_move 2.");
    stats.updateWithPrettyMove2();
    prettyMove(goomInfo, fx_data->cycle, fx_data);

    fx_data->cycle += 0.1f;
    if (fx_data->cycle > 1000)
    {
      stats.resetCycle();
      fx_data->cycle = 0;
    }
  }
}

TentaclesWrapper::PrettyMoveValues TentaclesWrapper::prettyMove(PluginInfo* goomInfo,
                                                                float cycle,
                                                                TentacleFXData* fx_data)
{
  constexpr float D = 256.0f;

  /* many magic numbers here... I don't really like that. */
  if (fx_data->happens)
  {
    fx_data->happens -= 1;
  }
  else if (fx_data->lock == 0)
  {
    stats.resetHappens();
    fx_data->happens = goomInfo->getNRand(200) ? 0 : static_cast<int>(100 + goomInfo->getNRand(60));
    fx_data->lock = fx_data->happens * 3 / 2;
  }
  else
  {
    fx_data->lock--;
  }

  PrettyMoveValues moveValues{};

  float tmp = fx_data->happens ? 8.0f : 0;
  moveValues.dist2 = fx_data->distt2 = (tmp + 15.0f * fx_data->distt2) / 16.0f;

  tmp = 30 + D - 90.0f * (1.0f + sin(cycle * 19 / 20));
  if (fx_data->happens)
  {
    tmp *= 0.6f;
  }

  moveValues.dist = fx_data->distt = (tmp + 3.0f * fx_data->distt) / 4.0f;

  if (!fx_data->happens)
  {
    tmp = M_PI * sin(cycle) / 32 + 3 * M_PI / 2;
  }
  else
  {
    fx_data->rotation =
        goomInfo->getNRand(500) ? fx_data->rotation : static_cast<int>(goomInfo->getNRand(2));
    if (fx_data->rotation)
    {
      cycle *= 2.0f * M_PI;
    }
    else
    {
      cycle *= -1.0f * M_PI;
    }
    tmp = cycle - (M_PI * 2.0) * floor(cycle / (M_PI * 2.0));
  }

  if (fabs(tmp - fx_data->rot) > fabs(tmp - (fx_data->rot + 2.0 * M_PI)))
  {
    fx_data->rot = (tmp + 15.0f * (fx_data->rot + 2 * M_PI)) / 16.0f;
    if (fx_data->rot > 2.0 * M_PI)
    {
      fx_data->rot -= 2.0 * M_PI;
    }
    moveValues.rotangle = fx_data->rot;
  }
  else if (fabs(tmp - fx_data->rot) > fabs(tmp - (fx_data->rot - 2.0 * M_PI)))
  {
    fx_data->rot = (tmp + 15.0f * (fx_data->rot - 2.0 * M_PI)) / 16.0f;
    if (fx_data->rot < 0.0f)
    {
      fx_data->rot += 2.0 * M_PI;
    }
    moveValues.rotangle = fx_data->rot;
  }
  else
  {
    moveValues.rotangle = fx_data->rot = (tmp + 15.0f * fx_data->rot) / 16.0f;
  }

  return moveValues;
}

inline std::tuple<uint32_t, uint32_t> TentaclesWrapper::getModColors(PluginInfo* goomInfo,
                                                                     TentacleFXData* fx_data)
{
  if (fx_data->happens)
  {
    stats.changeColorMap();
    dominantColorGroup = &colorMaps.getRandomColorMap();
  }

  if ((fx_data->lig > 10.0f) || (fx_data->lig < 1.1f))
  {
    fx_data->ligs = -fx_data->ligs;
  }

  if ((fx_data->lig < 6.3f) && probabilityOfMInN(goomInfo, 1, 5))
  {
    stats.changeTentacleColor();
    fx_data->col = ColorMap::getRandomColor(*dominantColorGroup);
  }

  fx_data->col = getEvolvedColor(fx_data->col);

  const uint32_t color = getLightenedColor(fx_data->col, fx_data->lig * 2.0f + 2.0f);
  //  const uint32_t colorLow = getLightenedColor(color, (fx_data->lig / 3.0f) + 0.67f);
  const uint32_t colorLow = getLightenedColor(color, (fx_data->lig / 2.0f) + 0.67f);

  return std::make_tuple(color, colorLow);
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

  data->cycle = 0.0f;
  data->col = (0x28 << (ROUGE * 8)) | (0x2c << (VERT * 8)) | (0x5f << (BLEU * 8));
  data->lig = 1.15f;
  data->ligs = 0.1f;

  data->distt = 10.0f;
  data->distt2 = 0.0f;
  data->rot = 0.0f; /* entre 0 et 2 * M_PI */
  data->happens = 0;

  data->rotation = 0;
  data->lock = 0;
  // data->colors;

  _this->params = &data->params;
  _this->fx_data = data;
}

void tentacle_fx_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* goomInfo)
{
  TentacleFXData* data = static_cast<TentacleFXData*>(_this->fx_data);
  if (BVAL(data->enabled_bp))
  {
    data->tentacles->update(goomInfo, dest, src, goomInfo->sound.samples, goomInfo->sound.accelvar,
                            goomInfo->curGDrawables.count(GoomDrawable::tentacles), data);
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
