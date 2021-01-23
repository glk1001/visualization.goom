#include "flying_stars_fx.h"

#include "goom_draw.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormaps.h"
#include "goomutils/colorutils.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
//#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "goomutils/random_colormaps.h"
#include "goomutils/random_colormaps_manager.h"

#include <array>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cmath>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

CEREAL_REGISTER_TYPE(GOOM::FlyingStarsFx)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GOOM::IVisualFx, GOOM::FlyingStarsFx)

namespace GOOM
{

using namespace GOOM::UTILS;
using UTILS::COLOR_DATA::ColorMapName;

class StarsStats
{
public:
  StarsStats() noexcept = default;

  void Reset();
  void Log(const StatsLogValueFunc& logVal) const;
  void AddBombButTooManyStars();
  void AddBomb();
  void SoundEventOccurred();
  void NoFxChosen();
  void FireworksFxChosen();
  void RainFxChosen();
  void FountainFxChosen();
  void UpdateStars();
  void DeadStar();
  void RemovedDeadStars(uint32_t val);
  void SetLastNumActive(uint32_t val);
  void SetLastMaxStars(uint32_t val);
  void SetLastMaxStarAge(uint32_t val);

private:
  uint32_t m_numAddBombButTooManyStars = 0;
  uint32_t m_numAddBombs = 0;
  uint32_t m_numSoundEvents = 0;
  uint32_t m_numNoFxChosen = 0;
  uint32_t m_numFireworksFxChosen = 0;
  uint32_t m_numRainFxChosen = 0;
  uint32_t m_numFountainFxChosen = 0;
  uint32_t m_numUpdateStars = 0;
  uint32_t m_numDeadStars = 0;
  uint32_t m_numRemovedStars = 0;
  uint32_t m_lastNumActive = 0;
  uint32_t m_lastMaxStars = 0;
  uint32_t m_lastMaxStarAge = 0;
};

void StarsStats::Reset()
{
  m_numAddBombButTooManyStars = 0;
  m_numAddBombs = 0;
  m_numSoundEvents = 0;
  m_numNoFxChosen = 0;
  m_numFireworksFxChosen = 0;
  m_numRainFxChosen = 0;
  m_numFountainFxChosen = 0;
  m_numUpdateStars = 0;
  m_numDeadStars = 0;
  m_numRemovedStars = 0;
}

void StarsStats::Log(const StatsLogValueFunc& logVal) const
{
  const constexpr char* MODULE = "Stars";

  logVal(MODULE, "numUpdateStars", m_numUpdateStars);
  logVal(MODULE, "numSoundEvents", m_numSoundEvents);
  logVal(MODULE, "numAddBombButTooManyStars", m_numAddBombButTooManyStars);
  logVal(MODULE, "numAddBombs", m_numAddBombs);
  logVal(MODULE, "numNoFxChosen", m_numNoFxChosen);
  logVal(MODULE, "numFireworksFxChosen", m_numFireworksFxChosen);
  logVal(MODULE, "numRainFxChosen", m_numRainFxChosen);
  logVal(MODULE, "numFountainFxChosen", m_numFountainFxChosen);
  logVal(MODULE, "numDeadStars", m_numDeadStars);
  logVal(MODULE, "numRemovedStars", m_numRemovedStars);
  logVal(MODULE, "lastNumActive", m_lastNumActive);
  logVal(MODULE, "lastMaxStars", m_lastMaxStars);
  logVal(MODULE, "lastMaxStarAge", m_lastMaxStarAge);
}

inline void StarsStats::UpdateStars()
{
  m_numUpdateStars++;
}

inline void StarsStats::AddBombButTooManyStars()
{
  m_numAddBombButTooManyStars++;
}

inline void StarsStats::AddBomb()
{
  m_numAddBombs++;
}

inline void StarsStats::SoundEventOccurred()
{
  m_numSoundEvents++;
}

inline void StarsStats::NoFxChosen()
{
  m_numNoFxChosen++;
}

inline void StarsStats::FireworksFxChosen()
{
  m_numFireworksFxChosen++;
}

inline void StarsStats::RainFxChosen()
{
  m_numRainFxChosen++;
}

inline void StarsStats::FountainFxChosen()
{
  m_numFountainFxChosen++;
}

inline void StarsStats::DeadStar()
{
  m_numDeadStars++;
}

inline void StarsStats::RemovedDeadStars(uint32_t val)
{
  m_numRemovedStars += val;
}

inline void StarsStats::SetLastNumActive(uint32_t val)
{
  m_lastNumActive = val;
}

inline void StarsStats::SetLastMaxStars(uint32_t val)
{
  m_lastMaxStars = val;
}

inline void StarsStats::SetLastMaxStarAge(uint32_t val)
{
  m_lastMaxStarAge = val;
}


constexpr uint32_t MIN_STAR_AGE = 10;
constexpr uint32_t MAX_STAR_EXTRA_AGE = 40;

/* TODO:-- FAIRE PROPREMENT... BOAH... */

// The different modes of the visual FX.
enum class StarModes
{
  NO_FX = 0,
  FIREWORKS,
  RAIN,
  FOUNTAIN,
  _SIZE // must be last - gives number of enums
};
constexpr size_t NUM_FX = static_cast<uint32_t>(StarModes::_SIZE);

struct Star
{
  float x = 0;
  float y = 0;
  float xVelocity = 0;
  float yVelocity = 0;
  float xAcceleration = 0;
  float yAcceleration = 0;
  float age = 0;
  float vage = 0;
  // TODO: Cereal for these pointers????
  std::shared_ptr<const IColorMap> dominantColormap{};
  std::shared_ptr<const IColorMap> dominantLowColormap{};
  std::shared_ptr<const IColorMap> currentColorMap{};
  std::shared_ptr<const IColorMap> currentLowColorMap{};

