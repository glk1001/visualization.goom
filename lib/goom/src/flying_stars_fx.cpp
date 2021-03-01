#include "flying_stars_fx.h"

#include "goom_config.h"
#include "goom_draw.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormaps.h"
#include "goomutils/colorutils.h"
#include "goomutils/goomrand.h"
#include "goomutils/image_bitmaps.h"
//#include "goomutils/logging_control.h"
//#undef NO_LOGGING
//#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "goomutils/random_colormaps.h"
#include "goomutils/random_colormaps_manager.h"
#include "goomutils/small_image_bitmaps.h"
#include "stats/stars_stats.h"

#include <cmath>
#include <cstddef>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace GOOM
{

using namespace GOOM::UTILS;
using UTILS::COLOR_DATA::ColorMapName;

constexpr uint32_t MIN_STAR_AGE = 15;
constexpr uint32_t MAX_STAR_EXTRA_AGE = 50;

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
  std::shared_ptr<const IColorMap> dominantColormap{};
  std::shared_ptr<const IColorMap> dominantLowColormap{};
  std::shared_ptr<const IColorMap> currentColorMap{};
  std::shared_ptr<const IColorMap> currentLowColorMap{};
};

class FlyingStarsFx::FlyingStarsImpl
{
public:
  explicit FlyingStarsImpl(std::shared_ptr<const PluginInfo> goomInfo) noexcept;
  ~FlyingStarsImpl() noexcept = default;
  FlyingStarsImpl(const FlyingStarsImpl&) noexcept = delete;
  FlyingStarsImpl(FlyingStarsImpl&&) noexcept = delete;
  auto operator=(const FlyingStarsImpl&) -> FlyingStarsImpl& = delete;
  auto operator=(FlyingStarsImpl&&) -> FlyingStarsImpl& = delete;

  [[nodiscard]] auto GetResourcesDirectory() const -> const std::string&;
  void SetResourcesDirectory(const std::string& dirName);
  void SetSmallImageBitmaps(const SmallImageBitmaps& smallBitmaps);
  void SetBuffSettings(const FXBuffSettings& settings);

  void Start();

  void UpdateBuffers(PixelBuffer& currentBuff, PixelBuffer& nextBuff);

  void Finish();
  void Log(const StatsLogValueFunc& logVal) const;

private:
  const std::shared_ptr<const PluginInfo> m_goomInfo;

