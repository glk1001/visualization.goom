#ifndef _GOOM_FX_H
#define _GOOM_FX_H

#include "goom_config.h"
#include "goom_visual_fx.h"

#include <cstddef>
#include <cstdint>

namespace goom
{

VisualFX convolve_create();

VisualFX flying_star_create(void);
void flying_star_log_stats(VisualFX* _this, const StatsLogValueFunc);

} // namespace goom
#endif