  auto operator==(const Star& s) const -> bool
  {
    return x == s.x && y == s.y && xVelocity == s.xVelocity && yVelocity == s.yVelocity &&
           xAcceleration == s.xAcceleration && yAcceleration == s.yAcceleration && age == s.age &&
           vage == s.vage;
  }

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(x, y, xVelocity, yVelocity, xAcceleration, yAcceleration, age, vage);
  }
};

class FlyingStarsFx::FlyingStarsImpl
{
public:
  FlyingStarsImpl() noexcept;
  explicit FlyingStarsImpl(std::shared_ptr<const PluginInfo> goomInfo) noexcept;
  ~FlyingStarsImpl() noexcept = default;
  FlyingStarsImpl(const FlyingStarsImpl&) noexcept = delete;
  FlyingStarsImpl(FlyingStarsImpl&&) noexcept = delete;
  auto operator=(const FlyingStarsImpl&) -> FlyingStarsImpl& = delete;
  auto operator=(FlyingStarsImpl&&) -> FlyingStarsImpl& = delete;

  void SetBuffSettings(const FXBuffSettings& settings);

  void UpdateBuffers(PixelBuffer& currentBuff, PixelBuffer& nextBuff);

  void Finish();
  void Log(const StatsLogValueFunc& logVal) const;

  auto operator==(const FlyingStarsImpl& f) const -> bool;

private:
  std::shared_ptr<const PluginInfo> m_goomInfo{};

  std::shared_ptr<WeightedColorMaps> m_colorMaps{
      std::make_shared<WeightedColorMaps>(Weights<ColorMapGroup>{{
          {ColorMapGroup::PERCEPTUALLY_UNIFORM_SEQUENTIAL, 10},
          {ColorMapGroup::SEQUENTIAL, 10},
          {ColorMapGroup::SEQUENTIAL2, 10},
          {ColorMapGroup::CYCLIC, 0},
          {ColorMapGroup::DIVERGING, 0},
          {ColorMapGroup::DIVERGING_BLACK, 0},
          {ColorMapGroup::QUALITATIVE, 10},
          {ColorMapGroup::MISC, 10},
      }})};
  std::shared_ptr<WeightedColorMaps> m_lowColorMaps{
      std::make_shared<WeightedColorMaps>(Weights<ColorMapGroup>{{
          {ColorMapGroup::PERCEPTUALLY_UNIFORM_SEQUENTIAL, 10},
          {ColorMapGroup::SEQUENTIAL, 0},
          {ColorMapGroup::SEQUENTIAL2, 10},
          {ColorMapGroup::CYCLIC, 5},
          {ColorMapGroup::DIVERGING, 10},
          {ColorMapGroup::DIVERGING_BLACK, 20},
          {ColorMapGroup::QUALITATIVE, 1},
          {ColorMapGroup::MISC, 10},
      }})};
  RandomColorMapsManager m_colorMapsManager{};
  uint32_t m_dominantColorMapID{};
  uint32_t m_dominantLowColorMapID{};
  uint32_t m_colorMapID{};
  uint32_t m_lowColorMapID{};

