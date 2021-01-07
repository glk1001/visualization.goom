#include "flying_stars_fx.h"

#include "goom_draw.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"
#include "goomutils/colorutils.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
//#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"

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

CEREAL_REGISTER_TYPE(goom::FlyingStarsFx)
CEREAL_REGISTER_POLYMORPHIC_RELATION(goom::VisualFx, goom::FlyingStarsFx)

namespace goom
{

using namespace goom::utils;

class StarsStats
{
public:
  StarsStats() noexcept = default;

  void reset();
  void log(const StatsLogValueFunc&) const;
  void addBombButTooManyStars();
  void addBomb();
  void soundEventOccurred();
  void noFxChosen();
  void fireworksFxChosen();
  void rainFxChosen();
  void fountainFxChosen();
  void updateStars();
  void deadStar();
  void removedDeadStars(uint32_t val);
  void setLastNumActive(uint32_t val);
  void setLastMaxStars(uint32_t val);
  void setLastMaxStarAge(uint32_t val);

private:
  uint32_t numAddBombButTooManyStars = 0;
  uint32_t numAddBombs = 0;
  uint32_t numSoundEvents = 0;
  uint32_t numNoFxChosen = 0;
  uint32_t numFireworksFxChosen = 0;
  uint32_t numRainFxChosen = 0;
  uint32_t numFountainFxChosen = 0;
  uint32_t numUpdateStars = 0;
  uint32_t numDeadStars = 0;
  uint32_t numRemovedStars = 0;
  uint32_t lastNumActive = 0;
  uint32_t lastMaxStars = 0;
  uint32_t lastMaxStarAge = 0;
};

void StarsStats::reset()
{
  numAddBombButTooManyStars = 0;
  numAddBombs = 0;
  numSoundEvents = 0;
  numNoFxChosen = 0;
  numFireworksFxChosen = 0;
  numRainFxChosen = 0;
  numFountainFxChosen = 0;
  numUpdateStars = 0;
  numDeadStars = 0;
  numRemovedStars = 0;
}

void StarsStats::log(const StatsLogValueFunc& logVal) const
{
  const constexpr char* module = "Stars";

  logVal(module, "numUpdateStars", numUpdateStars);
  logVal(module, "numSoundEvents", numSoundEvents);
  logVal(module, "numAddBombButTooManyStars", numAddBombButTooManyStars);
  logVal(module, "numAddBombs", numAddBombs);
  logVal(module, "numNoFxChosen", numNoFxChosen);
  logVal(module, "numFireworksFxChosen", numFireworksFxChosen);
  logVal(module, "numRainFxChosen", numRainFxChosen);
  logVal(module, "numFountainFxChosen", numFountainFxChosen);
  logVal(module, "numDeadStars", numDeadStars);
  logVal(module, "numRemovedStars", numRemovedStars);
  logVal(module, "lastNumActive", lastNumActive);
  logVal(module, "lastMaxStars", lastMaxStars);
  logVal(module, "lastMaxStarAge", lastMaxStarAge);
}

inline void StarsStats::updateStars()
{
  numUpdateStars++;
}

inline void StarsStats::addBombButTooManyStars()
{
  numAddBombButTooManyStars++;
}

inline void StarsStats::addBomb()
{
  numAddBombs++;
}

inline void StarsStats::soundEventOccurred()
{
  numSoundEvents++;
}

inline void StarsStats::noFxChosen()
{
  numNoFxChosen++;
}

inline void StarsStats::fireworksFxChosen()
{
  numFireworksFxChosen++;
}

inline void StarsStats::rainFxChosen()
{
  numRainFxChosen++;
}

inline void StarsStats::fountainFxChosen()
{
  numFountainFxChosen++;
}

inline void StarsStats::deadStar()
{
  numDeadStars++;
}

inline void StarsStats::removedDeadStars(uint32_t val)
{
  numRemovedStars += val;
}

inline void StarsStats::setLastNumActive(uint32_t val)
{
  lastNumActive = val;
}

inline void StarsStats::setLastMaxStars(uint32_t val)
{
  lastMaxStars = val;
}

inline void StarsStats::setLastMaxStarAge(uint32_t val)
{
  lastMaxStarAge = val;
}


constexpr uint32_t minStarAge = 10;
constexpr uint32_t maxStarExtraAge = 40;

/* TODO:-- FAIRE PROPREMENT... BOAH... */

// The different modes of the visual FX.
enum class StarModes
{
  noFx = 0,
  fireworks,
  rain,
  fountain,
  _size // must be last - gives number of enums
};
constexpr size_t numFx = static_cast<uint32_t>(StarModes::_size);

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
  const ColorMap* dominantColormap{};
  const ColorMap* dominantLowColormap{};
  const ColorMap* currentColorMap{};
  const ColorMap* currentLowColorMap{};

