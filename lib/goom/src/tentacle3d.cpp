#include "tentacle3d.h"

#include "goom.h"
#include "goom_config.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "surf3d.h"
#include "v3d.h"

#include <vivid/vivid.h>
#include <vector>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

constexpr float D = 256.0f;

constexpr int yticks_per_100 = 20;
constexpr int y_height = 50;
constexpr int nbgrid = yticks_per_100 * y_height / 100;
constexpr int y_step_mod = 10;
constexpr float y_start = -0.5 * y_height;

constexpr int xticks_per_100 = 30;
constexpr int x_width = 40;
constexpr int x_width_mod = 10;
constexpr size_t num_x = xticks_per_100 * x_width / 100;
constexpr int num_x_mod = 0;

constexpr int zticks_per_100 = 350;
constexpr int z_depth = 20;
constexpr int z_depth_mod = 10;
constexpr size_t num_z = zticks_per_100 * z_depth / 100;
constexpr int num_z_mod = 0;


constexpr vivid::ColorMap::Preset baseColorMaps[] = {
  vivid::ColorMap::Preset::BlueYellow,
  vivid::ColorMap::Preset::Plasma,
  vivid::ColorMap::Preset::Viridis
};
constexpr size_t NUM_COLOR_GROUPS = std::size(baseColorMaps);
constexpr size_t NUM_COLORS_IN_GROUP = 33;
constexpr size_t NUM_TENTACLE_COLORS = NUM_COLOR_GROUPS * NUM_COLORS_IN_GROUP;

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

static std::vector<ColorGroup> colorGroups;

static void init_color_groups(std::vector<ColorGroup>& colGroups)
{
  static const std::vector<vivid::ColorMap::Preset> cmaps = {
    vivid::ColorMap::Preset::BlueYellow,
    vivid::ColorMap::Preset::CoolWarm,
    /**
    vivid::ColorMap::Preset::Hsl,
    vivid::ColorMap::Preset::HslPastel,
    vivid::ColorMap::Preset::Inferno,
    vivid::ColorMap::Preset::Magma,
    vivid::ColorMap::Preset::Plasma,
    **/
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

static void init_colors(uint32_t* colors)
{
  for (size_t i = 0; i < NUM_COLOR_GROUPS; i++) {
    const ColorGroup group { baseColorMaps[i], NUM_COLORS_IN_GROUP };
    for (size_t j = 0; j < group.numColors(); j++) {
      colors[j] = group.getColor(j);
    }
  }
}

typedef struct _TENTACLE_FX_DATA {
  PluginParam enabled_bp;
  PluginParameters params;

  float cycle;
  grid3d* grille[nbgrid];
  float* vals;

  uint32_t colors[NUM_TENTACLE_COLORS];

  int col;
  int dstcol;
  float lig;
  float ligs;

  /* statics from pretty_move */
  float distt;
  float distt2;
  float rot; /* entre 0 et 2 * M_PI */
  int happens;
  int rotation;
  int lock;
} TentacleFXData;

static void tentacle_new(TentacleFXData* data);
static void tentacle_update(PluginInfo* goomInfo, Pixel* buf, Pixel* back, int W, int H,
                            gint16 data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN], float, int drawit,
                            TentacleFXData* fx_data);
static void tentacle_free(TentacleFXData* data);
static void init_colors(uint32_t* colors);
static void init_color_groups(std::vector<ColorGroup>& colorGroups);

/* 
 * VisualFX wrapper for the tentacles
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void tentacle_fx_init(VisualFX* _this, PluginInfo* info)
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
  init_colors(data->colors);
  init_color_groups(colorGroups);
  tentacle_new(data);

  _this->params = &data->params;
  _this->fx_data = (void*)data;
}

#pragma GCC diagnostic pop

static void tentacle_fx_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* goomInfo)
{
  TentacleFXData* data = (TentacleFXData*)_this->fx_data;
  if (BVAL(data->enabled_bp)) {
    tentacle_update(goomInfo, dest, src, goomInfo->screen.width, goomInfo->screen.height,
                    goomInfo->sound.samples, (float)goomInfo->sound.accelvar,
                    goomInfo->curGState->drawTentacle, data);
  }
}

static void tentacle_fx_free(VisualFX* _this)
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

static void tentacle_free(TentacleFXData* data)
{
  /* TODO : un vrai FREE GRID!! */
  for (int tmp = 0; tmp < nbgrid; tmp++) {
    grid3d* g = data->grille[tmp];
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

static void tentacle_new(TentacleFXData* data)
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
    const float radius_min = 3.1;
    const float radius_max = zsize - radius_min;
    float angle = 0.0;
    const float angle_step = M_PI/(nx-1);
    float zdepth_mins[nx];
    float zdepth_maxs[nx];
    for (size_t i=0; i < nx; i++) {
      const float sin_angle = sin(angle);
//      zdepth_mins[i] = radius_min * (1 - sin_angle);
//      zdepth_maxs[i] = radius_min + radius_max * sin_angle;
      zdepth_mins[i] = radius_min;
      zdepth_maxs[i] = radius_min + radius_max;
      angle += angle_step;
    }

    center.y = y + get_rand_in_range(-y_step_mod / 2, y_step_mod / 2);
    center.z = zdepth_maxs[nbgrid-1];

    data->grille[i] = grid3d_new(center, xsize, xsize, nx, zdepth_mins, zdepth_maxs, nz);

    y += y_step;
  }
}