  ColorMode m_colorMode = ColorMode::mixColors;
  void ChangeColorMode();
  [[nodiscard]] auto GetMixedColors(const Star& s, float t, float brightness)
      -> std::tuple<Pixel, Pixel>;

  uint32_t m_counter = 0;
  static constexpr uint32_t MAX_COUNT = 100;

  StarModes m_fxMode = StarModes::FIREWORKS;
  static constexpr size_t MAX_STARS_LIMIT = 1024;
  size_t m_maxStars = MAX_STARS_LIMIT;
  std::vector<Star> m_stars{};
  uint32_t m_maxStarAge = 15;
  float m_minSideWind = -01;
  float m_maxSideWind = +01;
  float m_minGravity = +0.01;
  float m_maxGravity = +0.09;

  // Fireworks Largest Bombs
  float m_minAge = 1.0F - (99.0F / 100.0F);
  // Fireworks Smallest Bombs
  float m_maxAge = 1.0F - (80.0F / 100.0F);

  FXBuffSettings m_buffSettings{};
  bool m_useSingleBufferOnly = true;
  GoomDraw m_draw{};
  StarsStats m_stats{};

  void SoundEventOccurred();
  static void UpdateStar(Star* s);
  [[nodiscard]] auto IsStarDead(const Star& s) const -> bool;
  void AddABomb(int32_t mx, int32_t my, float radius, float vage, float gravity, float sideWind);
  [[nodiscard]] auto GetBombAngle(float x, float y) const -> uint32_t;

  friend class cereal::access;
  template<class Archive>
  void save(Archive& ar) const;
  template<class Archive>
  void load(Archive& ar);
};

FlyingStarsFx::FlyingStarsFx() noexcept : m_fxImpl{new FlyingStarsImpl{}}
{
}

FlyingStarsFx::FlyingStarsFx(const std::shared_ptr<const PluginInfo>& info) noexcept
  : m_fxImpl{new FlyingStarsImpl{info}}
{
}

FlyingStarsFx::~FlyingStarsFx() noexcept = default;

auto FlyingStarsFx::operator==(const FlyingStarsFx& f) const -> bool
{
  return m_fxImpl->operator==(*f.m_fxImpl);
}

void FlyingStarsFx::SetBuffSettings(const FXBuffSettings& settings)
{
  m_fxImpl->SetBuffSettings(settings);
}

void FlyingStarsFx::Start()
{
}

void FlyingStarsFx::Finish()
{
  m_fxImpl->Finish();
}

void FlyingStarsFx::Log(const StatsLogValueFunc& logVal) const
{
  m_fxImpl->Log(logVal);
}

auto FlyingStarsFx::GetFxName() const -> std::string
{
  return "Flying Stars FX";
}

void FlyingStarsFx::Apply([[maybe_unused]] PixelBuffer& buff)
{
  throw std::logic_error("FlyingStarsFx::Apply should never be called.");
}

void FlyingStarsFx::Apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff)
{
  if (!m_enabled)
  {
    return;
  }

  m_fxImpl->UpdateBuffers(currentBuff, nextBuff);
}

template<class Archive>
void FlyingStarsFx::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_enabled), CEREAL_NVP(m_fxImpl));
}

// Need to explicitly instantiate template functions for serialization.
template void FlyingStarsFx::serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);
template void FlyingStarsFx::serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template void FlyingStarsFx::FlyingStarsImpl::save<cereal::JSONOutputArchive>(
    cereal::JSONOutputArchive&) const;
