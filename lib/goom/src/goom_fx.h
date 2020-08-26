#ifndef _GOOM_FX_H
#define _GOOM_FX_H

#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <cstddef>
#include <cstdint>

extern VisualFX convolve_create();
extern VisualFX flying_star_create(void);

constexpr size_t BUFFPOINTNB = 16;

extern void zoom_filter_c(const uint16_t sizeX,
                          const uint16_t sizeY,
                          Pixel* src,
                          Pixel* dest,
                          const int* brutS,
                          const int* brutD,
                          const int buffratio,
                          const int precalCoef[BUFFPOINTNB][BUFFPOINTNB]);

#endif
