#ifndef _GFONTLIB_H
#define _GFONTLIB_H

#include "goom_graphic.h"

#include <cstdint>

namespace goom
{

extern void gfont_load(void);

extern void goom_draw_text(PixelBuffer&,
                           const uint16_t resolx,
                           const uint16_t resoly,
                           int x,
                           int y,
                           const char* str,
                           const float charspace,
                           const int center);

} // namespace goom
#endif
