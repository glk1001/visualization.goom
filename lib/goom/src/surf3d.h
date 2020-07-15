#ifndef _SURF3D_H
#define _SURF3D_H

#include "goom_graphic.h"
#include "v3d.h"

#include <cstdlib>
#include <cstdint>
#include <tuple>
#include <vector>

class VertNum {
public:
  explicit VertNum(const int xw): xwidth(xw) {}
  int operator()(const int x, const int z) const { return z * xwidth + x; }
  int getPrevRowVertNum(const int vertNum) const { return vertNum - xwidth; }
  std::tuple<int, int> getXZ(const int vertNum) const
  {
    const int z = vertNum/xwidth;
    const int x = vertNum % xwidth;
    return std::make_tuple(x, z);
  }
private:
  const int xwidth;
};

class LineColorer {
public:
  virtual void resetColorNum()=0;
  virtual void setNumZ(size_t numz)=0;
  virtual std::tuple<uint32_t, uint32_t> getColorMix(size_t nx, size_t nz,
                                                     const uint32_t color, const uint32_t colorLow)=0;
  virtual ~LineColorer() {}                                                      
};

struct Surface {
  std::vector<v3d> vertex;
  std::vector<v3d> svertex;
  v3d center;
};

class Grid {
public:
  Grid(const v3d& center,
       const int x_width_min, const int x_width_max, const size_t num_x,
       const float zdepth_mins[], const float zdepth_maxs[], const size_t num_z);
  void update(const float angle, const std::vector<float>& vals, const float dist);
  void drawToBuffs(Pixel* front, Pixel* back, int width, int height,
                   LineColorer&, float dist, uint32_t colorMod, uint32_t colorLowMod);
private:
  Surface surf;
  const size_t num_x;
  const size_t num_z;
  int mode;
  void drawLineToBuffs(Pixel* front, Pixel* back, uint32_t color, uint32_t colorLow,
                       int width, int height, int x1, int y1, int x2, int y2) const;
};

#endif
