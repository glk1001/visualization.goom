#ifndef _FILTERS_H
#define _FILTERS_H

#include "goom_visual_fx.h"

#include <cereal/access.hpp>
#include <cstdint>
#include <memory>
#include <string>

namespace goom
{

class PixelBuffer;

enum class ZoomFilterMode
{
  _null = -1,
  normalMode = 0,
  waveMode,
  crystalBallMode,
  scrunchMode,
  amuletMode,
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
  uint32_t middleX = 16;
  uint32_t middleY = 1; // milieu de l'effet
  bool reverse = true; // inverse la vitesse

  // @since June 2001
  int hPlaneEffect = 0; // deviation horitontale
  int vPlaneEffect = 0; // deviation verticale

  //* @since April 2002
  bool waveEffect = false; // applique une "surcouche" de wave effect

  enum class HypercosEffect
  {
    none,
    sinCurlSwirl,
    cosCurlSwirl,
    sinRectangular,
    cosRectangular,
    _size
  };
  // applique une surcouche de hypercos effect
  // applies an overlay of hypercos effect
  HypercosEffect hypercosEffect = HypercosEffect::none;

  bool noisify = false; // ajoute un bruit a la transformation
  double noiseFactor = 1; // in range [0, 1]

  bool blockyWavy = false;

  static constexpr float defaultWaveFreqFactor = 20;
  static constexpr float minWaveFreqFactor = 1;
  static constexpr float maxWaveFreqFactor = 50;
  static constexpr float minWaveSmallFreqFactor = 0.001;
  static constexpr float maxWaveSmallFreqFactor = 0.1;
  float waveFreqFactor = defaultWaveFreqFactor;

  static constexpr float defaultWaveAmplitude = 0.01;
  static constexpr float minWaveAmplitude = 0.001;
  static constexpr float maxWaveAmplitude = 0.1;
  static constexpr float minLargeWaveAmplitude = 1;
  static constexpr float maxLargeWaveAmplitude = 50;
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

  bool operator==(const ZoomFilterData&) const = default;

  template<class Archive>
  void serialize(Archive&);
};

/* filtre de zoom :
 * le contenu de pix1 est copie dans pix2.
 * zf : si non NULL, configure l'effet.
 * resx,resy : taille des buffers.
 */

namespace utils
{
class Parallel;
}

class PluginInfo;

class ZoomFilterFx : public IVisualFx
{
public:
  ZoomFilterFx() noexcept;
  explicit ZoomFilterFx(utils::Parallel&, const std::shared_ptr<const PluginInfo>&) noexcept;
  ~ZoomFilterFx() noexcept override;
  ZoomFilterFx(const ZoomFilterFx&) = delete;
  ZoomFilterFx& operator=(const ZoomFilterFx&) = delete;

  void zoomFilterFastRGB(const PixelBuffer& pix1,
                         PixelBuffer& pix2,
                         const ZoomFilterData* zf,
                         int switchIncr,
                         float switchMult);

  [[nodiscard]] std::string getFxName() const override;
  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void apply(PixelBuffer&) override;
  void apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff) override;

  void log(const StatsLogValueFunc&) const override;
  void finish() override;

  bool operator==(const ZoomFilterFx&) const;

private:
  bool enabled = true;
  class ZoomFilterImpl;
  std::unique_ptr<ZoomFilterImpl> fxImpl;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace goom
#endif
