#ifndef _GFONTLIB_H
#define _GFONTLIB_H

#include "goom_graphic.h"

#include <cstdint>

extern void gfont_load(void);

extern void goom_draw_text(Pixel* buf,
                           const uint16_t resolx,
                           const uint16_t resoly,
                           int x,
                           int y,
                           const char* str,
                           const float charspace,
                           const int center);

#endif
