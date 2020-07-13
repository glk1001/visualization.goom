#include "tentacle3d.h"

#include "drawmethods.h"
#include "goom.h"
#include "goom_config.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "surf3d.h"
#include "v3d.h"
#include "SimplexNoise.h"

#include <algorithm>

#include <vivid/vivid.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <cmath>
#include <tuple>
#include <vector>

constexpr float D = 256.0f;

class ColorGroup {
public:
  explicit ColorGroup(vivid::ColorMap::Preset preset, size_t n = num_z);
  ColorGroup(const ColorGroup& other);
  ColorGroup& operator=(const ColorGroup& other) { colors = other.colors; return *this; }
  size_t numColors() const { return colors.size(); }
  uint32_t getColor(size_t i) const { return colors[i]; }
private:
  ColorGroup();
  std::vector<uint32_t> colors;
};

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

static std::vector<ColorGroup> colorGroups;

static void init_color_groups(std::vector<ColorGroup>& colGroups)
{
  static const std::vector<vivid::ColorMap::Preset> cmaps = {
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
  colGroups.clear();
  for (auto cm : cmaps) {
    colGroups.push_back(ColorGroup(cm));
  }
}

/* 
 * VisualFX wrapper for the tentacles
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

void tentacle_fx_init(VisualFX* _this, PluginInfo* info)
{
  TentacleFXData* data = (TentacleFXData*)malloc(sizeof(TentacleFXData));

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
  init_color_groups(colorGroups);
  tentacle_new(data);

  _this->params = &data->params;
  _this->fx_data = (void*)data;
}

#pragma GCC diagnostic pop

void tentacle_fx_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* goomInfo)
{
  TentacleFXData* data = (TentacleFXData*)_this->fx_data;
  if (BVAL(data->enabled_bp)) {
    tentacle_update(goomInfo, dest, src, goomInfo->screen.width, goomInfo->screen.height,
                    goomInfo->sound.samples, (float)goomInfo->sound.accelvar,
                    goomInfo->curGState->drawTentacle, data);
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

/* ----- */

void tentacle_free(TentacleFXData* data)
{
  /* TODO : un vrai FREE GRID!! */
  for (int i = 0; i < nbgrid; i++) {
    grid3d* g = data->grille[i];
    free(g->surf.vertex);
    free(g->surf.svertex);
    free(g);
  }
  free(data->vals);
}

inline int get_rand_in_range(int n1, int n2)
{
  const uint32_t range_len = (uint32_t)(n2 - n1 + 1);
  return n1 + (int)(pcg32_rand() % range_len);
}

void tentacle_new(TentacleFXData* data)
{
  // Start at bottom of grid, going up by 'y_step'.
  const float y_step = y_height / (float) (nbgrid - 1) + get_rand_in_range(-y_step_mod / 2, y_step_mod / 2);
  v3d center = {0, 0, 0};
  data->vals = (float*)malloc((num_x + 20) * sizeof(float));
  
  float y = y_start;
  for (int i = 0; i < nbgrid; i++) {
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

    data->grille[i] = grid3d_new(center, xsize, xsize, nx, zdepth_mins, zdepth_maxs, nz);

    y += y_step;
  }
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

class TentacleDraw {
public:
  explicit TentacleDraw(int w, int h);
  void drawToBuffs(PluginInfo* goomInfo, Pixel* front, Pixel* back,
                   grid3d* grid, float dist, uint32_t colorMod, uint32_t colorLowMod);
  void changeColors(PluginInfo* goomInfo);
  void changeReverseMix();
private:
  const int width;
  const int height;
  ColorGroup colorGroup;
  void drawLineToBuffs(Pixel* front, Pixel* back,
                       uint32_t color, uint32_t colorLow, int x1, int y1, int x2, int y2) const;
  bool reverseMixMode = false;
  uint32_t colorMix(const uint32_t col1, const uint32_t col2, const float t);
};

inline TentacleDraw::TentacleDraw(int w, int h)
  : width(w)
  , height(h)
  , colorGroup { colorGroups[0] }
{
}

inline void TentacleDraw::drawLineToBuffs(Pixel* front, Pixel* back, uint32_t color, uint32_t colorLow,
                                          int x1, int y1, int x2, int y2) const
{
  Pixel* buffs[2] = { front, back };
  std::vector<Pixel> colors(2);
  colors[0].val = colorLow;
  colors[1].val = color;
  draw_line(std::size(buffs), buffs, colors, x1, y1, x2, y2, width, height);
}

/*
 * projete le vertex 3D sur le plan d'affichage
 * retourne (0,0) si le point ne doit pas etre affiche.
 *
 * bonne valeur pour distance : 256
 */

static void v3d_to_v2d(const v3d* v3, int nbvertex, int width, int height, float distance, v2d* v2)
{
  for (int i = 0; i < nbvertex; ++i) {
    if (v3[i].z > 2) {
      const int Xp = (int)(distance * v3[i].x / v3[i].z);
      const int Yp = (int)(distance * v3[i].y / v3[i].z);
      v2[i].x = Xp + (width >> 1);
      v2[i].y = -Yp + (height >> 1);
    } else {
      v2[i].x = v2[i].y = -666;
    }
  }
}

inline uint32_t TentacleDraw::colorMix(const uint32_t col1, const uint32_t col2, const float t)
{
  const vivid::rgb_t c1 = vivid::rgb::fromRgb32(col1);
  const vivid::rgb_t c2 = vivid::rgb::fromRgb32(col2);
  if (reverseMixMode) {
    return vivid::lerpHsl(c2, c1, t).rgb32();
  } else {
    return vivid::lerpHsl(c1, c2, t).rgb32();
  }
}

inline void TentacleDraw::changeColors(PluginInfo* goomInfo)
{
  colorGroup = colorGroups[goom_irand(goomInfo->gRandom, colorGroups.size())];
}

inline void TentacleDraw::changeReverseMix()
{
  reverseMixMode = not reverseMixMode;
}

inline void TentacleDraw::drawToBuffs(
  PluginInfo* goomInfo, Pixel* front, Pixel* back,
  grid3d* grid, float dist, uint32_t colorMod, uint32_t colorLowMod)
{
  v2d v2_array[(size_t)grid->surf.nbvertex];
  v3d_to_v2d(grid->surf.svertex, grid->surf.nbvertex, width, height, dist, v2_array);

  auto tFac = [=](size_t z) -> float {
    return float(z+1)/float(grid->defz);
//    return (float(z+1)/float(grid->defz))*(0.5 + float(goom_irand(goomInfo->gRandom, 5))/8.0);
  };

  const VertNum vnum(grid->defx);

  for (size_t x = 0; x < grid->defx; x++) {
    v2d v2x = v2_array[x];

    size_t colNum = 0;
    for (size_t z = 1; z < grid->defz; z++) {
      const int i = vnum(x, z);
      const v2d v2 = v2_array[i];
      if ((v2x.x == v2.x) && (v2x.y == v2.y)) {
        v2x = v2;
        continue;
      }
      if (((v2.x != -666) || (v2.y != -666)) && ((v2x.x != -666) || (v2x.y != -666))) {
        const float t = tFac(z);
        const uint32_t segmentColor = colorGroup.getColor(colNum);
        const uint32_t color = colorMix(colorMod, segmentColor, t);
        const uint32_t colorLow = colorMix(colorLowMod, segmentColor, t);
//        GOOM_LOG_INFO("Drawing line: color = %x, colorLow = %x, v2x.x = %d, v2x.y = %d, v2.x = %d, v2.y = %d.",
//            color, colorLow, v2x.x, v2x.y, v2.x, v2.y);
        drawLineToBuffs(front, back, color, colorLow, v2x.x, v2x.y, v2.x, v2.y);
        colNum++;
      }
      v2x = v2;
    }
  }
}

inline std::tuple<uint32_t, uint32_t> getModColors(PluginInfo* goomInfo,
                                                   TentacleFXData* fx_data, const ColorGroup& colorGroup)
{
  if ((fx_data->lig > 10.0f) || (fx_data->lig < 1.1f)) {
    fx_data->ligs = -fx_data->ligs;
  }

  if ((fx_data->lig < 6.3f) && (goom_irand(goomInfo->gRandom, 30) == 0)) {
//    fx_data->dstcol = (int)goom_irand(goomInfo->gRandom, NUM_TENTACLE_COLORS);
    fx_data->dstcol = (int)goom_irand(goomInfo->gRandom, colorGroup.numColors()); // TODO Make numColors a constexpr
  }

  /**
  fx_data->col = evolvecolor((uint32_t)fx_data->col, (uint32_t)fx_data->colors[fx_data->dstcol], 0xff, 0x01);
  fx_data->col = evolvecolor((uint32_t)fx_data->col, (uint32_t)fx_data->colors[fx_data->dstcol], 0xff00, 0x0100);
  fx_data->col = evolvecolor((uint32_t)fx_data->col, (uint32_t)fx_data->colors[fx_data->dstcol], 0xff0000, 0x010000);
  fx_data->col = evolvecolor((uint32_t)fx_data->col, (uint32_t)fx_data->colors[fx_data->dstcol], 0xff000000, 0x01000000);
  **/
  const uint32_t baseColor = colorGroup.getColor(size_t(fx_data->dstcol));
  fx_data->col = evolvecolor((uint32_t)fx_data->col, baseColor, 0xff, 0x01);
  fx_data->col = evolvecolor((uint32_t)fx_data->col, baseColor, 0xff00, 0x0100);
  fx_data->col = evolvecolor((uint32_t)fx_data->col, baseColor, 0xff0000, 0x010000);
  fx_data->col = evolvecolor((uint32_t)fx_data->col, baseColor, 0xff000000, 0x01000000);
  uint32_t color = uint32_t(fx_data->col);
  uint32_t colorLow = color;

  lightencolor(&color, fx_data->lig * 2.0f + 2.0f);
  lightencolor(&colorLow, (fx_data->lig / 3.0f) + 0.67f);

  return std::make_tuple(color, colorLow);
}

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

inline float randFactor(PluginInfo* goomInfo, const float min)
{
  return min + (1.0 - min)*float(goom_irand(goomInfo->gRandom, 101))/100.0;
}

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

void tentacle_update(PluginInfo* goomInfo, Pixel* buf, Pixel* back, int width, int height,
                     gint16 data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN], float accelvar,
                     int drawit, TentacleFXData* fx_data)
{
  static SineWave sineWave{ 5.0 };
  const float modAccelvar = sineWave.getNext();
//  const float modAccelvar = accelvar;

  static ChangeTracker accelVarTracker{ 10 };
  accelVarTracker.nextVal(accelvar);
  if (accelVarTracker.significantIncreaseInARow()) {
    sineWave.setSinFrequency(std::min(10.0f, sineWave.getSinFrequency() + 1.0f));
  } else if (accelVarTracker.significantDecreaseInARow()) {
    sineWave.setSinFrequency(std::max(0.5f, sineWave.getSinFrequency() - 1.0f));
  }

  if ((!drawit) && (fx_data->ligs > 0.0f)) {
    fx_data->ligs = -fx_data->ligs;
  }
  fx_data->lig += fx_data->ligs;

  if (fx_data->lig > 1.01f) {
    float dist, dist2, rotangle;
    pretty_move(goomInfo, fx_data->cycle, &dist, &dist2, &rotangle, fx_data);

    static ColorGroup modColorGroup{ colorGroups[goom_irand(goomInfo->gRandom, colorGroups.size())] };
    if (fx_data->happens) {
//    if (goom_irand(goomInfo->gRandom, 100) == 0) {
      modColorGroup = colorGroups[goom_irand(goomInfo->gRandom, colorGroups.size())];
    }
    const auto modColors = getModColors(goomInfo, fx_data, modColorGroup);
    const uint32_t modColor = std::get<0>(modColors);
    const uint32_t modColorLow = std::get<1>(modColors);

    const float rapport = getRapport(modAccelvar);

    const float nx_div2 = 0.5 * float(num_x);
    for (int i = 0; i < nbgrid; i++) {
      const float val = 1.7*(goom_irand(goomInfo->gRandom, 101)/100.0) * rapport;
//      const float val =
//          (float)(ShiftRight(data[0][goom_irand(goomInfo->gRandom, AUDIO_SAMPLE_LEN - 1)], 10)) * rapport;
      for (size_t x = 0; x < num_x; x++) {
//        const float val =
//            (float)(ShiftRight(data[0][goom_irand(goomInfo->gRandom, AUDIO_SAMPLE_LEN - 1)], 10)) * rapport;
//        const float factor = 0.9  + 0.1*(std::abs(nx_div2 - float(x))/nx_div2);
//        fx_data->vals[x] = val * factor * randFactor(goomInfo, 0.97);
        fx_data->vals[x] = val * randFactor(goomInfo, 0.97);
      }

      // Note: Following did not originally have '0.5*M_PI -' but with 'grid3d_update'
      //   bug-fix this is the counter-fix.
      grid3d_update(goomInfo, fx_data->grille[i], 0.5*M_PI - rotangle, fx_data->vals, dist2);
    }
    fx_data->cycle += 0.01f;

    static TentacleDraw tentacle(width, height); // dodge static!
    if (!fx_data->happens) {
      if (goom_irand(goomInfo->gRandom, 20) == 0) {
        tentacle.changeColors(goomInfo);
      }
    } else {
      tentacle.changeColors(goomInfo);
      if (goom_irand(goomInfo->gRandom, 50) == 0) {
        tentacle.changeReverseMix();
      }
    }

    for (int i = 0; i < nbgrid; i++) {
      tentacle.drawToBuffs(goomInfo, buf, back, fx_data->grille[i], dist, modColor, modColorLow);
    }
  } else {
    fx_data->lig = 1.05f;
    if (fx_data->ligs < 0.0f) {
      fx_data->ligs = -fx_data->ligs;
    }
    float dist, dist2, rotangle;
    pretty_move(goomInfo, fx_data->cycle, &dist, &dist2, &rotangle, fx_data);
    fx_data->cycle += 0.1f;
    if (fx_data->cycle > 1000) {
      fx_data->cycle = 0;
    }
  }
}
