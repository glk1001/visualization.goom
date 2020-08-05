#include "surf3d.h"

#include "goom_plugin_info.h"
#include "mathtools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <tuple>

/*
 * projete le vertex 3D sur le plan d'affichage
 * retourne (0,0) si le point ne doit pas etre affiche.
 *
 * bonne valeur pour distance : 256
 */

static void v3d_to_v2d(const v3d* v3, int nbvertex, int width, int height, float distance, v2d* v2)
{
  for (int i = 0; i < nbvertex; ++i) {
    if (v3[i].z > 2) {
      const int Xp = (int)(distance * v3[i].x / v3[i].z);
      const int Yp = (int)(distance * v3[i].y / v3[i].z);
      v2[i].x = Xp + (width >> 1);
      v2[i].y = -Yp + (height >> 1);
    } else {
      v2[i].x = v2[i].y = -666;
    }
  }
}

/*
 * rotation selon Y du v3d vi d'angle a (cosa=cos(a), sina=sin(a))
 * centerz = centre de rotation en z
 */
static inline void Y_ROTATE_V3D(const v3d* vi, v3d* vf, const float sina, const float cosa)                                                           \
{
  vf->x = vi->x * cosa - vi->z * sina;
  vf->z = vi->x * sina + vi->z * cosa;
  vf->y = vi->y;
}

/*
 * translation
 */
static inline void TRANSLATE_V3D(const v3d* vsrc, v3d* vdest)
{
  vdest->x += vsrc->x;
  vdest->y += vsrc->y;
  vdest->z += vsrc->z;
}

class VertNum {
public:
  explicit VertNum(const int xw): xwidth(xw) {}
  int operator()(const int x, const int z) const { return z * xwidth + x; }
  std::tuple<int, int> getXZ(int vertNum) const
  {
    const int z = vertNum/xwidth;
    const int x = vertNum % xwidth;
    return std::make_tuple(x, z);
  }
private:
  const int xwidth;
};

grid3d* grid3d_new(
    const v3d center,
    const int x_width_0, const int x_width_n, const size_t num_x,
    const float zdepth_mins[], const float zdepth_maxs[], const size_t num_z)
{
  grid3d* g = (grid3d*)malloc(sizeof(grid3d));
  surf3d* s = &(g->surf);
  s->nbvertex = num_x * num_z;
  s->center = center;
  s->vertex = (v3d*)malloc((size_t)(s->nbvertex) * sizeof(v3d));
  s->svertex = (v3d*)malloc((size_t)(s->nbvertex) * sizeof(v3d));

  g->defx = num_x;
  g->defz = num_z;
  g->mode = 0;

  const float x_width_step = (x_width_n - x_width_0)/(float)(num_z-1);

  // For each z, get the x values
  float xval[num_x][num_z];
  float x_width = x_width_n;
  for (int nz=num_z-1; nz >= 0; nz--) {
    const float x_start = x_width / 2.0;
    const float x_step = x_width / (float)(num_x-1);
    float x = x_start;
    for (int nx=num_x-1; nx >= 0; nx--) {
      xval[nx][nz] = x;
      x -= x_step;
    }
    x_width -= x_width_step;
  }

  // For each x, get the z values
  float zval[num_x][num_z];
  for (int nx=num_x-1; nx >= 0; nx--) {
    const float z_depth = zdepth_maxs[nx] - zdepth_mins[nx];
    const float z_start = z_depth + zdepth_mins[nx];
    const float z_step = z_depth / (float)(num_z-1);
    float z = z_start;
    for (int nz=num_z-1; nz >= 0; nz--) {
      zval[nx][nz] = z;
      z -= z_step;
    }
  }

  const VertNum vnum(num_x);
  for (int nz=num_z-1; nz >= 0; nz--) {
    for (int nx=num_x-1; nx >= 0; nx--) {
      const int nv = vnum(nx, nz);
      s->vertex[nv].x = xval[nx][nz];
      s->vertex[nv].y = 0.0;
      s->vertex[nv].z = zval[nx][nz];
    }
  }

  return g;
}


void grid3d_draw(PluginInfo* plug, const grid3d* g, int color, int colorlow, int dist, Pixel* buf,
                 Pixel* back, int W, int H)
{
  v2d* v2_array = (v2d*)malloc((size_t)g->surf.nbvertex * sizeof(v2d));
  v3d_to_v2d(g->surf.svertex, g->surf.nbvertex, W, H, dist, v2_array);

  const VertNum vnum(g->defx);

  for (int x = 0; x < g->defx; x++) {
    v2d v2x = v2_array[x];

    for (int z = 1; z < g->defz; z++) {
      const int i = vnum(x, z);
      const v2d v2 = v2_array[i];
      if ((v2x.x == v2.x) && (v2x.y == v2.y)) {
        v2x = v2;
        continue;
      }
      if (((v2.x != -666) || (v2.y != -666)) && ((v2x.x != -666) || (v2x.y != -666))) {
        plug->methods.draw_line(buf, v2x.x, v2x.y, v2.x, v2.y, (uint32_t)colorlow, W, H);
        plug->methods.draw_line(back, v2x.x, v2x.y, v2.x, v2.y, (uint32_t)color, W, H);
      }
      v2x = v2;
    }
  }

  free(v2_array);
}

void grid3d_update(grid3d* g, float angle, float* vals, float dist)
{
  surf3d* s = &(g->surf);

  if (g->mode == 0) {
    if (vals)
      for (int i = 0; i < g->defx; i++) {
        s->vertex[i].y = s->vertex[i].y * 0.2 + vals[i] * 0.8;
      }

    for (int i = g->defx; i < s->nbvertex; i++) {
      s->vertex[i].y *= 0.255f;
      s->vertex[i].y += (s->vertex[i - g->defx].y * 0.777f);
    }
  }

  v3d cam = s->center;
  cam.z += dist;
  // Need this after surf3d bug-fix.
  // '0.5*M_PI - angle' gets passed in as workaround but we need 'angle'.
  cam.y += 2.0 * sin(-(angle - 0.5*M_PI) / 4.3f);

  const float cosa = cos(angle);
  const float sina = sin(angle);
  for (int i = 0; i < s->nbvertex; i++) {
    Y_ROTATE_V3D(&s->vertex[i], &s->svertex[i], sina, cosa);
    TRANSLATE_V3D(&cam, &s->svertex[i]);
  }
}