template void FlyingStarsFx::FlyingStarsImpl::load<cereal::JSONInputArchive>(
    cereal::JSONInputArchive&);

template<class Archive>
void FlyingStarsFx::FlyingStarsImpl::save(Archive& ar) const
{
  ar(CEREAL_NVP(m_goomInfo), CEREAL_NVP(m_fxMode), CEREAL_NVP(m_maxStars), CEREAL_NVP(m_stars),
     CEREAL_NVP(m_maxStarAge), CEREAL_NVP(m_minAge), CEREAL_NVP(m_maxAge),
     CEREAL_NVP(m_buffSettings), CEREAL_NVP(m_useSingleBufferOnly), CEREAL_NVP(m_draw));
}

template<class Archive>
void FlyingStarsFx::FlyingStarsImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(m_goomInfo), CEREAL_NVP(m_fxMode), CEREAL_NVP(m_maxStars), CEREAL_NVP(m_stars),
     CEREAL_NVP(m_maxStarAge), CEREAL_NVP(m_minAge), CEREAL_NVP(m_maxAge),
     CEREAL_NVP(m_buffSettings), CEREAL_NVP(m_useSingleBufferOnly), CEREAL_NVP(m_draw));
}

auto FlyingStarsFx::FlyingStarsImpl::operator==(const FlyingStarsImpl& f) const -> bool
{
  if (m_goomInfo == nullptr && f.m_goomInfo != nullptr)
  {
    return false;
  }
  if (m_goomInfo != nullptr && f.m_goomInfo == nullptr)
  {
    return false;
  }

  return ((m_goomInfo == nullptr && f.m_goomInfo == nullptr) || (*m_goomInfo == *f.m_goomInfo)) &&
         m_fxMode == f.m_fxMode && m_maxStars == f.m_maxStars && m_stars == f.m_stars &&
         m_maxStarAge == f.m_maxStarAge && m_minAge == f.m_minAge && m_maxAge == f.m_maxAge &&
         m_buffSettings == f.m_buffSettings && m_useSingleBufferOnly == f.m_useSingleBufferOnly &&
         m_draw == f.m_draw;
}

FlyingStarsFx::FlyingStarsImpl::FlyingStarsImpl() noexcept = default;

FlyingStarsFx::FlyingStarsImpl::FlyingStarsImpl(std::shared_ptr<const PluginInfo> info) noexcept
  : m_goomInfo{std::move(info)},
    m_draw{m_goomInfo->GetScreenInfo().width, m_goomInfo->GetScreenInfo().height}
{
  m_stars.reserve(MAX_STARS_LIMIT);

  m_dominantColorMapID =
      m_colorMapsManager.AddColorMapInfo({m_colorMaps, ColorMapName::_NULL, RandomColorMaps::ALL});
  m_dominantLowColorMapID = m_colorMapsManager.AddColorMapInfo(
      {m_lowColorMaps, ColorMapName::_NULL, RandomColorMaps::ALL});
  m_colorMapID =
      m_colorMapsManager.AddColorMapInfo({m_colorMaps, ColorMapName::_NULL, RandomColorMaps::ALL});
  m_lowColorMapID = m_colorMapsManager.AddColorMapInfo(
      {m_lowColorMaps, ColorMapName::_NULL, RandomColorMaps::ALL});

  constexpr float MIN_SATURATION = 0.5F;
  constexpr float MAX_SATURATION = 1.0F;
  m_colorMaps->SetSaturationLimts(MIN_SATURATION, MAX_SATURATION);
  m_lowColorMaps->SetSaturationLimts(MIN_SATURATION, MAX_SATURATION);

  constexpr float MIN_LIGHTNESS = 0.5F;
  constexpr float MAX_LIGHTNESS = 1.0F;
  m_colorMaps->SetLightnessLimits(MIN_LIGHTNESS, MAX_LIGHTNESS);
  m_lowColorMaps->SetLightnessLimits(MIN_LIGHTNESS, MAX_LIGHTNESS);
}

