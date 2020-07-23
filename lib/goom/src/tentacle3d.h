#ifndef _TENTACLE3D_H
#define _TENTACLE3D_H

#include "goom_visual_fx.h"
#include "surf3d.h"

#include <algorithm>
#include <memory>
#include <tuple>
#include <vector>

constexpr int yticks_per_100 = 20;
constexpr int y_height = 50;
constexpr int nbgrid = yticks_per_100 * y_height / 100;
constexpr int y_step_mod = 20;
constexpr float y_start = -0.5 * y_height;

constexpr int xticks_per_100 = 30;
constexpr int x_width = 40;
constexpr int x_width_mod = 10;
constexpr size_t num_x = xticks_per_100 * x_width / 100;
constexpr int num_x_mod = 0;

constexpr int zticks_per_100 = 350;
constexpr int z_depth = 15;
constexpr int z_depth_mod = 5;
constexpr size_t num_z = zticks_per_100 * z_depth / 100;
constexpr int num_z_mod = 0;

class Tentacles;

typedef struct _TENTACLE_FX_DATA {
  PluginParam enabled_bp;
  PluginParameters params;

  Tentacles* tentacles;

  float cycle;

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

class SineWave;
class ColorGroup;
class TentacleLineColorer;
class ChangeTracker;

class Tentacles {
public:
  explicit Tentacles(PluginInfo* goomInfo);
  void init();
  void update(PluginInfo* goomInfo, Pixel* buf, Pixel* back,
              gint16 data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN], float accelvar,
              int drawit, TentacleFXData* fx_data);
private:
  const int width;
  const int height;
  std::vector<std::unique_ptr<Grid>> grids;
  std::unique_ptr<SineWave> sineWave;
  std::unique_ptr<ColorGroup> modColorGroup;
  std::unique_ptr<TentacleLineColorer> lineColorer;
  std::unique_ptr<ChangeTracker> accelVarTracker;
  std::tuple<uint32_t, uint32_t> getModColors(PluginInfo* goomInfo, TentacleFXData* fx_data);
  void updateColors(PluginInfo* goomInfo, const TentacleFXData* fx_data);
  std::vector<float> getGridZeroAdditiveValues(PluginInfo* goomInfo, float rapport);
  static constexpr float maxSinFrequency = 0.2;
  static constexpr float minSinFrequency = 0.1;
  void updateSineWaveParams(float accelvar);
};

VisualFX tentacle_fx_create(void);
void tentacle_free(TentacleFXData* data);
void tentacle_fx_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* goomInfo);

inline float getRapport(const float accelvar)
{
  return std::min(1.12f, 1.2f*(1.0f + 2.0f*(accelvar - 1.0f)));
}

#endif
