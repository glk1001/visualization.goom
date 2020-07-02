#include "drawmethods.h"


static inline void draw_pixel(Pixel* frontBuff, Pixel* backBuff, uint32_t col)
{
  int tra = 0;
  unsigned char* bra = (unsigned char*)backBuff;
  unsigned char* dra = (unsigned char*)frontBuff;
  unsigned char* cra = (unsigned char*)&col;
  for (int i=0; i < 4; i++) {
    tra = *cra;
    tra += *bra;
    if (tra > 255) {
      tra = 255;
    }
    *dra = tra;
    ++dra;
    ++cra;
    ++bra;
  }                                                                                              \
}

void draw_line(Pixel* data, int x1, int y1, int x2, int y2, uint32_t col, int screenx, int screeny)
{
  Pixel* buffs[] = { data };
  uint32_t cols[] = { col };
  draw_line(1, buffs, cols, x1, y1, x2, y2, screenx, screeny);
}

static inline void draw_pixels(size_t n, Pixel* buffs[], const uint32_t cols[], const int pos)
{
  for (size_t i = 0; i < n ; i++) {
    Pixel* const p = &(buffs[i][pos]);
    draw_pixel(p, p, cols[i]);
  }
}

void draw_line(size_t n, Pixel* buffs[], const uint32_t cols[], int x1, int y1, int x2, int y2, int screenx, int screeny)
{
  if ((y1 < 0) || (y2 < 0) || (x1 < 0) || (x2 < 0) || (y1 >= screeny) || (y2 >= screeny) ||
      (x1 >= screenx) || (x2 >= screenx)) {
    return;
  }

  /* clip to top edge
     if ((y1 < 0) && (y2 < 0))
     return;
	
     if (y1 < 0) {
     x1 += (y1 * (x1 - x2)) / (y2 - y1);
     y1 = 0;
     }
     if (y2 < 0) {
     x2 += (y2 * (x1 - x2)) / (y2 - y1);
     y2 = 0;
     }
         
     clip to bottom edge 
     if ((y1 >= screeny) && (y2 >= screeny))
     return;
     if (y1 >= screeny) {
     x1 -= ((screeny - y1) * (x1 - x2)) / (y2 - y1);
     y1 = screeny - 1;
     }
     if (y2 >= screeny) {
     x2 -= ((screeny - y2) * (x1 - x2)) / (y2 - y1);
     y2 = screeny - 1;
     }
     clip to left edge 
     if ((x1 < 0) && (x2 < 0))
     return;
     if (x1 < 0) {
     y1 += (x1 * (y1 - y2)) / (x2 - x1);
     x1 = 0;
     }
     if (x2 < 0) {
     y2 += (x2 * (y1 - y2)) / (x2 - x1);
     x2 = 0;
     }
     clip to right edge 
     if ((x1 >= screenx) && (x2 >= screenx))
     return;
     if (x1 >= screenx) {
     y1 -= ((screenx - x1) * (y1 - y2)) / (x2 - x1);
     x1 = screenx - 1;
     }
     if (x2 >= screenx) {
     y2 -= ((screenx - x2) * (y1 - y2)) / (x2 - x1);
     x2 = screenx - 1;
     }
  */

  int dx = x2 - x1;
  int dy = y2 - y1;
  if (x1 > x2) {
    int tmp = x1;
    x1 = x2;
    x2 = tmp;
    tmp = y1;
    y1 = y2;
    y2 = tmp;
    dx = x2 - x1;
    dy = y2 - y1;
  }

  /* vertical line */
  if (dx == 0) {
    if (y1 < y2) {
      int i = (screenx * y1) + x1;
      for (int y = y1; y <= y2; y++) {
        draw_pixels(n, buffs, cols, i);
        i += screenx;
      }
    } else {
      int i = (screenx * y2) + x1;
      for (int y = y2; y <= y1; y++) {
        draw_pixels(n, buffs, cols, i);
        i += screenx;
      }
    }
    return;
  }
  /* horizontal line */
  if (dy == 0) {
    if (x1 < x2) {
      int i = (screenx * y1) + x1;
      for (int x = x1; x <= x2; x++) {
        draw_pixels(n, buffs, cols, i);
        i++;
      }
      return;
    } else {
      int i = (screenx * y1) + x2;
      for (int x = x2; x <= x1; x++) {
        draw_pixels(n, buffs, cols, i);
        i++;
      }
      return;
    }
  }
  /* 1    */
  /* \   */
  /* \  */
  /* 2 */
  if (y2 > y1) {
    /* steep */
    if (dy > dx) {
      dx = ((dx << 16) / dy);
      int x = x1 << 16;
      for (int y = y1; y <= y2; y++) {
        const int xx = x >> 16;
        int i = (screenx * y) + xx;
        draw_pixels(n, buffs, cols, i);
        if (xx < (screenx - 1)) {
          i++;
          /* DRAWMETHOD; */
        }
        x += dx;
      }
      return;
    }
    /* shallow */
    else {
      dy = ((dy << 16) / dx);
      int y = y1 << 16;
      for (int x = x1; x <= x2; x++) {
        const int yy = y >> 16;
        int i = (screenx * yy) + x;
        draw_pixels(n, buffs, cols, i);
        if (yy < (screeny - 1)) {
          i += screeny;
          /* DRAWMETHOD; */
        }
        y += dy;
      }
    }
  }
  /* 2 */
  /* /  */
  /* /   */
  /* 1    */
  else {
    /* steep */
    if (-dy > dx) {
      dx = ((dx << 16) / -dy);
      int x = (x1 + 1) << 16;
      for (int y = y1; y >= y2; y--) {
        const int xx = x >> 16;
        int i = (screenx * y) + xx;
        draw_pixels(n, buffs, cols, i);
        if (xx < (screenx - 1)) {
          i--;
          /* DRAWMETHOD; */
        }
        x += dx;
      }
      return;
    }
    /* shallow */
    else {
      dy = ((dy << 16) / dx);
      int y = y1 << 16;
      for (int x = x1; x <= x2; x++) {
        const int yy = y >> 16;
        int i = (screenx * yy) + x;
        draw_pixels(n, buffs, cols, i);
        if (yy < (screeny - 1)) {
          i += screeny;
          /* DRAWMETHOD; */
        }
        y += dy;
      }
      return;
    }
  }
}