void FlyingStarsFx::FlyingStarsImpl::SetBuffSettings(const FXBuffSettings& settings)
{
  m_draw.SetBuffIntensity(settings.buffIntensity);
  m_draw.SetAllowOverexposed(settings.allowOverexposed);
}

void FlyingStarsFx::FlyingStarsImpl::Finish()
{
  m_stats.SetLastNumActive(m_stars.size());
  m_stats.SetLastMaxStars(m_maxStars);
  m_stats.SetLastMaxStarAge(m_maxStarAge);
}

void FlyingStarsFx::FlyingStarsImpl::Log(const StatsLogValueFunc& logVal) const
{
  m_stats.Log(logVal);
}

void FlyingStarsFx::FlyingStarsImpl::UpdateBuffers(PixelBuffer& currentBuff, PixelBuffer& nextBuff)
{
  m_counter++;

  m_maxStars = GetRandInRange(100U, MAX_STARS_LIMIT);

  // look for events
  if (m_stars.empty() || m_goomInfo->GetSoundInfo().GetTimeSinceLastGoom() < 1)
  {
    SoundEventOccurred();
    if (GetNRand(20) == 1)
    {
      // Give a slight weight towards noFx mode by using numFX + 2.
      const uint32_t newVal = GetNRand(NUM_FX + 2);
      m_fxMode = newVal >= NUM_FX ? StarModes::NO_FX : static_cast<StarModes>(newVal);
      ChangeColorMode();
    }
    else if (m_counter > MAX_COUNT)
    {
      m_counter = 0;
      ChangeColorMode();
    }
  }
  // m_fxMode = StarModes::rain;

  // update particules
  m_stats.UpdateStars();

  const float flipSpeed = GetRandInRange(0.1F, 10.0F);

  for (auto& star : m_stars)
  {
    UpdateStar(&star);

    // dead particule
    if (star.age >= static_cast<float>(m_maxStarAge))
    {
      m_stats.DeadStar();
      continue;
    }

    // draws the particule
    constexpr float OLD_AGE = 0.95;
    const float tAge = star.age / static_cast<float>(m_maxStarAge);
    const float ageBrightness = 0.2F + 0.8F * Sq(0.5F - tAge) / 0.25F;
    const size_t numParts =
        tAge > OLD_AGE ? 4 : 2 + static_cast<size_t>(std::lround((1.0F - tAge) * 2.0F));
    const auto x0 = static_cast<int32_t>(star.x);
    const auto y0 = static_cast<int32_t>(star.y);
    int32_t x1 = x0;
    int32_t y1 = y0;
    for (size_t j = 1; j <= numParts; j++)
    {
      const int32_t x2 =
          x0 - static_cast<int32_t>(0.5 * (1.0 + std::sin(flipSpeed * star.xVelocity * j)) *
                                    star.xVelocity * j);
      const int32_t y2 =
          y0 - static_cast<int32_t>(0.5 * (1.0 + std::cos(flipSpeed * star.yVelocity * j)) *
                                    star.yVelocity * j);
      const uint8_t thickness = tAge < OLD_AGE ? 1 : GetRandInRange(2U, 5U);

      const float brightness = ageBrightness * static_cast<float>(j) / static_cast<float>(numParts);
      const auto [mixedColor, mixedLowColor] = GetMixedColors(star, tAge, brightness);

      if (m_useSingleBufferOnly)
      {
        m_draw.Line(currentBuff, x1, y1, x2, y2, mixedColor, thickness);
      }
      else
      {
        const std::vector<Pixel> colors = {mixedColor, mixedLowColor};
        std::vector<PixelBuffer*> buffs{&currentBuff, &nextBuff};
        m_draw.Line(buffs, x1, y1, x2, y2, colors, thickness);
      }

      x1 = x2;
      y1 = y2;
    }
  }

  // remove all dead particules
  const auto isDead = [&](const Star& s) { return IsStarDead(s); };
  // stars.erase(std::remove_if(stars.begin(), stars.end(), isDead), stars.end());
  const size_t numRemoved = std::erase_if(m_stars, isDead);
  if (numRemoved > 0)
  {
    m_stats.RemovedDeadStars(numRemoved);
  }
}

