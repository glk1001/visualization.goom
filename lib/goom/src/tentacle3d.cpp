#include "tentacle3d.h"

#include "goom.h"
#include "goom_config.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "surf3d.h"
#include "v3d.h"
//#include "SimplexNoise.h"

#include <vivid/vivid.h>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <memory>
#include <tuple>
#include <vector>


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


class ColorGroup {
public:
  explicit ColorGroup(vivid::ColorMap::Preset preset, size_t n = num_z);
  ColorGroup(const ColorGroup& other);
  ColorGroup& operator=(const ColorGroup& other) { colors = other.colors; return *this; }
  size_t numColors() const { return colors.size(); }
  uint32_t getColor(size_t i) const { return colors[i]; }
    static const ColorGroup& getRandomColorGroup(PluginInfo *goomInfo);
private:
  ColorGroup();
  std::vector<uint32_t> colors;
  static std::unique_ptr<std::vector<ColorGroup>> colorGroups;
  static std::vector<ColorGroup> getColorGroups();
};


class SineWave {
public:
  explicit SineWave(const float sinFreq);
  float getNext();
  float getSinFrequency() const { return sinFrequency; }
  void setSinFrequency(const float val) { sinFrequency = val; }
  void setAmplitude(const float val) { amplitude = val; }
  void setPiStepFrac(const float val) { piStepFrac = val; }
private:
  float sinFrequency;
  float amplitude;
  float piStepFrac;
  float x;
};


class ChangeTracker {
public:
  explicit ChangeTracker(size_t sigInRow=10);
  void nextVal(float val);
  bool significantIncreaseInARow();
  bool significantDecreaseInARow();
private:
  static constexpr float sigDiff = 0.01;
  const size_t significantInRow;
  size_t numInRowIncreasing;
  size_t numInRowDecreasing;
  float currentVal;
};


class TentacleLineColorer: public LineColorer {
public:
  virtual void resetColorNum() { colNum = 0; }
  std::tuple<uint32_t, uint32_t> getColorMix(const size_t nx, const size_t nz,
                                             const uint32_t color, const uint32_t colorLow);
  void changeReverseMix() { reverseMixMode = not reverseMixMode; };
  void setNumZ(const size_t numz) { num_z = numz; }
  static void changeColorGroup(PluginInfo* goomInfo);
private:
  static ColorGroup colorGroup;
  size_t colNum = 0;
  bool reverseMixMode = false;
  size_t num_z = 0;
  uint32_t colorMix(const uint32_t col1, const uint32_t col2, const float t) const;
};


Tentacles::Tentacles(PluginInfo* goomInfo)
  : width(goomInfo->screen.width)
  , height(goomInfo->screen.height)
  , grids{ }
  , sineWave{ new SineWave{ 5.0 } }
  , modColorGroup{ new ColorGroup{ ColorGroup::getRandomColorGroup(goomInfo) } }
  , lineColorer{ new TentacleLineColorer }
  , accelVarTracker{ new ChangeTracker{ 10 } }
{
  // Initialize the static to a better value.
  TentacleLineColorer::changeColorGroup(goomInfo);
}

static void pretty_move(PluginInfo* goomInfo, float cycle, float* dist, float* dist2,
                        float* rotangle, TentacleFXData* fx_data);
static int evolvecolor(uint32_t src, uint32_t dest, unsigned int mask, unsigned int incr);
static void lightencolor(uint32_t* col, float power);

void Tentacles::init()
{
  // Start at bottom of grid, going up by 'y_step'.
  const float y_step = y_height / (float) (nbgrid - 1) + get_rand_in_range(-y_step_mod / 2, y_step_mod / 2);
  v3d center = {0, 0, 0};

  grids.resize(nbgrid);
  float y = y_start;
  for (size_t i = 0; i < nbgrid; i++) {
    const size_t nx  = num_x + (size_t)get_rand_in_range(-num_x_mod, 0);
    const int xsize = x_width + get_rand_in_range(0, x_width_mod);

    const size_t nz = num_z + (size_t)get_rand_in_range(-num_z_mod, 0);
    const int zsize = z_depth + get_rand_in_range(-z_depth_mod/2, z_depth_mod/2);
    const float zmin = 3.1;
    const float zmax_min = 0.8*(float(zsize) - zmin);
    const float zmax = float(zsize) - zmax_min;
    float zdepth_mins[nx];
    float zdepth_maxs[nx];
    const float nx_div2 = 0.5 * float(nx);
    for (size_t x=0; x < nx; x++) {
      zdepth_mins[x] = zmin;
      zdepth_maxs[x] = zmin + zmax_min + zmax*(1.0 - std::abs(nx_div2 - float(x))/nx_div2);
    }

    center.y = y + get_rand_in_range(-y_step_mod / 2, y_step_mod / 2);
    center.z = zdepth_maxs[nbgrid-1];

    GOOM_LOG_INFO("Creating grid %d.", i);
    grids[i] = std::unique_ptr<Grid>{ new Grid(center, xsize, xsize, nx, zdepth_mins, zdepth_maxs, nz) };

    y += y_step;
  }
}

