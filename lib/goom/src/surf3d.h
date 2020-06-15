#ifndef _SURF3D_H
#define _SURF3D_H

#include "goom_graphic.h"
#include "goom_typedefs.h"
#include "v3d.h"

#include <cstdlib>

typedef struct {
  v3d* vertex;
  v3d* svertex;
  int nbvertex;

  v3d center;
} surf3d;

typedef struct {
  surf3d surf;
  int defx;
  int defz;
  int mode;
} grid3d;

/* hi-level */

/* works on grid3d */
grid3d* grid3d_new(
    const v3d center,
    const int x_width_0, const int x_width_n, const size_t num_x,
    const float zdepth_mins[], const float zdepth_maxs[], const size_t num_z);
void grid3d_update(grid3d* g, float angle, float* vals, float dist);

/* low level */
void grid3d_draw(PluginInfo* plug, const grid3d* g, int color, int colorlow, int dist, Pixel* buf,
                 Pixel* back, int W, int H);

#endif