inline auto FlyingStarsFx::FlyingStarsImpl::IsStarDead(const Star& s) const -> bool
{
  return (s.x < -64) || (s.x > static_cast<float>(m_goomInfo->GetScreenInfo().width + 64)) ||
         (s.y < -64) || (s.y > static_cast<float>(m_goomInfo->GetScreenInfo().height + 64)) ||
         (s.age >= static_cast<float>(this->m_maxStarAge));
}

void FlyingStarsFx::FlyingStarsImpl::ChangeColorMode()
{
  // clang-format off
  static const Weights<ColorMode> s_colorModeWeights{{
    { ColorMode::mixColors,       30 },
    { ColorMode::reverseMixColors,15 },
    { ColorMode::similarLowColors,10 },
    { ColorMode::sineMixColors,    5 },
  }};
  // clang-format on

  m_colorMode = s_colorModeWeights.GetRandomWeighted();
}

inline auto FlyingStarsFx::FlyingStarsImpl::GetMixedColors(const Star& star,
                                                           const float t,
                                                           const float brightness)
    -> std::tuple<Pixel, Pixel>
{
  static GammaCorrection s_gammaCorrect{4.2, 0.1};

  Pixel color;
  Pixel lowColor;
  Pixel dominantColor;
  Pixel dominantLowColor;

  switch (m_colorMode)
  {
    case ColorMode::sineMixColors:
    {
      static float s_freq = 20;
      static const float s_zStep = 0.1;
      static float s_z = 0;

      const float tSin = 0.5F * (1.0F + std::sin(s_freq * s_z));
      color = star.currentColorMap->GetColor(tSin);
      lowColor = star.currentLowColorMap->GetColor(tSin);
      dominantColor = star.dominantColormap->GetColor(tSin);
      dominantLowColor = star.dominantLowColormap->GetColor(tSin);

      s_z += s_zStep;
      break;
    }
    case ColorMode::mixColors:
    case ColorMode::similarLowColors:
    {
      color = star.currentColorMap->GetColor(t);
      lowColor = star.currentLowColorMap->GetColor(t);
      dominantColor = star.dominantColormap->GetColor(t);
      if (m_colorMode == ColorMode::similarLowColors)
      {
        dominantLowColor = dominantColor;
      }
      else
      {
        dominantLowColor = star.dominantLowColormap->GetColor(t);
      }
      break;
    }
    case ColorMode::reverseMixColors:
    {
      color = star.currentColorMap->GetColor(1.0F - t);
      lowColor = star.currentLowColorMap->GetColor(1.0F - t);
      dominantColor = star.dominantColormap->GetColor(1.0F - t);
      dominantLowColor = star.dominantLowColormap->GetColor(1.0F - t);
      break;
    }
    default:
      throw std::logic_error("Unknown ColorMode enum.");
  }

  constexpr float MIN_MIX = 0.2;
  constexpr float MAX_MIX = 0.8;
  const float tMix = std::lerp(MIN_MIX, MAX_MIX, t);
  const Pixel mixedColor =
      s_gammaCorrect.GetCorrection(brightness, IColorMap::GetColorMix(color, dominantColor, tMix));
  const Pixel mixedLowColor =
      GetLightenedColor(IColorMap::GetColorMix(lowColor, dominantLowColor, tMix), 10.0F);
  const Pixel remixedLowColor =
      m_colorMode == ColorMode::similarLowColors
          ? mixedLowColor
          : s_gammaCorrect.GetCorrection(brightness,
                                         IColorMap::GetColorMix(mixedColor, mixedLowColor, 0.4));

  return std::make_tuple(mixedColor, remixedLowColor);
}

/**
 * Met a jour la position et vitesse d'une particule.
 */
void FlyingStarsFx::FlyingStarsImpl::UpdateStar(Star* s)
{
  s->x += s->xVelocity;
  s->y += s->yVelocity;
  s->xVelocity += s->xAcceleration;
  s->yVelocity += s->yAcceleration;
  s->age += s->vage;
}

/**
 * Ajoute de nouvelles particules au moment d'un evenement sonore.
 */
