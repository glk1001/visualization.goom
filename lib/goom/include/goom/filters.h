#ifndef VISUALIZATION_GOOM_FILTERS_H
#define VISUALIZATION_GOOM_FILTERS_H

#include "goom_visual_fx.h"
#include "goomutils/mathutils.h"

#include <cstdint>
#include <memory>
#include <string>

namespace GOOM
{

class ImageDisplacement;

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

// 128 = vitesse nule...
// 256 = en arriere
//   hyper vite.. * * 0 = en avant hype vite.
// 128 = zero speed
// 256 = reverse
//   super fast ... 0 = forward quickly.
class Vitesse
{
  static constexpr int32_t MAX_VITESSE = 128;

public:
  static constexpr int32_t STOP_SPEED = MAX_VITESSE;
  static constexpr int32_t FASTEST_SPEED = 0;
  static constexpr int32_t DEFAULT_VITESSE = 127;

  auto GetVitesse() const -> int32_t { return vitesse; };
  void SetVitesse(const int32_t val);
  void SetDefault();
  void GoSlowerBy(const int32_t val);
  void GoFasterBy(const int32_t val);

  auto GetReverseVitesse() const -> bool { return reverseVitesse; }
  void SetReverseVitesse(const bool val) { reverseVitesse = val; }
  void ToggleReverseVitesse() { reverseVitesse = !reverseVitesse; }

  auto GetRelativeSpeed() const -> float;

private:
  int32_t vitesse = DEFAULT_VITESSE;
  bool reverseVitesse = true;
};

inline void Vitesse::SetVitesse(const int32_t val)
{
  vitesse = stdnew::clamp(val, FASTEST_SPEED, STOP_SPEED);
}

inline void Vitesse::SetDefault()
{
  vitesse = DEFAULT_VITESSE;
  reverseVitesse = true;
}

inline void Vitesse::GoSlowerBy(const int32_t val)
{
  SetVitesse(vitesse + val);
}

inline void Vitesse::GoFasterBy(const int32_t val)
{
  SetVitesse(vitesse - 30 * val);
}

inline auto Vitesse::GetRelativeSpeed() const -> float
{
  const float speed = static_cast<float>(vitesse - MAX_VITESSE) / static_cast<float>(MAX_VITESSE);
  return reverseVitesse ? -speed : +speed;
}

struct ZoomFilterData
{
  ZoomFilterMode mode = ZoomFilterMode::NORMAL_MODE;
  Vitesse vitesse{};
#if __cplusplus <= 201402L
  static const uint8_t pertedec; // NEVER SEEMS TO CHANGE
#else
  static constexpr uint8_t pertedec = 8; // NEVER SEEMS TO CHANGE
#endif
  uint32_t middleX = 16;
  uint32_t middleY = 1; // milieu de l'effet
  bool tanEffect = false;
  bool blockyWavy = false;
  static constexpr float MIN_ROTATE_SPEED = -0.5;
  static constexpr float MAX_ROTATE_SPEED = +0.5;
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
  std::shared_ptr<ImageDisplacement> imageDisplacement{};
  static constexpr float MIN_IMAGE_DISPL_COLOR_CUTOFF = 0.1;
  static constexpr float MAX_IMAGE_DISPL_COLOR_CUTOFF = 0.9;
  static constexpr float DEFAULT_IMAGE_DISPL_COLOR_CUTOFF = 0.5;
  float imageDisplacementXColorCutoff = DEFAULT_IMAGE_DISPL_COLOR_CUTOFF;
  float imageDisplacementYColorCutoff = DEFAULT_IMAGE_DISPL_COLOR_CUTOFF;
  static constexpr float MIN_IMAGE_DISPL_ZOOM_FACTOR = 1.00;
  static constexpr float MAX_IMAGE_DISPL_ZOOM_FACTOR = 10.0;
  static constexpr float DEFAULT_IMAGE_DISPL_ZOOM_FACTOR = 5.0;
  float imageDisplacementZoomFactor = DEFAULT_IMAGE_DISPL_ZOOM_FACTOR;

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

  // Y Only
  enum class YOnlyEffect
  {
    NONE,
    XSIN_YSIN,
    XSIN_YCOS,
    XCOS_YSIN,
    XCOS_YCOS,
    _SIZE
  };

  YOnlyEffect yOnlyEffect = YOnlyEffect::XSIN_YSIN;

  static constexpr float DEFAULT_Y_ONLY_X_FREQ_FACTOR = 1.0;
  static constexpr float MIN_Y_ONLY_X_FREQ_FACTOR = -10.0;
  static constexpr float MAX_Y_ONLY_X_FREQ_FACTOR = +10.0;
  float yOnlyXFreqFactor = DEFAULT_Y_ONLY_X_FREQ_FACTOR;

  static constexpr float DEFAULT_Y_ONLY_FREQ_FACTOR = 10.0;
  static constexpr float MIN_Y_ONLY_FREQ_FACTOR = -10.0;
  static constexpr float MAX_Y_ONLY_FREQ_FACTOR = +10.0;
  float yOnlyFreqFactor = DEFAULT_Y_ONLY_FREQ_FACTOR;

  static constexpr float DEFAULT_Y_ONLY_AMPLITUDE = 10.0;
  static constexpr float MIN_Y_ONLY_AMPLITUDE = 0.1;
  static constexpr float MAX_Y_ONLY_AMPLITUDE = 20.0;
  float yOnlyAmplitude = DEFAULT_Y_ONLY_AMPLITUDE;
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
