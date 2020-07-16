#include "surf3d.h"

#include "drawmethods.h"
#include "goom_tools.h"
#include "mathtools.h"
//#include "SimplexNoise.h"

#include <cmath>
#include <algorithm>
#include <tuple>
#include <vector>

const float Grid::gridZeroLerpFactor = 0.8;
const float Grid::gridYMultiplier = 0.255;
const float Grid::gridPrevYMultiplier = 0.85;

constexpr int coordIgnoreVal = -666;

/*
 * rotation selon Y du v3d vi d'angle a (cosa=cos(a), sina=sin(a))
 * centerz = centre de rotation en z
 */
inline void y_rotate_v3d(const v3d& vi, v3d& vf, const float sina, const float cosa)                                                           \
{
  const float vi_x = vi.x;
  const float vi_z = vi.z;
  vf.x = vi_x * cosa - vi_z * sina;
  vf.z = vi_x * sina + vi_z * cosa;
  vf.y = vi.y;
}

/*
 * translation
 */
inline void translate_v3d(const v3d& vsrc, v3d& vdest)
{
  vdest.x += vsrc.x;
  vdest.y += vsrc.y;
  vdest.z += vsrc.z;
}

inline float getRandInRange(const float n1, const float n2)
{
  constexpr int intRange = 10000;
  return n1 + (n2 - n1)*float(pcg32_rand() % (intRange+1))/intRange;
}


Grid::Grid(const v3d& center,
  const int x_width_min, const int x_width_max, const size_t numx,
  const float zdepth_mins[], const float zdepth_maxs[], const size_t numz)
  : surf()
  , num_x{ numx }
  , num_z{ numz }
  , mode{ 0 }
{
  surf.center = center;
  surf.vertex.resize(num_x * num_z);
  surf.svertex.resize(surf.vertex.size());

  const float x_width_step = (x_width_min - x_width_max)/(float)(num_z-1);

  // For each z, get the x values
  float xval[num_x][num_z];
  float x_width = x_width_max;
  for (int nz=int(num_z)-1; nz >= 0; nz--) {
    const float x_start = x_width / 2.0;
    const float x_step = x_width / float(num_x-1);
    float x = x_start;
    for (int nx=int(num_x)-1; nx >= 0; nx--) {
      xval[size_t(nx)][size_t(nz)] = x;
      x -= x_step;
    }
    x_width -= x_width_step;
  }

  // For each x, get the z values
  float zval[num_x][num_z];
  for (int nx=num_x-1; nx >= 0; nx--) {
    const float z_depth = zdepth_maxs[nx] - zdepth_mins[nx];
    const float z_start = z_depth + zdepth_mins[nx];
    const float z_step = z_depth / float(num_z-1);
    float z = z_start;
    for (int nz=int(num_z)-1; nz >= 0; nz--) {
      zval[size_t(nx)][size_t(nz)] = z;
      z -= z_step;
    }
  }

  const VertNum vnum(num_x);
  for (int nz=int(num_z)-1; nz >= 0; nz--) {
    for (int nx=int(num_x)-1; nx >= 0; nx--) {
      const size_t nv = size_t(vnum(nx, nz));
      surf.vertex[nv].x = xval[size_t(nx)][size_t(nz)];
      surf.vertex[nv].y = 0.0;
      surf.vertex[nv].z = zval[size_t(nx)][size_t(nz)];
    }
  }
}

/*
 * projete le vertex 3D sur le plan d'affichage
 * retourne (0,0) si le point ne doit pas etre affiche.
 *
 * bonne valeur pour distance : 256
 */

static void project_v3d_to_v2d(const std::vector<v3d>& v3, int width, int height, float distance, std::vector<v2d>& v2)
{
  for (size_t i = 0; i < v3.size(); ++i) {
    if (v3[i].z > 2) {
      const int Xp = (int)(distance * v3[i].x / v3[i].z);
      const int Yp = (int)(distance * v3[i].y / v3[i].z);
      v2[i].x = Xp + (width >> 1);
      v2[i].y = -Yp + (height >> 1);
    } else {
      v2[i].x = v2[i].y = coordIgnoreVal;
    }
  }
}

