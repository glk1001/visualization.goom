#ifndef _FILTERS_H
#define _FILTERS_H

#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <cstdint>

enum class ZoomFilterMode
{
  normalMode = 0,
  waveMode,
  crystalBallMode,
  scrunchMode,
  amuletteMode,
  waterMode,
  hyperCos1Mode,
  hyperCos2Mode,
  yOnlyMode,
  speedwayMode,
  _size // must be last - gives number of enums
};

struct ZoomFilterData
{
  ZoomFilterMode mode; // type d'effet à appliquer
  // 128 = vitesse nule... * * 256 = en arriere
  //   hyper vite.. * * 0 = en avant hype vite.
  int32_t vitesse;
  uint8_t pertedec;
  uint16_t middleX;
  uint16_t middleY; // milieu de l'effet
  bool reverse; // inverse la vitesse

  // @since June 2001
  int hPlaneEffect; // deviation horitontale
  int vPlaneEffect; // deviation verticale

  //* @since April 2002
  bool waveEffect; // applique une "surcouche" de wave effect
  bool hypercosEffect; // applique une "surcouche de hypercos effect

  bool noisify; // ajoute un bruit a la transformation
  float noiseFactor; // in range [0, 1]
};

VisualFX zoomFilterVisualFXWrapper_create();

/* filtre de zoom :
 * le contenu de pix1 est copie dans pix2.
 * zf : si non NULL, configure l'effet.
 * resx,resy : taille des buffers.
 */
struct PluginInfo;

void zoomFilterInitData(PluginInfo*);

void zoomFilterFastRGB(PluginInfo*,
                       Pixel* pix1,
                       Pixel* pix2,
                       ZoomFilterData*,
                       const uint16_t resx,
                       const uint16_t resy,
                       const int switchIncr,
                       const float switchMult);

#endif