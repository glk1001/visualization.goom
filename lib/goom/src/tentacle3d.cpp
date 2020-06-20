#include "tentacle3d.h"

#include "goom.h"
#include "goom_config.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "surf3d.h"
#include "v3d.h"

#include <vivid/vivid.h>
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
constexpr int x_width = 25;
constexpr int x_width_mod = 10;
constexpr size_t num_x = xticks_per_100 * x_width / 100;
constexpr int num_x_mod = 0;

constexpr int zticks_per_100 = 400;
constexpr int z_depth = 25;
constexpr int z_depth_mod = 10;
constexpr size_t num_z = zticks_per_100 * z_depth / 100;
constexpr int num_z_mod = 0;


constexpr int NB_TENTACLE_COLORS = 100;
constexpr int NUM_COLORS_IN_GROUP = NB_TENTACLE_COLORS / 3;

typedef struct _TENTACLE_FX_DATA {
  PluginParam enabled_bp;
  PluginParameters params;

  float cycle;
  grid3d* grille[nbgrid];
  float* vals;

  uint32_t colors[NB_TENTACLE_COLORS];

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
    const int xsize = x_width + get_rand_in_range(-x_width_mod / 2, x_width_mod / 2);

    const size_t nz = num_z + (size_t)get_rand_in_range(-num_z_mod, 0);
    const int zsize = z_depth + get_rand_in_range(-z_depth_mod / 2, z_depth_mod / 2);
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

static void init_colors(uint32_t* colors)
{
  for (int i = 0; i < NB_TENTACLE_COLORS; i++) {
    const uint8_t red = get_rand_in_range(20, 90);
    const uint8_t green = get_rand_in_range(20, 90);
    const uint8_t blue = get_rand_in_range(20, 90);
    colors[i] = (uint32_t)((red << (ROUGE * 8)) | (green << (VERT * 8)) | (blue << (BLEU * 8)));
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

static void tentacle_update(PluginInfo* goomInfo, Pixel* buf, Pixel* back, int W, int H,
                            gint16 data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN], float rapport,
                            int drawit, TentacleFXData* fx_data)
{
  float dist, dist2, rotangle;

  if ((!drawit) && (fx_data->ligs > 0.0f))
    fx_data->ligs = -fx_data->ligs;

  fx_data->lig += fx_data->ligs;

  if (fx_data->lig > 1.01f) {
    if ((fx_data->lig > 10.0f) || (fx_data->lig < 1.1f))
      fx_data->ligs = -fx_data->ligs;

    if ((fx_data->lig < 6.3f) && (goom_irand(goomInfo->gRandom, 30) == 0))
      fx_data->dstcol = (int)goom_irand(goomInfo->gRandom, NB_TENTACLE_COLORS);

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

    uint32_t tentacle_color = (uint32_t)fx_data->colors[0] * color;
    uint32_t tentacle_colorlow = (uint32_t)fx_data->colors[0] * colorlow;
    int color_num = (int)goom_irand(goomInfo->gRandom, NB_TENTACLE_COLORS);
    int num_colors_in_row = 0;
    for (int i = 0; i < nbgrid; i++) {
      if (num_colors_in_row >= NUM_COLORS_IN_GROUP) {
        tentacle_color = color_multiply(fx_data->colors[color_num], color);
        tentacle_colorlow = color_multiply(fx_data->colors[color_num], colorlow);
        color_num++;
        if (color_num >= NB_TENTACLE_COLORS)
          color_num = 0;
        num_colors_in_row = 0;
      }
      num_colors_in_row++;
      grid3d_draw(goomInfo, fx_data->grille[i], (int)tentacle_color, (int)tentacle_colorlow, dist,
                  buf, back, W, H);
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
