#ifndef VISUALIZATION_GOOM_FILTERS_H
#define VISUALIZATION_GOOM_FILTERS_H

#include "goom_visual_fx.h"

#include <cstdint>
#include <memory>
#include <string>

namespace GOOM
{

class PixelBuffer;

enum class ZoomFilterMode
{
  _NULL = -1,
  NORMAL_MODE = 0,
  WAVE_MODE,
  CRYSTAL_BALL_MODE,
  SCRUNCH_MODE,
  AMULET_MODE,
  WATER_MODE,
  HYPERCOS1_MODE,
  HYPERCOS2_MODE,
  Y_ONLY_MODE,
  SPEEDWAY_MODE,
  _SIZE // must be last - gives number of enums
};

struct ZoomFilterData
{
  ZoomFilterMode mode = ZoomFilterMode::NORMAL_MODE; // type d'effet à appliquer
  // 128 = vitesse nule... * * 256 = en arriere
  //   hyper vite.. * * 0 = en avant hype vite.
  static constexpr int32_t MAX_VITESSE = 128;
  static constexpr int32_t DEFAULT_VITESSE = 127;
  int32_t vitesse = DEFAULT_VITESSE;
  static constexpr float MIN_COEFF_VITESSE_DENOMINATOR = 30.0;
  static constexpr float MAX_COEFF_VITESSE_DENOMINATOR = 50.0;
  static constexpr float DEFAULT_COEFF_VITESSE_DENOMINATOR = 50.0;
  float coeffVitesseDenominator = DEFAULT_COEFF_VITESSE_DENOMINATOR;
  static constexpr uint8_t pertedec = 8; // NEVER SEEMS TO CHANGE
  uint32_t middleX = 16;
  uint32_t middleY = 1; // milieu de l'effet
  bool reverse = true; // inverse la vitesse
  bool tanEffect = false;
  static constexpr float MAX_ROTATE_SPEED = 0.9;
  static constexpr float MIN_ROTATE_SPEED = 0.3;
  static constexpr float DEFAULT_ROTATE_SPEED = 0.1;
  float rotateSpeed = DEFAULT_ROTATE_SPEED;

  // @since June 2001
  int hPlaneEffect = 0; // deviation horitontale
  int vPlaneEffect = 0; // deviation verticale

  //* @since April 2002
  bool waveEffect = false; // applique une "surcouche" de wave effect

  enum class HypercosEffect
  {
    NONE,
    SIN_CURL_SWIRL,
    COS_CURL_SWIRL,
    SIN_COS_CURL_SWIRL,
    COS_SIN_CURL_SWIRL,
    SIN_TAN_CURL_SWIRL,
    COS_TAN_CURL_SWIRL,
    SIN_RECTANGULAR,
    COS_RECTANGULAR,
    _SIZE
  };
  // applique une surcouche de hypercos effect
  // applies an overlay of hypercos effect
  HypercosEffect hypercosEffect = HypercosEffect::NONE;

  bool noisify = false; // ajoute un bruit a la transformation
  float noiseFactor = 1; // in range [0, 1]

  bool blockyWavy = false;

  static constexpr float DEFAULT_WAVE_FREQ_FACTOR = 20;
  static constexpr float MIN_WAVE_FREQ_FACTOR = 1;
  static constexpr float MAX_WAVE_FREQ_FACTOR = 50;
  static constexpr float MIN_WAVE_SMALL_FREQ_FACTOR = 0.001;
  static constexpr float MAX_WAVE_SMALL_FREQ_FACTOR = 0.1;
  float waveFreqFactor = DEFAULT_WAVE_FREQ_FACTOR;

  static constexpr float DEFAULT_WAVE_AMPLITUDE = 0.01;
  static constexpr float MIN_WAVE_AMPLITUDE = 0.001;
  static constexpr float MAX_WAVE_AMPLITUDE = 0.1;
  static constexpr float MIN_LARGE_WAVE_AMPLITUDE = 1;
  static constexpr float MAX_LARGE_WAVE_AMPLITUDE = 50;
  float waveAmplitude = DEFAULT_WAVE_AMPLITUDE;

  enum class WaveEffect
  {
    WAVE_SIN_EFFECT,
    WAVE_COS_EFFECT,
    WAVE_SIN_COS_EFFECT
  };
  static constexpr WaveEffect DEFAULT_WAVE_EFFECT_TYPE = WaveEffect::WAVE_SIN_EFFECT;
  WaveEffect waveEffectType = DEFAULT_WAVE_EFFECT_TYPE;

  static constexpr float DEFAULT_SCRUNCH_AMPLITUDE = 0.1;
  static constexpr float MIN_SCRUNCH_AMPLITUDE = 0.05;
  static constexpr float MAX_SCRUNCH_AMPLITUDE = 0.2;
  float scrunchAmplitude = DEFAULT_SCRUNCH_AMPLITUDE;

