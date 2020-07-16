#ifndef _V3D_H
#define _V3D_H

typedef struct {
  float x, y, z;
  bool ignore = false;
} v3d;

typedef struct {
  int x, y;
} v2d;

typedef struct {
  double x, y;
} v2g;

#endif
