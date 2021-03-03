#ifndef VISUALIZATION_GOOM_FILTERS_H
#define VISUALIZATION_GOOM_FILTERS_H

#include "goom_visual_fx.h"

#include <cstdint>
#include <memory>
#include <string>

namespace GOOM
{

enum class ZoomFilterMode
{
  _NULL = -1,
  AMULET_MODE = 0,
  CRYSTAL_BALL_MODE,
  HYPERCOS0_MODE,
  HYPERCOS1_MODE,
  HYPERCOS2_MODE,
  IMAGE_DISPLACEMENT_MODE,
  NORMAL_MODE,
  SCRUNCH_MODE,
  SPEEDWAY_MODE,
  WATER_MODE,
  WAVE_MODE0,
  WAVE_MODE1,
  Y_ONLY_MODE,
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
  static constexpr float MIN_COEFF_VITESSE_DENOMINATOR = 50.0;
  static constexpr float MAX_COEFF_VITESSE_DENOMINATOR = 50.01;
  static constexpr float DEFAULT_COEFF_VITESSE_DENOMINATOR = 50.0;
  float coeffVitesseDenominator = DEFAULT_COEFF_VITESSE_DENOMINATOR;
  static constexpr float MIN_COEF_VITESSE = -4.01;
  static constexpr float MAX_MAX_COEF_VITESSE = +4.01;
  static constexpr float DEFAULT_MAX_COEF_VITESSE = +2.01;
#if __cplusplus <= 201402L
  static const uint8_t pertedec; // NEVER SEEMS TO CHANGE
#else
  static constexpr uint8_t pertedec = 8; // NEVER SEEMS TO CHANGE
#endif
  uint32_t middleX = 16;
  uint32_t middleY = 1; // milieu de l'effet
  bool reverseSpeed = true; // inverse la vitesse
  bool tanEffect = false;
  bool blockyWavy = false;
  static constexpr float MAX_ROTATE_SPEED = +0.5;
  static constexpr float MIN_ROTATE_SPEED = -0.5;
  static constexpr float DEFAULT_ROTATE_SPEED = 0.0;
  float rotateSpeed = DEFAULT_ROTATE_SPEED;

  //* @since April 2002
  // TODO - Not used yet
  bool waveEffect = false; // applique une "surcouche" de wave effect

  // Noise:
  bool noisify = false; // ajoute un bruit a la transformation
  float noiseFactor = 1; // in range [0, 1]
  // For noise amplitude, take the reciprocal of these.
  static constexpr float NOISE_MIN = 70;
  static constexpr float NOISE_MAX = 120;

  // H Plane:
  // @since June 2001
  int hPlaneEffect = 0; // deviation horitontale
  static constexpr float DEFAULT_H_PLANE_EFFECT_AMPLITUDE = 0.0025;
  static constexpr float MIN_H_PLANE_EFFECT_AMPLITUDE = 0.0015;
  static constexpr float MAX_H_PLANE_EFFECT_AMPLITUDE = 0.0035;
  float hPlaneEffectAmplitude = DEFAULT_H_PLANE_EFFECT_AMPLITUDE;

  // V Plane:
  int vPlaneEffect = 0; // deviation verticale
  static constexpr float DEFAULT_V_PLANE_EFFECT_AMPLITUDE = 0.0025;
  static constexpr float MIN_V_PLANE_EFFECT_AMPLITUDE = 0.0015;
  static constexpr float MAX_V_PLANE_EFFECT_AMPLITUDE = 0.0035;
  float vPlaneEffectAmplitude = DEFAULT_V_PLANE_EFFECT_AMPLITUDE;

  // Amulet:
  static constexpr float DEFAULT_AMULET_AMPLITUDE = 3.5;
  static constexpr float MIN_AMULET_AMPLITUDE = 2;
  static constexpr float MAX_AMULET_AMPLITUDE = 5;
  float amuletAmplitude = DEFAULT_AMULET_AMPLITUDE;

  // Crystal ball:
  static constexpr float DEFAULT_CRYSTAL_BALL_AMPLITUDE = 0.1;
  static constexpr float MIN_CRYSTAL_BALL_AMPLITUDE = 0.05;
  static constexpr float MAX_CRYSTAL_BALL_AMPLITUDE = 2.0;
  float crystalBallAmplitude = DEFAULT_CRYSTAL_BALL_AMPLITUDE;
  static constexpr float DEFAULT_CRYSTAL_BALL_SQ_DIST_OFFSET = 0.3;
  static constexpr float MIN_CRYSTAL_BALL_SQ_DIST_OFFSET = 0.1;
  static constexpr float MAX_CRYSTAL_BALL_SQ_DIST_OFFSET = 1.5;
  float crystalBallSqDistOffset = DEFAULT_CRYSTAL_BALL_SQ_DIST_OFFSET;

  // Hypercos:
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
  bool hypercosReverse = false;