void FlyingStarsFx::FlyingStarsImpl::SoundEventOccurred()
{
  m_stats.SoundEventOccurred();

  if (ProbabilityOfMInN(1, 10))
  {
    m_minSideWind = GetRandInRange(-0.20F, -0.01F);
    m_maxSideWind = GetRandInRange(+0.01F, +0.20F);
    m_minGravity = GetRandInRange(+0.005F, +0.010F);
    m_maxGravity = GetRandInRange(+0.050F, +0.090F);
  }

  const auto halfWidth = static_cast<int32_t>(m_goomInfo->GetScreenInfo().width / 2);
  const auto halfHeight = static_cast<int32_t>(m_goomInfo->GetScreenInfo().height / 2);

  m_maxStarAge = MIN_STAR_AGE + GetNRand(MAX_STAR_EXTRA_AGE);
  m_useSingleBufferOnly = ProbabilityOfMInN(1, 100);

  float radius = (1.0F + m_goomInfo->GetSoundInfo().GetGoomPower()) *
                 static_cast<float>(GetNRand(150) + 50) / 300.0F;

  float gravityFactor = 1.0F;
  float windFactor = 1.0F;
  int32_t mx;
  int32_t my;
  float vage;

  switch (m_fxMode)
  {
    case StarModes::NO_FX:
      m_stats.NoFxChosen();
      return;
    case StarModes::FIREWORKS:
    {
      m_stats.FireworksFxChosen();
      const auto rsq = static_cast<double>(halfHeight * halfHeight);
      while (true)
      {
        mx = static_cast<int32_t>(GetNRand(m_goomInfo->GetScreenInfo().width));
        my = static_cast<int32_t>(GetNRand(m_goomInfo->GetScreenInfo().height));
        const double sqDist =
            SqDistance(static_cast<float>(mx - halfWidth), static_cast<float>(my - halfHeight));
        if (sqDist < rsq)
        {
          break;
        }
      }
      vage = m_maxAge * (1.0F - m_goomInfo->GetSoundInfo().GetGoomPower());
      windFactor = 0.1F;
      gravityFactor = 0.4F;
    }
    break;
    case StarModes::RAIN:
    {
      m_stats.RainFxChosen();
      const auto x0 = static_cast<int32_t>(m_goomInfo->GetScreenInfo().width / 25);
      mx = static_cast<int32_t>(
          GetRandInRange(x0, static_cast<int32_t>(m_goomInfo->GetScreenInfo().width) - x0));
      my = -GetRandInRange(3, 64);
      radius *= 1.5;
      vage = 0.002F;
      gravityFactor = 0.4F;
    }
    break;
    case StarModes::FOUNTAIN:
    {
      m_stats.FountainFxChosen();
      m_maxStarAge *= 2.0 / 3.0;
      const int32_t x0 = halfWidth / 5;
      mx = GetRandInRange(halfWidth - x0, halfWidth + x0);
      my = static_cast<int32_t>(m_goomInfo->GetScreenInfo().height + GetRandInRange(3U, 64U));
      vage = 0.001F;
      radius += 1.0F;
    }
    break;
    default:
      throw std::logic_error("Unknown StarModes enum.");
  }

  // Why 200 ? Because the FX was developed on 320x200.
  const auto heightRatio = static_cast<float>(m_goomInfo->GetScreenInfo().height) / 200.0F;
  auto maxStarsInBomb = static_cast<size_t>(
      heightRatio * (100.0F + (1.0F + m_goomInfo->GetSoundInfo().GetGoomPower()) *
                                  static_cast<float>(GetNRand(150))));
  radius *= heightRatio;
  if (m_goomInfo->GetSoundInfo().GetTimeSinceLastBigGoom() < 1)
  {
    radius *= 1.5;
    maxStarsInBomb *= 2;
  }

  m_colorMapsManager.ChangeColorMapNow(m_dominantColorMapID);
  m_colorMapsManager.ChangeColorMapNow(m_dominantLowColorMapID);

  const bool megaColorMode = ProbabilityOfMInN(1, 10);
  if (megaColorMode)
  {
    m_colorMapsManager.UpdateColorMapName(m_colorMapID, ColorMapName::_NULL);
    m_colorMapsManager.UpdateColorMapName(m_lowColorMapID, ColorMapName::_NULL);
  }
  else
  {
    m_colorMapsManager.UpdateColorMapName(m_colorMapID, m_colorMaps->GetRandomColorMapName());
    m_colorMapsManager.UpdateColorMapName(m_lowColorMapID, m_lowColorMaps->GetRandomColorMapName());
  }

  for (size_t i = 0; i < maxStarsInBomb; i++)
  {
    const float sideWind = windFactor * GetRandInRange(m_minSideWind, m_maxSideWind);
    const float gravity = gravityFactor * GetRandInRange(m_minGravity, m_maxGravity);
    m_colorMapsManager.ChangeColorMapNow(m_colorMapID);
    m_colorMapsManager.ChangeColorMapNow(m_lowColorMapID);
    AddABomb(mx, my, radius, vage, gravity, sideWind);
  }
}