  bool operator==(const Star& s) const
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
  explicit FlyingStarsImpl(std::shared_ptr<const PluginInfo>) noexcept;
  FlyingStarsImpl(const FlyingStarsImpl&) = delete;
  FlyingStarsImpl& operator=(const FlyingStarsImpl&) = delete;

  void setBuffSettings(const FXBuffSettings&);

  void updateBuffers(PixelBuffer& currentBuff, PixelBuffer& nextBuff);

  void finish();
  void log(const StatsLogValueFunc& logVal) const;

  bool operator==(const FlyingStarsImpl&) const;

private:
  std::shared_ptr<const PluginInfo> goomInfo{};

  const WeightedColorMaps colorMaps{Weights<ColorMapGroup>{{
      {ColorMapGroup::perceptuallyUniformSequential, 10},
      {ColorMapGroup::sequential, 10},
      {ColorMapGroup::sequential2, 10},
      {ColorMapGroup::cyclic, 0},
      {ColorMapGroup::diverging, 0},
      {ColorMapGroup::diverging_black, 0},
      {ColorMapGroup::qualitative, 10},
      {ColorMapGroup::misc, 10},
  }}};
  const WeightedColorMaps lowColorMaps{Weights<ColorMapGroup>{{
      {ColorMapGroup::perceptuallyUniformSequential, 10},
      {ColorMapGroup::sequential, 0},
      {ColorMapGroup::sequential2, 10},
      {ColorMapGroup::cyclic, 5},
      {ColorMapGroup::diverging, 10},
      {ColorMapGroup::diverging_black, 20},
      {ColorMapGroup::qualitative, 1},
      {ColorMapGroup::misc, 10},
  }}};

  ColorMode m_colorMode = ColorMode::mixColors;
  uint32_t m_counter = 0;
  static constexpr uint32_t maxCount = 100;

  StarModes m_fxMode = StarModes::fireworks;
  static constexpr size_t maxStarsLimit = 1024;
  size_t maxStars = maxStarsLimit;
  std::vector<Star> stars{};
  uint32_t maxStarAge = 15;

  // Fireworks Largest Bombs
  float minAge = 1.0F - (99.0F / 100.0F);
  // Fireworks Smallest Bombs
  float maxAge = 1.0F - (80.0F / 100.0F);

  FXBuffSettings buffSettings{};
  bool useSingleBufferOnly = true;
  GoomDraw draw{};
  StarsStats stats{};

  void soundEventOccurred();
  static void updateStar(Star*);
  void changeColorMode();
  [[nodiscard]] std::tuple<Pixel, Pixel> getMixedColors(const Star&, float t, float brightness);
  [[nodiscard]] bool isStarDead(const Star&) const;
  void addABomb(const ColorMap& dominantColormap,
                const ColorMap& dominantLowColormap,
                const ColorMap& colorMap,
                const ColorMap& lowColorMap,
                int32_t mx,
                int32_t my,
                float radius,
                float vage,
                float gravity);
  [[nodiscard]] uint32_t getBombAngle(float x, float y) const;

  friend class cereal::access;
  template<class Archive>
  void save(Archive&) const;
  template<class Archive>
  void load(Archive&);
};

FlyingStarsFx::FlyingStarsFx() noexcept : fxImpl{new FlyingStarsImpl{}}
{
}