  static constexpr float DEFAULT_SPEEDWAY_AMPLITUDE = 4;
  static constexpr float MIN_SPEEDWAY_AMPLITUDE = 1;
  static constexpr float MAX_SPEEDWAY_AMPLITUDE = 8;
  float speedwayAmplitude = DEFAULT_SPEEDWAY_AMPLITUDE;

  static constexpr float DEFAULT_AMULET_AMPLITUDE = 3.5;
  static constexpr float MIN_AMULET_AMPLITUDE = 2;
  static constexpr float MAX_AMULET_AMPLITUDE = 5;
  float amuletAmplitude = DEFAULT_AMULET_AMPLITUDE;

  static constexpr float DEFAULT_CRYSTAL_BALL_AMPLITUDE = 1.0 / 15.0;
  static constexpr float MIN_CRYSTAL_BALL_AMPLITUDE = 0.05;
  static constexpr float MAX_CRYSTAL_BALL_AMPLITUDE = 0.1;
  float crystalBallAmplitude = DEFAULT_CRYSTAL_BALL_AMPLITUDE;

  static constexpr float DEFAULT_HYPERCOS_FREQ = 10;
  static constexpr float MIN_HYPERCOS_FREQ = 5;
  static constexpr float MAX_HYPERCOS_FREQ = 15;
  float hypercosFreqX = DEFAULT_HYPERCOS_FREQ;
  float hypercosFreqY = DEFAULT_HYPERCOS_FREQ;

  static constexpr float DEFAULT_HYPERCOS_AMPLITUDE = 1.0 / 120.0;
  static constexpr float MIN_HYPERCOS_AMPLITUDE = 1.0F / 140.0;
  static constexpr float MAX_HYPERCOS_AMPLITUDE = 1.0F / 100.0;
  float hypercosAmplitudeX = DEFAULT_HYPERCOS_AMPLITUDE;
  float hypercosAmplitudeY = DEFAULT_HYPERCOS_AMPLITUDE;
  bool hypercosReverse = false;

  static constexpr float DEFAULT_H_PLANE_EFFECT_AMPLITUDE = 0.0025;
  static constexpr float MIN_H_PLANE_EFFECT_AMPLITUDE = 0.0015;
  static constexpr float MAX_H_PLANE_EFFECT_AMPLITUDE = 0.0035;
  float hPlaneEffectAmplitude = DEFAULT_H_PLANE_EFFECT_AMPLITUDE;

  static constexpr float DEFAULT_V_PLANE_EFFECT_AMPLITUDE = 0.0025;
  static constexpr float MIN_V_PLANE_EFFECT_AMPLITUDE = 0.0015;
  static constexpr float MAX_V_PLANE_EFFECT_AMPLITUDE = 0.0035;
  float vPlaneEffectAmplitude = DEFAULT_V_PLANE_EFFECT_AMPLITUDE;
};

/* filtre de zoom :
 * le contenu de pix1 est copie dans pix2.
 * zf : si non NULL, configure l'effet.
 * resx,resy : taille des buffers.
 */

namespace UTILS
{
class Parallel;
} // namespace UTILS

class PluginInfo;

class ZoomFilterFx : public IVisualFx
{
public:
  ZoomFilterFx() noexcept = delete;
  ZoomFilterFx(UTILS::Parallel&, const std::shared_ptr<const PluginInfo>&) noexcept;
  ~ZoomFilterFx() noexcept override;
  ZoomFilterFx(const ZoomFilterFx&) noexcept = delete;
  ZoomFilterFx(ZoomFilterFx&&) noexcept = delete;
  auto operator=(const ZoomFilterFx&) -> ZoomFilterFx& = delete;
  auto operator=(ZoomFilterFx&&) -> ZoomFilterFx& = delete;

  void ZoomFilterFastRgb(const PixelBuffer& pix1,
                         PixelBuffer& pix2,
                         const ZoomFilterData* zf,
                         int switchIncr,
                         float switchMult,
                         uint32_t& numClipped);

  [[nodiscard]] auto GetResourcesDirectory() const -> const std::string& override;
  void SetResourcesDirectory(const std::string& dirName) override;

  [[nodiscard]] auto GetFxName() const -> std::string override;
  void SetBuffSettings(const FXBuffSettings& settings) override;

  void Start() override;

  auto GetFilterData() const -> const ZoomFilterData&;
  auto GetGeneralSpeed() const -> float;
  auto GetTranBuffYLineStart() const -> uint32_t;

  void Log(const StatsLogValueFunc& l) const override;
  void Finish() override;

private:
  bool m_enabled = true;
  class ZoomFilterImpl;
  const std::unique_ptr<ZoomFilterImpl> m_fxImpl;
};

} // namespace GOOM
#endif
