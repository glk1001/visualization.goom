#ifndef _FILTERS_H
#define _FILTERS_H

#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <cstdint>

namespace goom
{

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
  uint8_t pertedec; // NEVER SEEMS TO CHANGE
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
  double noiseFactor; // in range [0, 1]

  bool blockyWavy;

  float waveFreqFactor;
  static constexpr float defaultWaveFreqFactor = 20;
  static constexpr float minWaveFreqFactor = 1;
  static constexpr float maxWaveFreqFactor = 50;

  float waveAmplitude;
  static constexpr float defaultWaveAmplitude = 0.01;
  static constexpr float minWaveAmplitude = 0.001;
  static constexpr float maxWaveAmplitude = 0.1;

  enum class WaveEffect
  {
    waveSinEffect,
    waveCosEffect,
    waveSinCosEffect
  };
  WaveEffect waveEffectType;
  static constexpr WaveEffect defaultWaveEffectType = WaveEffect::waveSinEffect;

  float scrunchAmplitude;
  static constexpr float defaultScrunchAmplitude = 0.1;
  static constexpr float minScrunchAmplitude = 0.05;
  static constexpr float maxScrunchAmplitude = 0.2;

  float speedwayAmplitude;
  static constexpr float defaultSpeedwayAmplitude = 4;
  static constexpr float minSpeedwayAmplitude = 1;
  static constexpr float maxSpeedwayAmplitude = 8;

  float amuletteAmplitude;
  static constexpr float defaultAmuletteAmplitude = 3.5;
  static constexpr float minAmuletteAmplitude = 2;
  static constexpr float maxAmuletteAmplitude = 5;

  float crystalBallAmplitude;
  static constexpr float defaultCrystalBallAmplitude = 1.0 / 15.0;
  static constexpr float minCrystalBallAmplitude = 0.05;
  static constexpr float maxCrystalBallAmplitude = 0.1;

  float hypercosFreq;
  static constexpr float defaultHypercosFreq = 10;
  static constexpr float minHypercosFreq = 5;
  static constexpr float maxHypercosFreq = 15;

  float hypercosAmplitude;
  static constexpr float defaultHypercosAmplitude = 1.0 / 120.0;
  static constexpr float minHypercosAmplitude = 1.0f / 140.0;
  static constexpr float maxHypercosAmplitude = 1.0f / 100.0;

  float hPlaneEffectAmplitude;
  static constexpr float defaultHPlaneEffectAmplitude = 0.0025;
  static constexpr float minHPlaneEffectAmplitude = 0.0015;
  static constexpr float maxHPlaneEffectAmplitude = 0.0035;

  float vPlaneEffectAmplitude;
  static constexpr float defaultVPlaneEffectAmplitude = 0.0025;
  static constexpr float minVPlaneEffectAmplitude = 0.0015;
  static constexpr float maxVPlaneEffectAmplitude = 0.0035;
};

VisualFX zoomFilterVisualFXWrapper_create();

/* filtre de zoom :
 * le contenu de pix1 est copie dans pix2.
 * zf : si non NULL, configure l'effet.
 * resx,resy : taille des buffers.
 */
struct PluginInfo;

void zoomFilterFastRGB(PluginInfo*,
                       Pixel* pix1,
                       Pixel* pix2,
                       ZoomFilterData*,
                       const uint16_t resx,
                       const uint16_t resy,
                       const int switchIncr,
                       const float switchMult);

void filter_log_stats(VisualFX* _this, const StatsLogValueFunc);

} // namespace goom
#endif