void Tentacles::update(PluginInfo* goomInfo, Pixel* buf, Pixel* back,
                       gint16 data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN], float accelvar,
                       int drawit, TentacleFXData* fx_data)
{
  GOOM_LOG_INFO("Starting Tentacles::update.");

  const float modAccelvar = sineWave->getNext();
  updateSineWaveParams(accelvar);
//  const float modAccelvar = accelvar;

  if ((!drawit) && (fx_data->ligs > 0.0f)) {
    fx_data->ligs = -fx_data->ligs;
  }
  fx_data->lig += fx_data->ligs;

  if (fx_data->lig > 1.01f) {
    GOOM_LOG_INFO("Starting pretty_move 1.");
    float dist, dist2, rotangle;
    pretty_move(goomInfo, fx_data->cycle, &dist, &dist2, &rotangle, fx_data);

    const auto modColors = getModColors(goomInfo, fx_data);
    const uint32_t modColor = std::get<0>(modColors);
    const uint32_t modColorLow = std::get<1>(modColors);

    const float rapport = getRapport(modAccelvar);
    //const float nx_div2 = 0.5 * float(num_x);
    // NOTE: Putting vals outside loop gives same variation per grid.
    const std::vector<float> vals = getGridZeroAdditiveValues(goomInfo, rapport);

    for (size_t i = 0; i < nbgrid; i++) {
      // Note: Following did not originally have '0.5*M_PI -' but with 'grid3d_update'
      //   bug-fix this is the counter-fix.
      grids[i]->update(0.5*M_PI - rotangle, vals, dist2);
    }

    fx_data->cycle += 0.01f;
    updateColors(goomInfo, fx_data);

    for (size_t i = 0; i < nbgrid; i++) {
      grids[i]->drawToBuffs(buf, back, width, height, *lineColorer, dist, modColor, modColorLow);
    }
  } else {
    fx_data->lig = 1.05f;
    if (fx_data->ligs < 0.0f) {
      fx_data->ligs = -fx_data->ligs;
    }

    GOOM_LOG_INFO("Starting pretty_move 2.");
    float dist, dist2, rotangle;
    pretty_move(goomInfo, fx_data->cycle, &dist, &dist2, &rotangle, fx_data);

    fx_data->cycle += 0.1f;
    if (fx_data->cycle > 1000) {
      fx_data->cycle = 0;
    }
  }
}

inline std::tuple<uint32_t, uint32_t> Tentacles::getModColors(PluginInfo* goomInfo, TentacleFXData* fx_data)
{
  if (fx_data->happens) {
//    if (goom_irand(goomInfo->gRandom, 100) == 0) {
    *modColorGroup = ColorGroup::getRandomColorGroup(goomInfo);
  }

  if ((fx_data->lig > 10.0f) || (fx_data->lig < 1.1f)) {
    fx_data->ligs = -fx_data->ligs;
  }

  if ((fx_data->lig < 6.3f) && (goom_irand(goomInfo->gRandom, 30) == 0)) {
//    fx_data->dstcol = (int)goom_irand(goomInfo->gRandom, NUM_TENTACLE_COLORS);
    fx_data->dstcol = (int)goom_irand(goomInfo->gRandom, modColorGroup->numColors()); // TODO Make numColors a constexpr
  }

  /**
  fx_data->col = evolvecolor((uint32_t)fx_data->col, (uint32_t)fx_data->colors[fx_data->dstcol], 0xff, 0x01);
  fx_data->col = evolvecolor((uint32_t)fx_data->col, (uint32_t)fx_data->colors[fx_data->dstcol], 0xff00, 0x0100);
  fx_data->col = evolvecolor((uint32_t)fx_data->col, (uint32_t)fx_data->colors[fx_data->dstcol], 0xff0000, 0x010000);
  fx_data->col = evolvecolor((uint32_t)fx_data->col, (uint32_t)fx_data->colors[fx_data->dstcol], 0xff000000, 0x01000000);
  **/
  const uint32_t baseColor = modColorGroup->getColor(size_t(fx_data->dstcol));
  fx_data->col = evolvecolor((uint32_t)fx_data->col, baseColor, 0xff, 0x01);
  fx_data->col = evolvecolor((uint32_t)fx_data->col, baseColor, 0xff00, 0x0100);
  fx_data->col = evolvecolor((uint32_t)fx_data->col, baseColor, 0xff0000, 0x010000);
  fx_data->col = evolvecolor((uint32_t)fx_data->col, baseColor, 0xff000000, 0x01000000);

  uint32_t color = uint32_t(fx_data->col);
  lightencolor(&color, fx_data->lig * 2.0f + 2.0f);

  uint32_t colorLow = color;
  lightencolor(&colorLow, (fx_data->lig / 3.0f) + 0.67f);

  return std::make_tuple(color, colorLow);
}