/**
 * Cree une nouvelle 'bombe', c'est a dire une particule appartenant a une fusee d'artifice.
 */
void FlyingStarsFx::FlyingStarsImpl::AddABomb(const int32_t mx,
                                              const int32_t my,
                                              const float radius,
                                              float vage,
                                              const float gravity,
                                              const float sideWind)
{
  if (m_stars.size() >= m_maxStars)
  {
    m_stats.AddBombButTooManyStars();
    return;
  }
  m_stats.AddBomb();

  m_stars.push_back(Star{});

  const size_t i = m_stars.size() - 1;
  m_stars[i].x = mx;
  m_stars[i].y = my;

  // TODO Get colormap based on current mode.
  m_stars[i].dominantColormap = m_colorMapsManager.GetColorMapPtr(m_dominantColorMapID);
  m_stars[i].dominantLowColormap = m_colorMapsManager.GetColorMapPtr(m_dominantLowColorMapID);
  m_stars[i].currentColorMap = m_colorMapsManager.GetColorMapPtr(m_colorMapID);
  m_stars[i].currentLowColorMap = m_colorMapsManager.GetColorMapPtr(m_lowColorMapID);

  const float ro = radius * GetRandInRange(0.01F, 2.0F);
  const uint32_t theta = GetBombAngle(m_stars[i].x, m_stars[i].y);

  m_stars[i].xVelocity = ro * cos256[theta];
  m_stars[i].yVelocity = -0.2F + ro * sin256[theta];

  m_stars[i].xAcceleration = sideWind;
  m_stars[i].yAcceleration = gravity;

  m_stars[i].age = GetRandInRange(m_minAge, 0.5F * m_maxAge);
  if (vage < m_minAge)
  {
    vage = m_minAge;
  }
  m_stars[i].vage = vage;
}

auto FlyingStarsFx::FlyingStarsImpl::GetBombAngle(const float x,
                                                  [[maybe_unused]] const float y) const -> uint32_t
{
  float minAngle;
  float maxAngle;

  const float xFactor = x / static_cast<float>(m_goomInfo->GetScreenInfo().width - 1);

  switch (m_fxMode)
  {
    case StarModes::NO_FX:
      return 0;
    case StarModes::FIREWORKS:
      minAngle = 0;
      maxAngle = m_two_pi;
      break;
    case StarModes::RAIN:
    {
      constexpr float MIN_RAIN_ANGLE = 0.1;
      constexpr float MAX_RAIN_ANGLE = m_pi - 0.1;
      minAngle = std::lerp(MIN_RAIN_ANGLE, m_half_pi - 0.1F, 1.0F - xFactor);
      maxAngle = std::lerp(m_half_pi + 0.1F, MAX_RAIN_ANGLE, xFactor);
      break;
    }
    case StarModes::FOUNTAIN:
      minAngle = 1.0F * m_pi / 3.0F;
      maxAngle = 5.0F * m_pi / 3.0F;
      break;
    default:
      throw std::logic_error("Unknown StarModes enum.");
  }

  const float randAngle = GetRandInRange(minAngle, maxAngle);
  return static_cast<uint32_t>(0.001 +
                               static_cast<float>(NUM_SIN_COS_ANGLES - 1) * randAngle / m_two_pi);
}

} // namespace GOOM