void Grid::drawToBuffs(Pixel* front, Pixel* back, int width, int height,
                       LineColorer& lineColorer, float dist, uint32_t colorMod, uint32_t colorLowMod)
{
  std::vector<v2d> v2_array(surf.svertex.size());
  project_v3d_to_v2d(surf.svertex, width, height, dist, v2_array);

  const VertNum vnum(num_x);

  for (size_t x = 0; x < num_x; x++) {
    v2d v2x = v2_array[x];

    lineColorer.resetColorNum();
    lineColorer.setNumZ(num_z);

    for (size_t z = 1; z < num_z; z++) {
      const size_t i = size_t(vnum(x, z));
      const v2d v2 = v2_array[i];
      if ((v2x.x == v2.x) && (v2x.y == v2.y)) {
        v2x = v2;
        continue;
      }
      if (((v2.x != coordIgnoreVal) || (v2.y != coordIgnoreVal))
          && ((v2x.x != coordIgnoreVal) || (v2x.y != coordIgnoreVal))) {
        const auto colors = lineColorer.getColorMix(x, z, colorMod, colorLowMod);
        const uint32_t color = std::get<0>(colors);
        const uint32_t colorLow = std::get<1>(colors);;
//        GOOM_LOG_INFO("Drawing line: color = %x, colorLow = %x, v2x.x = %d, v2x.y = %d, v2.x = %d, v2.y = %d.",
//            color, colorLow, v2x.x, v2x.y, v2.x, v2.y);
        draw2DLineToBuffs(front, back, color, colorLow, width, height, v2x.x, v2x.y, v2.x, v2.y);
      }
      v2x = v2;
    }
  }
}

inline void Grid::draw2DLineToBuffs(Pixel* front, Pixel* back, uint32_t color, uint32_t colorLow,
                                    int width, int height, int x1, int y1, int x2, int y2) const
{
  Pixel* buffs[2] = { front, back };
  std::vector<Pixel> colors(2);
  colors[0].val = colorLow;
  colors[1].val = color;
  draw_line(std::size(buffs), buffs, colors, x1, y1, x2, y2, width, height);
}

void Grid::update(const float angle, const std::vector<float>& vals, const float dist)
{
  if (mode == 0) {
    static long iterNum = 0;
    iterNum++;
    if (!vals.empty()) {
      GOOM_LOG_INFO("Init floor.");
      for (size_t x = 0; x < num_x; x++) {
        GOOM_LOG_INFO("Iter %d: surf.vertex[%d].y = %f, vals[%d] = %f.", iterNum, x, surf.vertex[x].y, x, vals[x]);
        surf.vertex[x].y = std::lerp(surf.vertex[x].y, vals[x], gridZeroLerpFactor);
      }
    }

    const VertNum vnum(num_x);
    GOOM_LOG_INFO("Iter %d: Rest of tentacles.", iterNum);
    for (size_t x = 0; x < num_x; x++) {
//      const float lastY = 20*getRandInRange(0.8, 1.2);
      for (size_t y = 1; y < num_z; y++) {
        const size_t nv = size_t(vnum(x, y));
        const size_t prevRow_nv = size_t(vnum.getPrevRowVertNum(nv));
        GOOM_LOG_INFO("Iter %d: prevRow surf.vertex[%d,%d].y = %f.", iterNum, x, y-1, surf.vertex[prevRow_nv].y);
        GOOM_LOG_INFO("Iter %d: prior   surf.vertex[%d,%d].y = %f.", iterNum, x, y, surf.vertex[nv].y);
        surf.vertex[nv].y = gridYMultiplier*surf.vertex[nv].y + gridPrevYMultiplier*surf.vertex[prevRow_nv].y;
        GOOM_LOG_INFO("Iter %d: after   surf.vertex[%d,%d].y = %f.", iterNum, x, y, surf.vertex[nv].y);
//        surf.vertex[nv].y = std::min(float(surf.vertex[nv].y), lastY);
      }
    }
  }

  v3d cam = surf.center;
  cam.z += dist;
  // Need this after surf3d bug-fix.
  // '0.5*M_PI - angle' gets passed in as workaround but we need 'angle'.
  cam.y += 2.0 * sin(-(angle - 0.5*M_PI) / 4.3f);

  GOOM_LOG_INFO("Rotation angle = %f deg.", angle*360.0);
  const float cosa = cos(angle);
  const float sina = sin(angle);
  for (size_t i = 0; i < surf.vertex.size(); i++) {
    y_rotate_v3d(surf.vertex[i], surf.svertex[i], sina, cosa);
    translate_v3d(cam, surf.svertex[i]);
    GOOM_LOG_INFO("surf.svertex[%d].y = %f.", i, surf.svertex[i].y);
  }
}