  const std::shared_ptr<WeightedColorMaps> m_colorMaps{
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
  const std::shared_ptr<WeightedColorMaps> m_lowColorMaps{
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

  std::string m_resourcesDirectory{};

  uint32_t m_counter = 0;
  static constexpr uint32_t MAX_COUNT = 100;

  StarModes m_fxMode = StarModes::FIREWORKS;
  static constexpr uint32_t MAX_NUM_STARS = 2048;
  static constexpr uint32_t MIN_NUM_STARS = 100;
  uint32_t m_maxStars = MAX_NUM_STARS;
  std::vector<Star> m_stars{};
  static constexpr float OLD_AGE = 0.95;
  uint32_t m_maxStarAge = 15;

  static constexpr float MIN_MIN_SIDE_WIND = -0.10F;
  static constexpr float MAX_MIN_SIDE_WIND = -0.01F;
  static constexpr float MIN_MAX_SIDE_WIND = +0.01F;
  static constexpr float MAX_MAX_SIDE_WIND = +0.10F;
  float m_minSideWind = 0.0;
  float m_maxSideWind = 0.00001;

  static constexpr float MIN_MIN_GRAVITY = +0.005F;
  static constexpr float MAX_MIN_GRAVITY = +0.010F;
  static constexpr float MIN_MAX_GRAVITY = +0.050F;
  static constexpr float MAX_MAX_GRAVITY = +0.090F;
  float m_minGravity = MAX_MIN_GRAVITY;
  float m_maxGravity = MAX_MAX_GRAVITY;

  // Fireworks Largest Bombs
  float m_minAge = 1.0F - (99.0F / 100.0F);
  // Fireworks Smallest Bombs
  float m_maxAge = 1.0F - (80.0F / 100.0F);

  FXBuffSettings m_buffSettings{};
  bool m_useSingleBufferOnly = true;
  GoomDraw m_draw;
  StarsStats m_stats{};

  void CheckForStarEvents();
  void SoundEventOccurred();
  void DrawStars(PixelBuffer& currentBuff, PixelBuffer& nextBuff);
  static void UpdateStar(Star& s);
  [[nodiscard]] auto IsStarDead(const Star& s) const -> bool;
  enum class DrawMode
  {
    CIRCLES,
    LINES,
    DOTS,
    CIRCLES_AND_LINES,
  };
  DrawMode m_drawMode = DrawMode::CIRCLES;
  void ChangeDrawMode();
  const SmallImageBitmaps* m_smallBitmaps{};
  auto GetImageBitmap(size_t size) -> const ImageBitmap&;
  using DrawFunc = std::function<void(const std::vector<PixelBuffer*>& buffs,
                                      int32_t x1,
                                      int32_t y1,
                                      int32_t x2,
                                      int32_t y2,
                                      uint32_t size,
                                      const std::vector<Pixel>& colors)>;
  const std::map<DrawMode, const DrawFunc> m_drawFuncs{};
  auto GetDrawFunc() const -> DrawFunc;
  void DrawParticle(PixelBuffer& currentBuff,
                    PixelBuffer& nextBuff,
                    const Star& star,
                    float flipSpeed,
                    const DrawFunc& drawFunc);
  void DrawParticleCircle(const std::vector<PixelBuffer*>& buffs,
                          int32_t x1,
                          int32_t y1,
                          int32_t x2,
                          int32_t y2,
                          uint32_t size,
                          const std::vector<Pixel>& colors);
  void DrawParticleLine(const std::vector<PixelBuffer*>& buffs,
                        int32_t x1,
                        int32_t y1,
                        int32_t x2,
                        int32_t y2,
                        uint32_t size,
                        const std::vector<Pixel>& colors);
  void DrawParticleDot(const std::vector<PixelBuffer*>& buffs,
                       int32_t x1,
                       int32_t y1,
                       int32_t x2,
                       int32_t y2,
                       uint32_t size,
                       const std::vector<Pixel>& colors);
  void RemoveDeadStars();
  void AddABomb(int32_t mx, int32_t my, float radius, float vage, float gravity, float sideWind);
  [[nodiscard]] auto GetBombAngle(float x, float y) const -> float;

  static constexpr size_t MIN_DOT_SIZE = 3;
  static constexpr size_t MAX_DOT_SIZE = 5;
  static_assert(MAX_DOT_SIZE <= SmallImageBitmaps::MAX_IMAGE_SIZE, "Max dot size mismatch.");
};

FlyingStarsFx::FlyingStarsFx(const std::shared_ptr<const PluginInfo>& info) noexcept
  : m_fxImpl{new FlyingStarsImpl{info}}
{
}

FlyingStarsFx::~FlyingStarsFx() noexcept = default;

auto FlyingStarsFx::GetResourcesDirectory() const -> const std::string&
{
  return m_fxImpl->GetResourcesDirectory();
}

void FlyingStarsFx::SetResourcesDirectory(const std::string& dirName)
{
  m_fxImpl->SetResourcesDirectory(dirName);
}

void FlyingStarsFx::SetBuffSettings(const FXBuffSettings& settings)
{
  m_fxImpl->SetBuffSettings(settings);
}

void FlyingStarsFx::SetSmallImageBitmaps(const SmallImageBitmaps& smallBitmaps)
{
  m_fxImpl->SetSmallImageBitmaps(smallBitmaps);
}

void FlyingStarsFx::Start()
{
  m_fxImpl->Start();
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

void FlyingStarsFx::Apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff)
{
  if (!m_enabled)
  {
    return;
  }

  m_fxImpl->UpdateBuffers(currentBuff, nextBuff);
}

FlyingStarsFx::FlyingStarsImpl::FlyingStarsImpl(std::shared_ptr<const PluginInfo> info) noexcept
  : m_goomInfo{std::move(info)},
    m_draw{m_goomInfo->GetScreenInfo().width, m_goomInfo->GetScreenInfo().height},
    m_drawFuncs{{DrawMode::CIRCLES,
                 [&](const std::vector<PixelBuffer*>& buffs,
                     const int32_t x1,
                     const int32_t y1,
                     const int32_t x2,
                     const int32_t y2,
                     const uint32_t size,
                     const std::vector<Pixel>& colors) {
                   DrawParticleCircle(buffs, x1, y1, x2, y2, size, colors);
                 }},
                {DrawMode::LINES,
                 [&](const std::vector<PixelBuffer*>& buffs,
                     const int32_t x1,
                     const int32_t y1,
                     const int32_t x2,
                     const int32_t y2,
                     const uint32_t size,
                     const std::vector<Pixel>& colors) {
                   DrawParticleLine(buffs, x1, y1, x2, y2, size, colors);
                 }},
                {
                    DrawMode::DOTS,
                    [&](const std::vector<PixelBuffer*>& buffs,
                        const int32_t x1,
                        const int32_t y1,
                        const int32_t x2,
                        const int32_t y2,
                        const uint32_t size,
                        const std::vector<Pixel>& colors) {
                      DrawParticleDot(buffs, x1, y1, x2, y2, size, colors);
                    },
                }}
{
  m_stars.reserve(MAX_NUM_STARS);

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

inline auto FlyingStarsFx::FlyingStarsImpl::GetResourcesDirectory() const -> const std::string&
{
  return m_resourcesDirectory;
}

inline void FlyingStarsFx::FlyingStarsImpl::SetResourcesDirectory(const std::string& dirName)
{
  m_resourcesDirectory = dirName;
}

void FlyingStarsFx::FlyingStarsImpl::SetBuffSettings(const FXBuffSettings& settings)
{
  m_draw.SetBuffIntensity(settings.buffIntensity);
  m_draw.SetAllowOverexposed(settings.allowOverexposed);
}

inline void FlyingStarsFx::FlyingStarsImpl::SetSmallImageBitmaps(
    const SmallImageBitmaps& smallBitmaps)
{
  m_smallBitmaps = &smallBitmaps;
}

inline auto FlyingStarsFx::FlyingStarsImpl::GetImageBitmap(const size_t size) -> const ImageBitmap&
{
  return m_smallBitmaps->GetImageBitmap(
      SmallImageBitmaps::ImageNames::CIRCLE,
      stdnew::clamp(size % 2 != 0 ? size : size + 1, MIN_DOT_SIZE, MAX_DOT_SIZE));
}

inline void FlyingStarsFx::FlyingStarsImpl::Start()
{
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

  m_maxStars = GetRandInRange(MIN_NUM_STARS, MAX_NUM_STARS);

  CheckForStarEvents();
  DrawStars(currentBuff, nextBuff);
  RemoveDeadStars();
}

void FlyingStarsFx::FlyingStarsImpl::CheckForStarEvents()
{
  if (m_stars.empty() || m_goomInfo->GetSoundInfo().GetTimeSinceLastGoom() < 1)
  {
    SoundEventOccurred();
    if (ProbabilityOfMInN(1, 20))
    {
      // Give a slight weight towards noFx mode by using numFX + 2.
      const uint32_t newVal = GetNRand(NUM_FX + 2);
      m_fxMode = newVal >= NUM_FX ? StarModes::NO_FX : static_cast<StarModes>(newVal);
      ChangeColorMode();
      ChangeDrawMode();
    }
    else if (m_counter > MAX_COUNT)
    {
      m_counter = 0;
      ChangeColorMode();
      ChangeDrawMode();
    }
  }
  // m_fxMode = StarModes::rain;
}

void FlyingStarsFx::FlyingStarsImpl::DrawStars(PixelBuffer& currentBuff, PixelBuffer& nextBuff)
{
  m_stats.UpdateStars();
  const float flipSpeed = GetRandInRange(0.1F, 10.0F);

  for (auto& star : m_stars)
  {
    UpdateStar(star);

    // Is it a dead particle?
    if (star.age >= static_cast<float>(m_maxStarAge))
    {
      m_stats.DeadStar();
      continue;
    }

    DrawParticle(currentBuff, nextBuff, star, flipSpeed, GetDrawFunc());
  }
}

void FlyingStarsFx::FlyingStarsImpl::ChangeDrawMode()
{
  // clang-format off
  static const Weights<DrawMode> s_drawModeWeights{{
      { DrawMode::DOTS,              30 },
      { DrawMode::CIRCLES,           20 },
      { DrawMode::LINES,             10 },
      { DrawMode::CIRCLES_AND_LINES, 15 },
  }};
  // clang-format on

  m_drawMode = s_drawModeWeights.GetRandomWeighted();
}


inline auto FlyingStarsFx::FlyingStarsImpl::GetDrawFunc() const -> DrawFunc
{
  if (m_drawMode != DrawMode::CIRCLES_AND_LINES)
  {
    return m_drawFuncs.at(m_drawMode);
  }
  return ProbabilityOfMInN(1, 2) ? m_drawFuncs.at(DrawMode::CIRCLES)
                                 : m_drawFuncs.at(DrawMode::LINES);
}

void FlyingStarsFx::FlyingStarsImpl::DrawParticle(PixelBuffer& currentBuff,
                                                  PixelBuffer& nextBuff,
                                                  const Star& star,
                                                  const float flipSpeed,
                                                  const DrawFunc& drawFunc)
{
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
        x0 - static_cast<int32_t>(
                 0.5 * (1.0 + std::sin(flipSpeed * star.xVelocity * static_cast<float>(j))) *
                 star.xVelocity * static_cast<float>(j));
    const int32_t y2 =
        y0 - static_cast<int32_t>(
                 0.5 * (1.0 + std::cos(flipSpeed * star.yVelocity * static_cast<float>(j))) *
                 star.yVelocity * static_cast<float>(j));

    const float brightness = ageBrightness * static_cast<float>(j) / static_cast<float>(numParts);
#if __cplusplus <= 201402L
    const auto mixedColors = GetMixedColors(star, tAge, brightness);
    const auto mixedColor = std::get<0>(mixedColors);
    const auto mixedLowColor = std::get<1>(mixedColors);
#else
    const auto [mixedColor, mixedLowColor] = GetMixedColors(star, tAge, brightness);
#endif
    const std::vector<Pixel> colors = {mixedColor, mixedLowColor};
    const std::vector<PixelBuffer*> buffs{&currentBuff, &nextBuff};
    const uint32_t size = tAge < OLD_AGE ? 1 : GetRandInRange(2U, MAX_DOT_SIZE + 1);

    drawFunc(buffs, x1, y1, x2, y2, size, colors);

    x1 = x2;
    y1 = y2;
  }
}

void FlyingStarsFx::FlyingStarsImpl::DrawParticleCircle(const std::vector<PixelBuffer*>& buffs,
                                                        const int32_t x1,
                                                        const int32_t y1,
                                                        [[maybe_unused]] const int32_t x2,
                                                        [[maybe_unused]] const int32_t y2,
                                                        const uint32_t size,
                                                        const std::vector<Pixel>& colors)
{
  if (m_useSingleBufferOnly)
  {
    m_draw.Circle(*(buffs[0]), x1, y1, static_cast<int>(size), colors[0]);
  }
  else
  {
    m_draw.Circle(buffs, x1, y1, static_cast<int>(size), colors);
  }
}

void FlyingStarsFx::FlyingStarsImpl::DrawParticleLine(const std::vector<PixelBuffer*>& buffs,
                                                      const int32_t x1,
                                                      const int32_t y1,
                                                      const int32_t x2,
                                                      const int32_t y2,
                                                      const uint32_t size,
                                                      const std::vector<Pixel>& colors)
{
  if (m_useSingleBufferOnly)
  {
    m_draw.Line(*(buffs[0]), x1, y1, x2, y2, colors[0], static_cast<int>(size));
  }
  else
  {
    m_draw.Line(buffs, x1, y1, x2, y2, colors, static_cast<int>(size));
  }
}

void FlyingStarsFx::FlyingStarsImpl::DrawParticleDot(const std::vector<PixelBuffer*>& buffs,
                                                     const int32_t x1,
                                                     const int32_t y1,
                                                     [[maybe_unused]] const int32_t x2,
                                                     [[maybe_unused]] const int32_t y2,
                                                     const uint32_t size,
                                                     const std::vector<Pixel>& colors)
{
  const auto getColor = [&]([[maybe_unused]] const size_t x, [[maybe_unused]] const size_t y,
                            const Pixel& b) -> Pixel {
    return GetColorMultiply(b, colors[0], m_buffSettings.allowOverexposed);
  };
  const auto getLowColor = [&]([[maybe_unused]] const size_t x, [[maybe_unused]] const size_t y,
                               const Pixel& b) -> Pixel {
    return GetColorMultiply(b, colors[1], m_buffSettings.allowOverexposed);
  };

  const ImageBitmap& bitmap1 = GetImageBitmap(size);

  if (m_useSingleBufferOnly)
  {
    m_draw.Bitmap(*(buffs[0]), x1, y1, bitmap1, getColor);
  }
  else
  {
    const std::vector<GoomDraw::GetBitmapColorFunc> getColors{getColor, getLowColor};
    const std::vector<const PixelBuffer*> bitmaps{&bitmap1, &bitmap1};
    m_draw.Bitmap(buffs, x1, y1, bitmaps, getColors);
  }
}

void FlyingStarsFx::FlyingStarsImpl::RemoveDeadStars()
{
  const auto isDead = [&](const Star& s) { return IsStarDead(s); };
#if __cplusplus <= 201402L
  m_stars.erase(std::remove_if(m_stars.begin(), m_stars.end(), isDead), m_stars.end());
#else
  const size_t numRemoved = std::erase_if(m_stars, isDead);
  if (numRemoved > 0)
  {
    m_stats.RemovedDeadStars(numRemoved);
  }
#endif
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
  const float tMix = stdnew::lerp(MIN_MIX, MAX_MIX, t);
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
void FlyingStarsFx::FlyingStarsImpl::UpdateStar(Star& s)
{
  s.x += s.xVelocity;
  s.y += s.yVelocity;
  s.xVelocity += s.xAcceleration;
  s.yVelocity += s.yAcceleration;
  s.age += s.vage;
}

/**
 * Ajoute de nouvelles particules au moment d'un evenement sonore.
 */
void FlyingStarsFx::FlyingStarsImpl::SoundEventOccurred()
{
  m_stats.SoundEventOccurred();

  if (ProbabilityOfMInN(1, 10))
  {
    m_minSideWind = GetRandInRange(MIN_MIN_SIDE_WIND, MAX_MIN_SIDE_WIND);
    m_maxSideWind = GetRandInRange(MIN_MAX_SIDE_WIND, MAX_MAX_SIDE_WIND);
    m_minGravity = GetRandInRange(MIN_MIN_GRAVITY, MAX_MIN_GRAVITY);
    m_maxGravity = GetRandInRange(MIN_MAX_GRAVITY, MAX_MAX_GRAVITY);
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
  m_stars[i].x = static_cast<float>(mx);
  m_stars[i].y = static_cast<float>(my);

  // TODO Get colormap based on current mode.
  m_stars[i].dominantColormap = m_colorMapsManager.GetColorMapPtr(m_dominantColorMapID);
  m_stars[i].dominantLowColormap = m_colorMapsManager.GetColorMapPtr(m_dominantLowColorMapID);
  m_stars[i].currentColorMap = m_colorMapsManager.GetColorMapPtr(m_colorMapID);
  m_stars[i].currentLowColorMap = m_colorMapsManager.GetColorMapPtr(m_lowColorMapID);

  const float ro = radius * GetRandInRange(0.01F, 2.0F);
  const float angle = GetBombAngle(m_stars[i].x, m_stars[i].y);

  m_stars[i].xVelocity = ro * std::cos(angle);
  m_stars[i].yVelocity = -0.2F + ro * std::sin(angle);

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
                                                  [[maybe_unused]] const float y) const -> float
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
      minAngle = stdnew::lerp(MIN_RAIN_ANGLE, m_half_pi - 0.1F, 1.0F - xFactor);
      maxAngle = stdnew::lerp(m_half_pi + 0.1F, MAX_RAIN_ANGLE, xFactor);
      break;
    }
    case StarModes::FOUNTAIN:
      minAngle = 1.0F * m_pi / 3.0F;
      maxAngle = 5.0F * m_pi / 3.0F;
      break;
    default:
      throw std::logic_error("Unknown StarModes enum.");
  }

  return GetRandInRange(minAngle, maxAngle);
}

} // namespace GOOM
