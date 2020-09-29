#ifndef _TENTACLE3D_H
#define _TENTACLE3D_H

#include "goom_config_param.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"

#include <algorithm>
#include <cstdint>

namespace goom
{

class TentaclesWrapper;

struct TentacleFXData
{
  PluginParam enabled_bp;
  PluginParameters params;
  TentaclesWrapper* tentacles;
};

VisualFX tentacle_fx_create();
void tentacle_free(TentacleFXData*);
void tentacle_fx_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo*);
void tentacle_fx_update_no_draw(VisualFX* _this, PluginInfo* goomInfo);
void tentacle_log_stats(VisualFX* _this, const StatsLogValueFunc);

inline float getRapport(const float accelvar)
{
  return std::min(1.12f, 1.2f * (1.0f + 2.0f * (accelvar - 1.0f)));
}

} // namespace goom
#endif
