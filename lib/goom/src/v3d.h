#ifndef _V3D_H
#define _V3D_H

typedef struct
{
  float x = 0;
  float y = 0;
  float z = 0;
  bool ignore = false;
} v3d;

typedef struct
{
  int x, y;
} v2d;

typedef struct
{
  double x, y;
} v2g;

#endif
