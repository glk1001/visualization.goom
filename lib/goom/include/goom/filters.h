#ifndef _FILTERS_H
#define _FILTERS_H

#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>
#include <string>

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
  ZoomFilterMode mode = ZoomFilterMode::normalMode; // type d'effet à appliquer
  // 128 = vitesse nule... * * 256 = en arriere
  //   hyper vite.. * * 0 = en avant hype vite.
  int32_t vitesse = 127;
  static constexpr uint8_t pertedec = 8; // NEVER SEEMS TO CHANGE
  uint16_t middleX = 16;
  uint16_t middleY = 1; // milieu de l'effet
  bool reverse = true; // inverse la vitesse

  // @since June 2001
  int hPlaneEffect = 0; // deviation horitontale
  int vPlaneEffect = 0; // deviation verticale

  //* @since April 2002
  bool waveEffect = false; // applique une "surcouche" de wave effect
  bool hypercosEffect = false; // applique une "surcouche de hypercos effect

  bool noisify = false; // ajoute un bruit a la transformation
  double noiseFactor = 1; // in range [0, 1]

  bool blockyWavy = false;

  static constexpr float defaultWaveFreqFactor = 20;
  static constexpr float minWaveFreqFactor = 1;
  static constexpr float maxWaveFreqFactor = 50;
  float waveFreqFactor = defaultWaveFreqFactor;

  static constexpr float defaultWaveAmplitude = 0.01;
  static constexpr float minWaveAmplitude = 0.001;
  static constexpr float maxWaveAmplitude = 0.1;
  float waveAmplitude = defaultWaveAmplitude;

  enum class WaveEffect
  {
    waveSinEffect,
    waveCosEffect,
    waveSinCosEffect
  };
  static constexpr WaveEffect defaultWaveEffectType = WaveEffect::waveSinEffect;
  WaveEffect waveEffectType = defaultWaveEffectType;

  static constexpr float defaultScrunchAmplitude = 0.1;
  static constexpr float minScrunchAmplitude = 0.05;
  static constexpr float maxScrunchAmplitude = 0.2;
  float scrunchAmplitude = defaultScrunchAmplitude;

  static constexpr float defaultSpeedwayAmplitude = 4;
  static constexpr float minSpeedwayAmplitude = 1;
  static constexpr float maxSpeedwayAmplitude = 8;
  float speedwayAmplitude = defaultSpeedwayAmplitude;

  static constexpr float defaultAmuletteAmplitude = 3.5;
  static constexpr float minAmuletteAmplitude = 2;
  static constexpr float maxAmuletteAmplitude = 5;
  float amuletteAmplitude = defaultAmuletteAmplitude;

  static constexpr float defaultCrystalBallAmplitude = 1.0 / 15.0;
  static constexpr float minCrystalBallAmplitude = 0.05;
  static constexpr float maxCrystalBallAmplitude = 0.1;
  float crystalBallAmplitude = defaultCrystalBallAmplitude;

  static constexpr float defaultHypercosFreq = 10;
  static constexpr float minHypercosFreq = 5;
  static constexpr float maxHypercosFreq = 15;
  float hypercosFreq = defaultHypercosFreq;

  static constexpr float defaultHypercosAmplitude = 1.0 / 120.0;
  static constexpr float minHypercosAmplitude = 1.0f / 140.0;
  static constexpr float maxHypercosAmplitude = 1.0f / 100.0;
  float hypercosAmplitude = defaultHypercosAmplitude;

  static constexpr float defaultHPlaneEffectAmplitude = 0.0025;
  static constexpr float minHPlaneEffectAmplitude = 0.0015;
  static constexpr float maxHPlaneEffectAmplitude = 0.0035;
  float hPlaneEffectAmplitude = defaultHPlaneEffectAmplitude;

  static constexpr float defaultVPlaneEffectAmplitude = 0.0025;
  static constexpr float minVPlaneEffectAmplitude = 0.0015;
  static constexpr float maxVPlaneEffectAmplitude = 0.0035;
  float vPlaneEffectAmplitude = defaultVPlaneEffectAmplitude;

  template<class Archive>
  void serialize(Archive&);
};

/* filtre de zoom :
 * le contenu de pix1 est copie dans pix2.
 * zf : si non NULL, configure l'effet.
 * resx,resy : taille des buffers.
 */

struct PluginInfo;
struct ZoomFilterFxData;

class ZoomFilterFx : public VisualFx
{
public:
  ZoomFilterFx() = delete;
  explicit ZoomFilterFx(PluginInfo*);
  ~ZoomFilterFx() noexcept;

  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void apply(Pixel* prevBuff, Pixel* currentBuff) override;

  std::string getFxName() const override;
  void saveState(std::ostream&) override;
  void loadState(std::istream&) override;

  void log(const StatsLogValueFunc& logVal) const override;
  void finish() override;

  void zoomFilterFastRGB(Pixel* pix1,
                         Pixel* pix2,
                         const ZoomFilterData* zf,
                         const uint16_t resx,
                         const uint16_t resy,
                         const int switchIncr,
                         const float switchMult);

private:
  bool enabled = true;
  PluginInfo* const goomInfo;
  std::unique_ptr<ZoomFilterFxData> fxData;
};

} // namespace goom
#endif
