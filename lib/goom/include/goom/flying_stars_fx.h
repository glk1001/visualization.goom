#ifndef _FLYING_STARS_FX_H
#define _FLYING_STARS_FX_H

#include "goom_config.h"
#include "goom_visual_fx.h"

namespace goom
{

VisualFX flying_star_create(void);
void flying_star_log_stats(VisualFX* _this, const StatsLogValueFunc);

} // namespace goom
#endif
