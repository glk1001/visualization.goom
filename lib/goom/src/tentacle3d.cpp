#include "tentacle3d.h"

#include "goom.h"
#include "goom_config.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "surf3d.h"
#include "tentacle_driver.h"
#include "tentacles_new.h"
#include "v3d.h"
//#include "SimplexNoise.h"

#include "goomutils/colormap.h"
#include "goomutils/logging.h"
#include "goomutils/math_utils.h"

#include <cstdint>
#include <cmath>
#include <tuple>
#include <vector>

class TentaclesWrapper {
public:
  explicit TentaclesWrapper(const int screenWidth, const int screenHeight);
  void update(PluginInfo* goomInfo, Pixel* buf, Pixel* back,
              gint16 data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN],
              float accelvar, int drawit, TentacleFXData* fx_data);
private:
  WeightedColorMaps colorMaps;
  const ColorMap *dominantColorGroup;
  TentacleDriver driver;
  void pretty_move(PluginInfo* goomInfo, float cycle, float* dist, float* dist2,
                   float* rotangle, TentacleFXData* fx_data);
  std::tuple<uint32_t, uint32_t> getModColors(PluginInfo* goomInfo, TentacleFXData* fx_data);
  static uint32_t getEvolvedColor(const uint32_t baseColor);
  static uint32_t evolvecolor(uint32_t src, uint32_t dest, const uint32_t mask, const uint32_t incr);
  static uint32_t lightencolor(const uint32_t oldColor, const float power);
  static unsigned char lighten(const unsigned char value, const float power);
  std::vector<float> getGridZeroAdditiveValues(PluginInfo* goomInfo, const float rapport);
};

TentaclesWrapper::TentaclesWrapper(const int screenWidth, const int screenHeight)
  : colorMaps{
      Weights<ColorMapGroup>{ {
        { ColorMapGroup::perceptuallyUniformSequential, 10 },
        { ColorMapGroup::sequential, 5 },
        { ColorMapGroup::sequential2, 5 },
        { ColorMapGroup::cyclic, 10 },
        { ColorMapGroup::diverging, 20 },
        { ColorMapGroup::diverging_black, 20 },
        { ColorMapGroup::qualitative, 10 },
        { ColorMapGroup::misc, 20 },
      } }
    }
  , dominantColorGroup{ &colorMaps.getRandomColorMap() }
  , driver{ &colorMaps, screenWidth, screenHeight }
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

constexpr float D = 256.0f;

inline float randFactor(PluginInfo* goomInfo, const float min)
{
  return min + (1.0 - min)*float(goom_irand(goomInfo->gRandom, 101))/100.0;
}

inline int get_rand_in_range(int n1, int n2)
{
  const uint32_t range_len = (uint32_t)(n2 - n1 + 1);
  return n1 + (int)(pcg32_rand() % range_len);
}


/**
void updateColors(PluginInfo* goomInfo, const TentacleFXData* fx_data)
{
  if (!fx_data->happens) {
    if (goom_irand(goomInfo->gRandom, 20) == 0) {
      TentacleLineColorer::changeColorGroup(goomInfo);
    }
  } else {
    TentacleLineColorer::changeColorGroup(goomInfo);
    if (goom_irand(goomInfo->gRandom, 50) == 0) {
      lineColorer->changeReverseMix();
    }
  }
}
**/