std::vector<float> Tentacles::getGridZeroAdditiveValues(PluginInfo* goomInfo, float rapport)
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

void Tentacles::updateColors(PluginInfo* goomInfo, const TentacleFXData* fx_data)
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

void Tentacles::updateSineWaveParams(float accelvar)
{
  accelVarTracker->nextVal(accelvar);
  if (accelVarTracker->significantIncreaseInARow()) {
    sineWave->setSinFrequency(std::min(10.0f, sineWave->getSinFrequency() + 1.0f));
  } else if (accelVarTracker->significantDecreaseInARow()) {
    sineWave->setSinFrequency(std::max(0.5f, sineWave->getSinFrequency() - 1.0f));
  }
}

SineWave::SineWave(const float sinFreq)
  : sinFrequency(sinFreq)
  , amplitude(1)
  , piStepFrac(1.0/16.0)
  , x(0)
{
}

float SineWave::getNext()
{
  const float modAccelVar = amplitude*0.5*(1.0 + sin(sinFrequency*x));
  x += piStepFrac*M_PI;
  return modAccelVar;
}


ChangeTracker::ChangeTracker(size_t sigInRow)
  : significantInRow { sigInRow }
  , numInRowIncreasing { 0 }
  , numInRowDecreasing { 0 }
  , currentVal { 0 }
{
}

void ChangeTracker::nextVal(float val)
{
  if (val > (currentVal+sigDiff)) {
    numInRowIncreasing++;
    numInRowDecreasing = 0;
  } else if (val < (currentVal-sigDiff)) {
    numInRowDecreasing++;
    numInRowIncreasing = 0;
  }
}

bool ChangeTracker::significantIncreaseInARow()
{
  if (numInRowIncreasing >= significantInRow) {
    numInRowIncreasing = 0;
    return true;
  }
  return false;
}

bool ChangeTracker::significantDecreaseInARow()
{
  if (numInRowDecreasing >= significantInRow) {
    numInRowDecreasing = 0;
    return true;
  }
  return false;
}


inline ColorGroup::ColorGroup()
: colors()
{
}

inline ColorGroup::ColorGroup(vivid::ColorMap::Preset preset, size_t n)
: colors(n)
{
  const vivid::ColorMap cmap(preset);
  for (size_t i=0; i < n; i++) {
    const float t = float(i) / (n - 1.0f);
    const vivid::rgb_t col{ cmap.at(t) };
    const vivid::rgb_t brightCol { 1.0f*col[0], 1.0f*col[1], 1.0f*col[2] };
    colors[i] = vivid::Color(brightCol).rgb32();
  }
}

inline ColorGroup::ColorGroup(const ColorGroup& other)
: colors(other.colors)
{
}

// TODO Have contrasting color groups - one for segment color, one for mod color.

std::unique_ptr<std::vector<ColorGroup>> ColorGroup::colorGroups;