static inline unsigned char lighten(unsigned char value, float power)
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

static inline uint32_t color_multiply(uint32_t col1, uint32_t col2)
{
  uint8_t* color1 = (uint8_t*)&col1;
  uint8_t* color2 = (uint8_t*)&col2;
  uint32_t col;
  uint8_t* color = (uint8_t*)&col;

  *color = (*color1) * (*color2) / 256;
  color++;
  color1++;
  color2++;
  *color = (*color1) * (*color2) / 256;
  color++;
  color1++;
  color2++;
  *color = (*color1) * (*color2) / 256;
  color++;
  color1++;
  color2++;
  *color = (*color1) * (*color2) / 256;

  return col;
}

class TentacleDraw {
public:
  explicit TentacleDraw(PluginInfo* info, Pixel* front, Pixel* back, int w, int h, float d);
  void drawToBuffs(grid3d* grid, uint32_t colorMod, uint32_t colorLowMod);
private:
  PluginInfo* const goomInfo;
  Pixel* const frontBuff;
  Pixel* const backBuff;
  const int width;
  const int height;
  const float dist;
};

inline TentacleDraw::TentacleDraw(PluginInfo* info, Pixel* front, Pixel* back, int w, int h, float d)
  : goomInfo(info)
  , frontBuff(front)
  , backBuff(back)
  , width(w)
  , height(h)
  , dist(d)
{
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

class VertNum {
public:
  explicit VertNum(const int xw): xwidth(xw) {}
  int operator()(const int x, const int z) const { return z * xwidth + x; }
  std::tuple<int, int> getXZ(int vertNum) const
  {
    const int z = vertNum/xwidth;
    const int x = vertNum % xwidth;
    return std::make_tuple(x, z);
  }
private:
  const int xwidth;
};

inline uint32_t color_mix(const uint32_t col1, const uint32_t col2, const float t)
{
  const uint8_t* c1 = (uint8_t*)&col1;
  const uint8_t* c2 = (uint8_t*)&col2;
  const vivid::Color color1{ c1[1], c1[2], c1[3] };
  const vivid::Color color2{ c2[1], c2[2], c2[3] };
  return vivid::lerpHsl(color1, color2, t).rgb32();
}

inline void TentacleDraw::drawToBuffs(grid3d* grid, uint32_t colorMod, uint32_t colorLowMod)
{
  v2d v2_array[(size_t)grid->surf.nbvertex];
  v3d_to_v2d(grid->surf.svertex, grid->surf.nbvertex, width, height, dist, v2_array);

  const VertNum vnum(grid->defx);

  for (size_t x = 0; x < grid->defx; x++) {
    v2d v2x = v2_array[x];
    const ColorGroup colorGroup { colorGroups[goom_irand(goomInfo->gRandom, colorGroups.size())] };

    size_t colNum = 0;

    for (size_t z = 1; z < grid->defz; z++) {
      const int i = vnum(x, z);
      const v2d v2 = v2_array[i];
      if ((v2x.x == v2.x) && (v2x.y == v2.y)) {
        v2x = v2;
        continue;
      }
      if (((v2.x != -666) || (v2.y != -666)) && ((v2x.x != -666) || (v2x.y != -666))) {
        const float t = float(z+1)/float(grid->defz);
//        const float t = float(goom_irand(goomInfo->gRandom, 100))/100.0;
        const uint32_t segmentColor = colorGroup.getColor(colNum);
        const uint32_t color = color_mix(colorMod, segmentColor, t);
        const uint32_t colorlow = color_mix(colorLowMod, segmentColor, t);
//        const uint32_t color = color_multiply(segmentColor, colorMod);
//        const uint32_t colorlow = color_multiply(segmentColor, colorLowMod);
        goomInfo->methods.draw_line(frontBuff, v2x.x, v2x.y, v2.x, v2.y, colorlow, width, height);
        goomInfo->methods.draw_line(backBuff, v2x.x, v2x.y, v2.x, v2.y, color, width, height);
        colNum++;
      }
      v2x = v2;
    }
  }
}

static void tentacle_update(PluginInfo* goomInfo, Pixel* buf, Pixel* back, int W, int H,
                            gint16 data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN], float rapport,
                            int drawit, TentacleFXData* fx_data)
{
  float dist, dist2, rotangle;

  if ((!drawit) && (fx_data->ligs > 0.0f)) {
    fx_data->ligs = -fx_data->ligs;
  }

  fx_data->lig += fx_data->ligs;

  if (fx_data->lig > 1.01f) {
    if ((fx_data->lig > 10.0f) || (fx_data->lig < 1.1f)) {
      fx_data->ligs = -fx_data->ligs;
    }

    if ((fx_data->lig < 6.3f) && (goom_irand(goomInfo->gRandom, 30) == 0)) {
      fx_data->dstcol = (int)goom_irand(goomInfo->gRandom, NUM_TENTACLE_COLORS);
    }

    fx_data->col =
        evolvecolor((uint32_t)fx_data->col, (uint32_t)fx_data->colors[fx_data->dstcol], 0xff, 0x01);
    fx_data->col = evolvecolor((uint32_t)fx_data->col, (uint32_t)fx_data->colors[fx_data->dstcol],
                               0xff00, 0x0100);
    fx_data->col = evolvecolor((uint32_t)fx_data->col, (uint32_t)fx_data->colors[fx_data->dstcol],
                               0xff0000, 0x010000);
    fx_data->col = evolvecolor((uint32_t)fx_data->col, (uint32_t)fx_data->colors[fx_data->dstcol],
                               0xff000000, 0x01000000);
    uint32_t color = (uint32_t)fx_data->col;
    uint32_t colorlow = (uint32_t)fx_data->col;

    lightencolor(&color, fx_data->lig * 2.0f + 2.0f);
    lightencolor(&colorlow, (fx_data->lig / 3.0f) + 0.67f);

    rapport = 1.0f + 2.0f * (rapport - 1.0f);
    rapport *= 1.2f;
    if (rapport > 1.12f)
      rapport = 1.12f;

    pretty_move(goomInfo, fx_data->cycle, &dist, &dist2, &rotangle, fx_data);

    for (int tmp = 0; tmp < nbgrid; tmp++) {
      for (size_t tmp2 = 0; tmp2 < num_x; tmp2++) {
        const float val =
            (float)(ShiftRight(data[0][goom_irand(goomInfo->gRandom, AUDIO_SAMPLE_LEN - 1)], 10)) *
            rapport;
        fx_data->vals[tmp2] = val;
      }

      // Note: Following did not originally have '0.5*M_PI -' but with 'grid3d_update'
      //   bug-fix this is the counter-fix.
      grid3d_update(fx_data->grille[tmp], 0.5*M_PI - rotangle, fx_data->vals, dist2);
    }
    fx_data->cycle += 0.01f;

    TentacleDraw tentacle(goomInfo, buf, back, W, H, dist);
//    const ColorGroup colorGroup { colorGroups[goom_irand(goomInfo->gRandom, colorGroups.size())] };
//    size_t color_num = goom_irand(goomInfo->gRandom, colorGroup.numColors());
    for (int i = 0; i < nbgrid; i++) {
//      const ColorGroup colorGroup { colorGroups[goom_irand(goomInfo->gRandom, colorGroups.size())] };
//      const size_t color_num = goom_irand(goomInfo->gRandom, colorGroup.numColors());
//      const uint32_t tentacle_color = color_multiply(colorGroup.getColor(color_num), color);
//      const uint32_t tentacle_colorlow = color_multiply(colorGroup.getColor(color_num), colorlow);
//      color_num++;
//      if (color_num >= colorGroup.numColors()) {
//        color_num = 0;
//      }
      tentacle.drawToBuffs(fx_data->grille[i], color, colorlow);
    }
  } else {
    fx_data->lig = 1.05f;
    if (fx_data->ligs < 0.0f)
      fx_data->ligs = -fx_data->ligs;
    pretty_move(goomInfo, fx_data->cycle, &dist, &dist2, &rotangle, fx_data);
    fx_data->cycle += 0.1f;
    if (fx_data->cycle > 1000)
      fx_data->cycle = 0;
  }
}