void TentaclesWrapper::update(PluginInfo* goomInfo, Pixel* buf, Pixel* back,
                              gint16 data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN],
                              float accelvar, int drawit, TentacleFXData* fx_data)
{
  logDebug("Starting update.");

  const float modAccelvar = accelvar;

  if ((!drawit) && (fx_data->ligs > 0.0f)) {
    fx_data->ligs = -fx_data->ligs;
  }
  fx_data->lig += fx_data->ligs;

  if (fx_data->lig > 1.01f) {
    logDebug("Starting pretty_move 1.");
    float dist, dist2, rotangle;
    pretty_move(goomInfo, fx_data->cycle, &dist, &dist2, &rotangle, fx_data);

    const auto [modColor, modColorLow] = getModColors(goomInfo, fx_data);

    if (modAccelvar < 0.7) {
      driver.setGlitchValues(0.0, 0.0);
      driver.setReverseColorMix(false);
    } else {
      const float glitchLength = modAccelvar*getRandInRange(0.3f, 2.0f);
      driver.setGlitchValues(-0.5*glitchLength, +0.5*glitchLength);
// Reversing seems too jarring
//      driver.setReverseColorMix(true);
    }

    const float rapport = getRapport(modAccelvar);
    //const float nx_div2 = 0.5 * float(num_x);
    // NOTE: Putting vals outside loop gives same variation per grid.
//    const std::vector<float> vals = getGridZeroAdditiveValues(goomInfo, rapport);

    driver.multiplyIterZeroYValWaveFreq(1.0/(1.20 - modAccelvar));

    fx_data->cycle += 0.01f;
//    updateColors(goomInfo, fx_data);

    driver.update(0.5*M_PI - rotangle, dist, dist2, modColor, modColorLow, buf, back);

  } else {
    fx_data->lig = 1.05f;
    if (fx_data->ligs < 0.0f) {
      fx_data->ligs = -fx_data->ligs;
    }

    logDebug("Starting pretty_move 2.");
    float dist, dist2, rotangle;
    pretty_move(goomInfo, fx_data->cycle, &dist, &dist2, &rotangle, fx_data);

    fx_data->cycle += 0.1f;
    if (fx_data->cycle > 1000) {
      fx_data->cycle = 0;
    }
  }
}

void TentaclesWrapper::pretty_move(PluginInfo* goomInfo, float cycle, float* dist, float* dist2,
                                   float* rotangle, TentacleFXData* fx_data)
{
  /* many magic numbers here... I don't really like that. */
  if (fx_data->happens) {
    fx_data->happens -= 1;
  } else if (fx_data->lock == 0) {
    fx_data->happens =
        goom_irand(goomInfo->gRandom, 200) ? 0 : (int)(100 + goom_irand(goomInfo->gRandom, 60));
    fx_data->lock = fx_data->happens * 3 / 2;
  } else {
    fx_data->lock--;
  }

  float tmp = fx_data->happens ? 8.0f : 0;
  *dist2 = fx_data->distt2 = (tmp + 15.0f * fx_data->distt2) / 16.0f;

  tmp = 30 + D - 90.0f * (1.0f + sin(cycle * 19 / 20));
  if (fx_data->happens)
    tmp *= 0.6f;

  *dist = fx_data->distt = (tmp + 3.0f * fx_data->distt) / 4.0f;

  if (!fx_data->happens) {
    tmp = M_PI * sin(cycle) / 32 + 3 * M_PI / 2;
  } else {
    fx_data->rotation = goom_irand(goomInfo->gRandom, 500) ? fx_data->rotation
                                                           : (int)goom_irand(goomInfo->gRandom, 2);
    if (fx_data->rotation) {
      cycle *= 2.0f * M_PI;
    } else {
      cycle *= -1.0f * M_PI;
    }
    tmp = cycle - (M_PI * 2.0) * floor(cycle / (M_PI * 2.0));
  }

  if (fabs(tmp - fx_data->rot) > fabs(tmp - (fx_data->rot + 2.0 * M_PI))) {
    fx_data->rot = (tmp + 15.0f * (fx_data->rot + 2 * M_PI)) / 16.0f;
    if (fx_data->rot > 2.0 * M_PI) {
      fx_data->rot -= 2.0 * M_PI;
    }
    *rotangle = fx_data->rot;
  } else if (fabs(tmp - fx_data->rot) > fabs(tmp - (fx_data->rot - 2.0 * M_PI))) {
    fx_data->rot = (tmp + 15.0f * (fx_data->rot - 2.0 * M_PI)) / 16.0f;
    if (fx_data->rot < 0.0f)
      fx_data->rot += 2.0 * M_PI;
    *rotangle = fx_data->rot;
  } else {
    *rotangle = fx_data->rot = (tmp + 15.0f * fx_data->rot) / 16.0f;
  }
}

/**
std::vector<float> TentaclesWrapper::getGridZeroAdditiveValues(PluginInfo* goomInfo, const float rapport)
{
  const float val = 1.7*(goom_irand(goomInfo->gRandom, 101)/100.0) * rapport;
//      const float val =
//          (float)(ShiftRight(data[0][goom_irand(goomInfo->gRandom, AUDIO_SAMPLE_LEN - 1)], 10)) * rapport;
  std::vector<float> vals(num_x);
  for (size_t x = 0; x < num_x; x++) {
//        const float val =
//            (float)(ShiftRight(data[0][goom_irand(goomInfo->gRandom, AUDIO_SAMPLE_LEN - 1)], 10)) * rapport;
//        const float factor = 0.9  + 0.1*(std::abs(nx_div2 - float(x))/nx_div2);
//        fx_data->vals[x] = val * factor * randFactor(goomInfo, 0.97);
    vals[x] = val * randFactor(goomInfo, 0.97);
  }
  return vals;
}
***/

