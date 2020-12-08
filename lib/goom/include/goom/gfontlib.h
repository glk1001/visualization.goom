#ifndef _GFONTLIB_H
#define _GFONTLIB_H

#include "goom_graphic.h"

#include <cstdint>

namespace goom
{

void gfont_load();

void goom_draw_text(PixelBuffer&,
                    uint16_t resolx,
                    uint16_t resoly,
                    int x,
                    int y,
                    const char* str,
                    float charspace,
                    int center);

} // namespace goom
#endif
