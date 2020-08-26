#ifndef _FILTERS_H
#define _FILTERS_H

#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <cstdint>

enum class ZoomFilterMode {
  NORMAL_MODE = 0,
  WAVE_MODE,
  CRYSTAL_BALL_MODE,
  SCRUNCH_MODE,
  AMULETTE_MODE,
  WATER_MODE,
  HYPERCOS1_MODE,
  HYPERCOS2_MODE,
  YONLY_MODE,
  SPEEDWAY_MODE,
  _size // last - gives num enums
};

struct ZoomFilterData
{
  int vitesse; // 128 = vitesse nule... * * 256 = en arriere
	             // hyper vite.. * * 0 = en avant hype vite.
  uint8_t pertedec;
  uint16_t middleX;
  uint16_t middleY; // milieu de l'effet
  bool reverse; // inverse la vitesse
  ZoomFilterMode mode; // type d'effet à appliquer (cf les #define)
  // @since June 2001
  int hPlaneEffect; // deviation horitontale
  int vPlaneEffect; // deviation verticale
  //* @since April 2002
  int waveEffect; // applique une "surcouche" de wave effect
  int hypercosEffect; // applique une "surcouche de hypercos effect

  uint8_t noisify; // ajoute un bruit a la transformation
};

VisualFX zoomFilterVisualFXWrapper_create(void);

/* filtre de zoom :
 * le contenu de pix1 est copie dans pix2.
 * zf : si non NULL, configure l'effet.
 * resx,resy : taille des buffers.
 */
struct PluginInfo;

void zoomFilterFastRGB(PluginInfo* goomInfo,
                       Pixel* pix1,
                       Pixel* pix2,
                       ZoomFilterData* zf,
                       const uint16_t resx,
                       const uint16_t resy,
                       const int switchIncr,
                       const float switchMult);

#endif