  static constexpr float DEFAULT_HYPERCOS_FREQ = 10;
  static constexpr float MIN_HYPERCOS_FREQ = 1;
  static constexpr float MAX_HYPERCOS_FREQ = 100;
  // Tried 1000 for hypercos_freq but effect was too fine and annoying.
  static constexpr float BIG_MAX_HYPERCOS_FREQ = 500;
  float hypercosFreqX = DEFAULT_HYPERCOS_FREQ;
  float hypercosFreqY = DEFAULT_HYPERCOS_FREQ;

  static constexpr float DEFAULT_HYPERCOS_AMPLITUDE = 1.0 / 120.0;
  static constexpr float MIN_HYPERCOS_AMPLITUDE = 1.0F / 140.0;
  static constexpr float MAX_HYPERCOS_AMPLITUDE = 1.0F / 100.0;
  float hypercosAmplitudeX = DEFAULT_HYPERCOS_AMPLITUDE;
  float hypercosAmplitudeY = DEFAULT_HYPERCOS_AMPLITUDE;

  // Image Displacement:
  static constexpr float DEFAULT_IMAGE_DISPL_AMPLITUDE = 0.0250;
  static constexpr float MIN_IMAGE_DISPL_AMPLITUDE = 0.0025;
  static constexpr float MAX_IMAGE_DISPL_AMPLITUDE = 0.1000;
  float imageDisplacementAmplitude = DEFAULT_IMAGE_DISPL_AMPLITUDE;
  std::string imageDisplacementFilename{};

  // Wave:
  enum class WaveEffect
  {
    WAVE_SIN_EFFECT,
    WAVE_COS_EFFECT,
    WAVE_SIN_COS_EFFECT
  };
  static constexpr WaveEffect DEFAULT_WAVE_EFFECT_TYPE = WaveEffect::WAVE_SIN_EFFECT;
  WaveEffect waveEffectType = DEFAULT_WAVE_EFFECT_TYPE;

  static constexpr float DEFAULT_WAVE_FREQ_FACTOR = 20;
  static constexpr float MIN_WAVE_FREQ_FACTOR = 1;
  static constexpr float MAX_WAVE_FREQ_FACTOR = 50;
  float waveFreqFactor = DEFAULT_WAVE_FREQ_FACTOR;

  static constexpr float DEFAULT_WAVE_AMPLITUDE = 0.01;
  static constexpr float MIN_WAVE_AMPLITUDE = 0.001;
  static constexpr float MAX_WAVE_AMPLITUDE = 0.25;
  float waveAmplitude = DEFAULT_WAVE_AMPLITUDE;

  // These give weird but interesting wave results
  static constexpr float SMALL_MIN_WAVE_FREQ_FACTOR = 0.001;
  static constexpr float SMALL_MAX_WAVE_FREQ_FACTOR = 0.1;
  static constexpr float BIG_MIN_WAVE_AMPLITUDE = 1;
  static constexpr float BIG_MAX_WAVE_AMPLITUDE = 50;

  // Scrunch:
  static constexpr float DEFAULT_SCRUNCH_AMPLITUDE = 0.1;
  static constexpr float MIN_SCRUNCH_AMPLITUDE = 0.05;
  static constexpr float MAX_SCRUNCH_AMPLITUDE = 0.2;
  float scrunchAmplitude = DEFAULT_SCRUNCH_AMPLITUDE;

  // Speedway:
  static constexpr float DEFAULT_SPEEDWAY_AMPLITUDE = 4;
  static constexpr float MIN_SPEEDWAY_AMPLITUDE = 1;
  static constexpr float MAX_SPEEDWAY_AMPLITUDE = 8;
  float speedwayAmplitude = DEFAULT_SPEEDWAY_AMPLITUDE;
};

namespace UTILS
{
class Parallel;
} // namespace UTILS

class PluginInfo;
class PixelBuffer;

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

  [[nodiscard]] auto GetResourcesDirectory() const -> const std::string& override;
  void SetResourcesDirectory(const std::string& dirName) override;

  [[nodiscard]] auto GetFxName() const -> std::string override;
  void SetBuffSettings(const FXBuffSettings& settings) override;

  void Start() override;

  auto GetFilterSettings() const -> const ZoomFilterData&;
  auto GetFilterSettingsArePending() const -> bool;

  auto GetTranLerpFactor() const -> int32_t;
  auto GetGeneralSpeed() const -> float;
  auto GetTranBuffYLineStart() const -> uint32_t;

  void ChangeFilterSettings(const ZoomFilterData& filterSettings);

  void ZoomFilterFastRgb(const PixelBuffer& pix1,
                         PixelBuffer& pix2,
                         int switchIncr,
                         float switchMult,
                         uint32_t& numClipped);

  void Log(const StatsLogValueFunc& l) const override;
  void Finish() override;

private:
  bool m_enabled = true;
  class ZoomFilterImpl;
  const std::unique_ptr<ZoomFilterImpl> m_fxImpl;
};

} // namespace GOOM
#endif
