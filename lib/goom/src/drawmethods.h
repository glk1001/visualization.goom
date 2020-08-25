#ifndef _DRAWMETHODS_H
#define _DRAWMETHODS_H

#include "goom_config.h"
#include "goom_graphic.h"

#include <cstddef>
#include <stdint.h>
#include <vector>

extern void draw_line(size_t n,
                      Pixel* buffs[],
                      const std::vector<Pixel>& cols,
                      int x1,
                      int y1,
                      int x2,
                      int y2,
                      int screenx,
                      int screeny);
extern void draw_line(
    Pixel* data, int x1, int y1, int x2, int y2, uint32_t col, int screenx, int screeny);

#endif /* _DRAWMETHODS_H */