FlyingStarsFx::FlyingStarsFx(const std::shared_ptr<const PluginInfo>& info) noexcept
  : fxImpl{new FlyingStarsImpl{info}}
{
}

FlyingStarsFx::~FlyingStarsFx() noexcept = default;

bool FlyingStarsFx::operator==(const FlyingStarsFx& f) const
{
  return fxImpl->operator==(*f.fxImpl);
}

void FlyingStarsFx::setBuffSettings(const FXBuffSettings& settings)
{
  fxImpl->setBuffSettings(settings);
}

void FlyingStarsFx::start()
{
}

void FlyingStarsFx::finish()
{
  fxImpl->finish();
}

void FlyingStarsFx::log(const StatsLogValueFunc& logVal) const
{
  fxImpl->log(logVal);
}

std::string FlyingStarsFx::getFxName() const
{
  return "Flying Stars FX";
}

void FlyingStarsFx::apply(PixelBuffer&)
{
  throw std::logic_error("FlyingStarsFx::apply should never be called.");
}

void FlyingStarsFx::apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff)
{
  if (!enabled)
  {
    return;
  }

  fxImpl->updateBuffers(currentBuff, nextBuff);
}

template<class Archive>
void FlyingStarsFx::serialize(Archive& ar)
{
  ar(CEREAL_NVP(enabled), CEREAL_NVP(fxImpl));
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
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(m_fxMode), CEREAL_NVP(maxStars), CEREAL_NVP(stars),
     CEREAL_NVP(maxStarAge), CEREAL_NVP(minAge), CEREAL_NVP(maxAge), CEREAL_NVP(buffSettings),
     CEREAL_NVP(useSingleBufferOnly), CEREAL_NVP(draw));
}

template<class Archive>
void FlyingStarsFx::FlyingStarsImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(m_fxMode), CEREAL_NVP(maxStars), CEREAL_NVP(stars),
     CEREAL_NVP(maxStarAge), CEREAL_NVP(minAge), CEREAL_NVP(maxAge), CEREAL_NVP(buffSettings),
     CEREAL_NVP(useSingleBufferOnly), CEREAL_NVP(draw));
}

bool FlyingStarsFx::FlyingStarsImpl::operator==(const FlyingStarsImpl& f) const
{
  if (goomInfo == nullptr && f.goomInfo != nullptr)
  {
    return false;
  }
  if (goomInfo != nullptr && f.goomInfo == nullptr)
  {
    return false;
  }

  return ((goomInfo == nullptr && f.goomInfo == nullptr) || (*goomInfo == *f.goomInfo)) &&
         m_fxMode == f.m_fxMode && maxStars == f.maxStars && stars == f.stars &&
         maxStarAge == f.maxStarAge && minAge == f.minAge && maxAge == f.maxAge &&
         buffSettings == f.buffSettings && useSingleBufferOnly == f.useSingleBufferOnly &&
         draw == f.draw;
}

FlyingStarsFx::FlyingStarsImpl::FlyingStarsImpl() noexcept = default;

FlyingStarsFx::FlyingStarsImpl::FlyingStarsImpl(std::shared_ptr<const PluginInfo> info) noexcept
  : goomInfo{std::move(info)},
    draw{goomInfo->getScreenInfo().width, goomInfo->getScreenInfo().height}
{
  stars.reserve(maxStarsLimit);
}

void FlyingStarsFx::FlyingStarsImpl::setBuffSettings(const FXBuffSettings& settings)
{
  draw.setBuffIntensity(settings.buffIntensity);
  draw.setAllowOverexposed(settings.allowOverexposed);
}

void FlyingStarsFx::FlyingStarsImpl::finish()
{
  stats.setLastNumActive(stars.size());
  stats.setLastMaxStars(maxStars);
  stats.setLastMaxStarAge(maxStarAge);
}

void FlyingStarsFx::FlyingStarsImpl::log(const StatsLogValueFunc& logVal) const
{
  stats.log(logVal);
}