inline std::tuple<uint32_t, uint32_t> TentaclesWrapper::getModColors(PluginInfo* goomInfo, TentacleFXData* fx_data)
{
  if (fx_data->happens) {
    dominantColorGroup = &colorMaps.getRandomColorMap();
  }

  if ((fx_data->lig > 10.0f) || (fx_data->lig < 1.1f)) {
    fx_data->ligs = -fx_data->ligs;
  }

  if ((fx_data->lig < 6.3f) && (goom_irand(goomInfo->gRandom, 30) == 0)) {
//    fx_data->dstcol = (int)goom_irand(goomInfo->gRandom, NUM_TENTACLE_COLORS);
//    fx_data->dstcol = (int)goom_irand(goomInfo->gRandom, modColorGroup->numColors()); // TODO Make numColors a constexpr
    fx_data->col = ColorMap::getRandomColor(*dominantColorGroup);
  }

  fx_data->col = getEvolvedColor(fx_data->col);

  const uint32_t color = lightencolor(fx_data->col, fx_data->lig * 2.0f + 2.0f);
//  const uint32_t colorLow = lightencolor(color, (fx_data->lig / 3.0f) + 0.67f);
  const uint32_t colorLow = lightencolor(color, (fx_data->lig / 2.0f) + 0.67f);

  return std::make_tuple(color, colorLow);
}

inline uint32_t TentaclesWrapper::getEvolvedColor(const uint32_t baseColor)
{
  uint32_t newColor = baseColor;

  newColor = evolvecolor(newColor, baseColor, 0xff, 0x01);
  newColor = evolvecolor(newColor, baseColor, 0xff00, 0x0100);
  newColor = evolvecolor(newColor, baseColor, 0xff0000, 0x010000);
  newColor = evolvecolor(newColor, baseColor, 0xff000000, 0x01000000);

  newColor = lightencolor(newColor, 10.0 * 2.0f + 2.0f);

  return newColor;
}

inline uint32_t TentaclesWrapper::evolvecolor(uint32_t src, uint32_t dest, const uint32_t mask, const uint32_t incr)
{
  const uint32_t color = src & (~mask);
  src &= mask;
  dest &= mask;

  if ((src != mask) && (src < dest)) {
    src += incr;
  }

  if (src > dest) {
    src -= incr;
  }
  return uint32_t((src & mask) | color);
}

inline uint32_t TentaclesWrapper::lightencolor(const uint32_t oldColor, const float power)
{
  Pixel pixel{ .val = oldColor };

  pixel.channels.r = lighten(pixel.channels.r, power);
  pixel.channels.g = lighten(pixel.channels.g, power);
  pixel.channels.b = lighten(pixel.channels.b, power);

  return pixel.val;
}

inline unsigned char TentaclesWrapper::lighten(const unsigned char value, const float power)
{
  int val = value;

  const float t = float(val * std::log10(power) / 2.0);
  if (t <= 0.0) {
    return 0;
  }

  val = int(t); /* (32.0f * log (t)); */
  if (val > 255) {
    val = 255;
  } else if (val < 0) {
    val = 0;
  }
  return val;
}


/* 
 * VisualFX wrapper for the tentacles
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

void tentacle_fx_init(VisualFX* _this, PluginInfo* info)
{
  TentacleFXData* data = (TentacleFXData*)malloc(sizeof(TentacleFXData));

  data->tentacles = new TentaclesWrapper{ info->screen.width, info->screen.height };

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
  _this->fx_data = (void*)data;
}

#pragma GCC diagnostic pop

void tentacle_fx_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* goomInfo)
{
  TentacleFXData* data = (TentacleFXData*)_this->fx_data;
  if (BVAL(data->enabled_bp)) {
    data->tentacles->update(goomInfo, dest, src, goomInfo->sound.samples,
                            goomInfo->sound.accelvar, goomInfo->curGState->drawTentacle, data);
  }
}

void tentacle_fx_free(VisualFX* _this)
{
  TentacleFXData* data = (TentacleFXData*)_this->fx_data;
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