std::vector<ColorGroup> ColorGroup::getColorGroups()
{
  const std::vector<vivid::ColorMap::Preset> cmaps = {
    vivid::ColorMap::Preset::BlueYellow,
    vivid::ColorMap::Preset::CoolWarm,
    vivid::ColorMap::Preset::Hsl,
    vivid::ColorMap::Preset::HslPastel,
    vivid::ColorMap::Preset::Inferno,
    vivid::ColorMap::Preset::Magma,
    vivid::ColorMap::Preset::Plasma,
    vivid::ColorMap::Preset::Rainbow,
    vivid::ColorMap::Preset::Turbo,
    vivid::ColorMap::Preset::Viridis,
    vivid::ColorMap::Preset::Vivid
  };
  std::vector<ColorGroup> colGroups;
  for (auto cm : cmaps) {
    colGroups.push_back(ColorGroup(cm));
  }
  return colGroups;
}

inline const ColorGroup& ColorGroup::getRandomColorGroup(PluginInfo *goomInfo)
{
  if (!colorGroups) {
    colorGroups = std::unique_ptr<std::vector<ColorGroup>>{ new std::vector<ColorGroup>{ getColorGroups() }};
  }
  return (*colorGroups)[goom_irand(goomInfo->gRandom, ColorGroup::colorGroups->size())];
}


ColorGroup TentacleLineColorer::colorGroup { vivid::ColorMap::Preset::BlueYellow, 2 };

inline void TentacleLineColorer::changeColorGroup(PluginInfo* goomInfo)
{
  colorGroup = ColorGroup::getRandomColorGroup(goomInfo);
}

std::tuple<uint32_t, uint32_t> TentacleLineColorer::getColorMix(
    const size_t nx, const size_t nz, const uint32_t color, const uint32_t colorLow)
{
  const auto tFac = [this](const size_t nz) -> float {
    return float(nz+1)/float(num_z);
  };
  
  const float t = tFac(nz);
  const uint32_t segmentColor = colorGroup.getColor(colNum);
  const uint32_t col = colorMix(color, segmentColor, t);
  const uint32_t colLow = colorMix(colorLow, segmentColor, t);
  colNum++;

  return std::make_tuple(col, colLow);
}

inline uint32_t TentacleLineColorer::colorMix(const uint32_t col1, const uint32_t col2, const float t) const
{
  const vivid::rgb_t c1 = vivid::rgb::fromRgb32(col1);
  const vivid::rgb_t c2 = vivid::rgb::fromRgb32(col2);
  if (reverseMixMode) {
    return vivid::lerpHsl(c2, c1, t).rgb32();
  } else {
    return vivid::lerpHsl(c1, c2, t).rgb32();
  }
}

/* 
 * VisualFX wrapper for the tentacles
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

void tentacle_fx_init(VisualFX* _this, PluginInfo* info)
{
//  ColorGroup::colorGroups = ColorGroup::getColorGroups();

  TentacleFXData* data = (TentacleFXData*)malloc(sizeof(TentacleFXData));

  data->tentacles = new Tentacles(info);

  data->enabled_bp = secure_b_param("Enabled", 1);
  data->params = plugin_parameters("3D Tentacles", 1);
  data->params.params[0] = &data->enabled_bp;

  data->cycle = 0.0f;
  data->col = (0x28 << (ROUGE * 8)) | (0x2c << (VERT * 8)) | (0x5f << (BLEU * 8));
  data->dstcol = 0;
  data->lig = 1.15f;
  data->ligs = 0.1f;

  data->distt = 10.0f;
  data->distt2 = 0.0f;
  data->rot = 0.0f; /* entre 0 et 2 * M_PI */
  data->happens = 0;

  data->rotation = 0;
  data->lock = 0;
  // data->colors;

  data->tentacles->init();

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

inline unsigned char lighten(unsigned char value, float power)
{
  int val = value;
  float t = (float)val * log10(power) / 2.0;

  if (t > 0) {
    val = (int)t; /* (32.0f * log (t)); */
    if (val > 255)
      val = 255;
    if (val < 0)
      val = 0;
    return val;
  } else {
    return 0;
  }
}

static void lightencolor(uint32_t* col, float power)
{
  uint8_t* color = (uint8_t*)col;

  *color = lighten(*color, power);
  color++;
  *color = lighten(*color, power);
  color++;
  *color = lighten(*color, power);
  color++;
  *color = lighten(*color, power);
}

/* retourne x>>s , en testant le signe de x */
#define ShiftRight(_x, _s) ((_x < 0) ? -(-_x >> _s) : (_x >> _s))

static int evolvecolor(uint32_t src, uint32_t dest, unsigned int mask, unsigned int incr)
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
  return (int)((src & mask) | color);
}

static void pretty_move(PluginInfo* goomInfo, float cycle, float* dist, float* dist2,
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