void FlyingStarsFx::FlyingStarsImpl::updateBuffers(PixelBuffer& currentBuff, PixelBuffer& nextBuff)
{
  m_counter++;

  maxStars = getRandInRange(100U, maxStarsLimit);

  // look for events
  if (stars.empty() || goomInfo->getSoundInfo().getTimeSinceLastGoom() < 1)
  {
    soundEventOccurred();
    if (getNRand(20) == 1)
    {
      // Give a slight weight towards noFx mode by using numFX + 2.
      const uint32_t newVal = getNRand(numFx + 2);
      m_fxMode = newVal >= numFx ? StarModes::noFx : static_cast<StarModes>(newVal);
      changeColorMode();
    }
    else if (m_counter > maxCount)
    {
      m_counter = 0;
      changeColorMode();
    }
  }
  // m_fxMode = StarModes::rain;

  // update particules
  stats.updateStars();

  const float flipSpeed = getRandInRange(0.1F, 10.0F);

  for (auto& star : stars)
  {
    updateStar(&star);

    // dead particule
    if (star.age >= maxStarAge)
    {
      stats.deadStar();
      continue;
    }

    // draws the particule
    constexpr float oldAge = 0.95;
    const float tAge = star.age / static_cast<float>(maxStarAge);
    const size_t numParts =
        tAge > oldAge ? 4 : 2 + static_cast<size_t>(std::lround((1.0F - tAge) * 2.0F));
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
      const uint8_t thickness = tAge < oldAge ? 1 : getRandInRange(2U, 5U);

      const float brightness =
          2.0F * (1.0F - tAge) * static_cast<float>(j - 1) / static_cast<float>(numParts - 1);
      const auto [mixedColor, mixedLowColor] = getMixedColors(star, tAge, brightness);

      if (useSingleBufferOnly)
      {
        draw.line(currentBuff, x1, y1, x2, y2, mixedColor, thickness);
      }
      else
      {
        const std::vector<Pixel> colors = {mixedColor, mixedLowColor};
        std::vector<PixelBuffer*> buffs{&currentBuff, &nextBuff};
        draw.line(buffs, x1, y1, x2, y2, colors, thickness);
      }

      x1 = x2;
      y1 = y2;
    }
  }

  // remove all dead particules
  const auto isDead = [&](const Star& s) { return isStarDead(s); };
  // stars.erase(std::remove_if(stars.begin(), stars.end(), isDead), stars.end());
  const size_t numRemoved = std::erase_if(stars, isDead);
  if (numRemoved > 0)
  {
    stats.removedDeadStars(numRemoved);
  }
}

inline bool FlyingStarsFx::FlyingStarsImpl::isStarDead(const Star& star) const
{
  return (star.x < -64) || (star.x > static_cast<float>(goomInfo->getScreenInfo().width + 64)) ||
         (star.y < -64) || (star.y > static_cast<float>(goomInfo->getScreenInfo().height + 64)) ||
         (star.age >= maxStarAge);
}

void FlyingStarsFx::FlyingStarsImpl::changeColorMode()
{
  // clang-format off
  static const Weights<ColorMode> colorModeWeights{{
    { ColorMode::mixColors,       30 },
    { ColorMode::reverseMixColors,15 },
    { ColorMode::similarLowColors,10 },
    { ColorMode::sineMixColors,    5 },
  }};
  // clang-format on

  m_colorMode = colorModeWeights.getRandomWeighted();
}

