#ifndef _TENTACLE3D_H
#define _TENTACLE3D_H

#include "goom_visual_fx.h"
#include "surf3d.h"

class TentaclesWrapper;

typedef struct _TENTACLE_FX_DATA {
  PluginParam enabled_bp;
  PluginParameters params;

  TentaclesWrapper* tentacles;

  float cycle;

  unsigned int col;
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

VisualFX tentacle_fx_create(void);
void tentacle_free(TentacleFXData* data);
void tentacle_fx_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* goomInfo);

inline float getRapport(const float accelvar)
{
  return std::min(1.12f, 1.2f*(1.0f + 2.0f*(accelvar - 1.0f)));
}

#endif
