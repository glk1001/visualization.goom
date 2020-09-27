#ifndef _GOOM_FX_H
#define _GOOM_FX_H

#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <cstddef>
#include <cstdint>

namespace goom
{

VisualFX convolve_create();

VisualFX flying_star_create(void);
void flying_star_log_stats(VisualFX* _this, const StatsLogValueFunc);

constexpr size_t BUFFPOINTNB = 16;

extern void zoom_filter_c(const uint16_t sizeX,
                          const uint16_t sizeY,
                          Pixel* src,
                          Pixel* dest,
                          const int* brutS,
                          const int* brutD,
                          const int buffratio,
                          const int precalCoef[BUFFPOINTNB][BUFFPOINTNB]);

} // namespace goom
#endif