inline std::tuple<Pixel, Pixel> FlyingStarsFx::FlyingStarsImpl::getMixedColors(
    const Star& star, const float t, const float brightness)
{
  static GammaCorrection gammaCorrect{4.2, 0.1};

  Pixel color;
  Pixel lowColor;
  Pixel dominantColor;
  Pixel dominantLowColor;

  switch (m_colorMode)
  {
    case ColorMode::sineMixColors:
    {
      static float freq = 20;
      static const float zStep = 0.1;
      static float z = 0;

      const float tSin = 0.5F * (1.0F + std::sin(freq * z));
      color = star.currentColorMap->getColor(tSin);
      lowColor = star.currentLowColorMap->getColor(tSin);
      dominantColor = star.dominantColormap->getColor(tSin);
      dominantLowColor = star.dominantLowColormap->getColor(tSin);

      z += zStep;
      break;
    }
    case ColorMode::mixColors:
    case ColorMode::similarLowColors:
    {
      color = star.currentColorMap->getColor(t);
      lowColor = star.currentLowColorMap->getColor(t);
      dominantColor = star.dominantColormap->getColor(t);
      if (m_colorMode == ColorMode::similarLowColors)
      {
        dominantLowColor = dominantColor;
      }
      else
      {
        dominantLowColor = star.dominantLowColormap->getColor(t);
      }
      break;
    }
    case ColorMode::reverseMixColors:
    {
      color = star.currentColorMap->getColor(1.0F - t);
      lowColor = star.currentLowColorMap->getColor(1.0F - t);
      dominantColor = star.dominantColormap->getColor(1.0F - t);
      dominantLowColor = star.dominantLowColormap->getColor(1.0F - t);
      break;
    }
    default:
      throw std::logic_error("Unknown ColorMode enum.");
  }

  constexpr float minMix = 0.2;
  constexpr float maxMix = 0.8;
  const float tMix = std::lerp(minMix, maxMix, t);
  const Pixel mixedColor =
      gammaCorrect.getCorrection(brightness, ColorMap::colorMix(color, dominantColor, tMix));
  const Pixel mixedLowColor =
      getLightenedColor(ColorMap::colorMix(lowColor, dominantLowColor, tMix), 10.0F);
  const Pixel remixedLowColor =
      m_colorMode == ColorMode::similarLowColors
          ? mixedLowColor
          : gammaCorrect.getCorrection(brightness,
                                       ColorMap::colorMix(mixedColor, mixedLowColor, 0.4));

  return std::make_tuple(mixedColor, remixedLowColor);
}

/**
 * Met a jour la position et vitesse d'une particule.
 */
void FlyingStarsFx::FlyingStarsImpl::updateStar(Star* s)
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
void FlyingStarsFx::FlyingStarsImpl::soundEventOccurred()
{
  stats.soundEventOccurred();

  const auto halfWidth = static_cast<int32_t>(goomInfo->getScreenInfo().width / 2);
  const auto halfHeight = static_cast<int32_t>(goomInfo->getScreenInfo().height / 2);

  maxStarAge = minStarAge + getNRand(maxStarExtraAge);
  useSingleBufferOnly = probabilityOfMInN(1, 100);

  float radius = (1.0f + goomInfo->getSoundInfo().getGoomPower()) *
                 static_cast<float>(getNRand(150) + 50) / 300.0F;
  float gravity = 0.02f;

  int32_t mx;
  int32_t my;
  float vage;

  switch (m_fxMode)
  {
    case StarModes::noFx:
      stats.noFxChosen();
      return;
    case StarModes::fireworks:
    {
      stats.fireworksFxChosen();
      const double rsq = halfHeight * halfHeight;
      while (true)
      {
        mx = static_cast<int32_t>(getNRand(goomInfo->getScreenInfo().width));
        my = static_cast<int32_t>(getNRand(goomInfo->getScreenInfo().height));
        const double sqDist =
            sq_distance(static_cast<float>(mx - halfWidth), static_cast<float>(my - halfHeight));
        if (sqDist < rsq)
        {
          break;
        }
      }
      vage = maxAge * (1.0F - goomInfo->getSoundInfo().getGoomPower());
    }
    break;
    case StarModes::rain:
      stats.rainFxChosen();
      mx = static_cast<int32_t>(
          getRandInRange(50, static_cast<int32_t>(goomInfo->getScreenInfo().width) - 50));
      my = -getRandInRange(3, 64);
      radius *= 1.5;
      vage = 0.002F;
      break;
    case StarModes::fountain:
      stats.fountainFxChosen();
      maxStarAge *= 2.0 / 3.0;
      mx = getRandInRange(halfWidth - 50, halfWidth + 50);
      my = static_cast<int32_t>(goomInfo->getScreenInfo().height + getRandInRange(3U, 64U));
      vage = 0.001F;
      radius += 1.0F;
      gravity = 0.05F;
      break;
    default:
      throw std::logic_error("Unknown StarModes enum.");
  }

  // Why 200 ? Because the FX was developed on 320x200.
  const float heightRatio = goomInfo->getScreenInfo().height / 200.0F;
  size_t maxStarsInBomb = heightRatio * (100.0F + (1.0F + goomInfo->getSoundInfo().getGoomPower()) *
                                                      static_cast<float>(getNRand(150)));
  radius *= heightRatio;
  if (goomInfo->getSoundInfo().getTimeSinceLastBigGoom() < 1)
  {
    radius *= 1.5;
    maxStarsInBomb *= 2;
  }

  const ColorMap& dominantColorMap = colorMaps.getRandomColorMap();
  const ColorMap& dominantLowColorMap = lowColorMaps.getRandomColorMap();
  for (size_t i = 0; i < maxStarsInBomb; i++)
  {
    addABomb(dominantColorMap, dominantLowColorMap, colorMaps.getRandomColorMap(),
             lowColorMaps.getRandomColorMap(), mx, my, radius, vage, gravity);
  }
}

