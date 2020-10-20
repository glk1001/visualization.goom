#ifndef _TENTACLES_FX_H
#define _TENTACLES_FX_H

#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"

#include <cstdint>

namespace goom
{

class TentaclesWrapper;

struct TentacleFXData
{
  bool enabled = true;
  TentaclesWrapper* tentacles;
};

VisualFX tentacle_fx_create();
void tentacle_free(TentacleFXData*);
void tentacle_fx_apply(VisualFX* _this, PluginInfo*, Pixel* prevBuff, Pixel* currentBuff);
void tentacle_fx_update_no_draw(VisualFX* _this, PluginInfo* goomInfo);
void tentacle_log_stats(VisualFX* _this, const StatsLogValueFunc);

} // namespace goom
#endif