/**
 * Cree une nouvelle 'bombe', c'est a dire une particule appartenant a une fusee d'artifice.
 */
void FlyingStarsFx::FlyingStarsImpl::addABomb(const ColorMap& dominantColormap,
                                              const ColorMap& dominantLowColormap,
                                              const ColorMap& colorMap,
                                              const ColorMap& lowColorMap,
                                              const int32_t mx,
                                              const int32_t my,
                                              const float radius,
                                              float vage,
                                              const float gravity)
{
  if (stars.size() >= maxStars)
  {
    stats.addBombButTooManyStars();
    return;
  }
  stats.addBomb();

  stars.push_back(Star{});

  const size_t i = stars.size() - 1;
  stars[i].x = mx;
  stars[i].y = my;

  // TODO Get colormap based on current mode.
  stars[i].dominantColormap = &dominantColormap;
  stars[i].dominantLowColormap = &dominantLowColormap;
  stars[i].currentColorMap = &colorMap;
  stars[i].currentLowColorMap = &lowColorMap;

  const float ro = radius * getRandInRange(0.01F, 2.0F);
  const uint32_t theta = getBombAngle(stars[i].x, stars[i].y);

  stars[i].xVelocity = ro * cos256[theta];
  stars[i].yVelocity = -0.2F + ro * sin256[theta];

  stars[i].xAcceleration = getRandInRange(-0.01F, 0.01F);
  stars[i].yAcceleration = gravity;

  stars[i].age = getRandInRange(minAge, 0.5F * maxAge);
  if (vage < minAge)
  {
    vage = minAge;
  }
  stars[i].vage = vage;
}

uint32_t FlyingStarsFx::FlyingStarsImpl::getBombAngle(const float x,
                                                      [[maybe_unused]] const float y) const
{
  float minAngle;
  float maxAngle;

  const float xFactor = x / static_cast<float>(goomInfo->getScreenInfo().width - 1);

  switch (m_fxMode)
  {
    case StarModes::noFx:
      return 0;
    case StarModes::fireworks:
      minAngle = 0;
      maxAngle = m_two_pi;
      break;
    case StarModes::rain:
    {
      constexpr float minRainAngle = 0.1;
      constexpr float maxRainAngle = m_pi - 0.1;
      minAngle = std::lerp(minRainAngle, m_half_pi - 0.1F, 1.0F - xFactor);
      maxAngle = std::lerp(m_half_pi + 0.1F, maxRainAngle, xFactor);
      break;
    }
    case StarModes::fountain:
      minAngle = 4 * m_pi / 3;
      maxAngle = 5 * m_pi / 3;
      break;
    default:
      throw std::logic_error("Unknown StarModes enum.");
  }

  const float randAngle = getRandInRange(minAngle, maxAngle);
  return static_cast<uint32_t>(0.001 +
                               static_cast<float>(numSinCosAngles - 1) * randAngle / m_two_pi);
}

} // namespace goom
