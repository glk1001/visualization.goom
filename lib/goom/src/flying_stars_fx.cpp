#include "flying_stars_fx.h"

#include "goom_draw.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"
#include "goomutils/colorutils.h"
#include "goomutils/goomrand.h"
#include "goomutils/mathutils.h"

#include <algorithm>
#include <array>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cmath>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

CEREAL_REGISTER_TYPE(goom::FlyingStarsFx);
CEREAL_REGISTER_POLYMORPHIC_RELATION(goom::VisualFx, goom::FlyingStarsFx);

namespace goom
{

using namespace goom::utils;

class StarsStats
{
public:
  StarsStats() noexcept = default;

  void reset();
  void log(const StatsLogValueFunc) const;
  void addBombButTooManyStars();
  void addBomb();
  void soundEventOccurred();
  void noFxChosen();
  void fireworksFxChosen();
  void rainFxChosen();
  void fountainFxChosen();
  void updateStars();
  void deadStar();
  void removedDeadStar();

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

void StarsStats::log(const StatsLogValueFunc logVal) const
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

inline void StarsStats::removedDeadStar()
{
  numRemovedStars++;
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
  float vx = 0;
  float vy = 0;
  float ax = 0;
  float ay = 0;
  float age = 0;
  float vage = 0;
  // TODO: Cereal for these pointers????
  const ColorMap* currentColorMap = nullptr;
  const ColorMap* currentLowColorMap = nullptr;

  bool operator==(const Star& s) const
  {
    return x == s.x && y == s.y && vx == s.vx && vy == s.vy && ax == s.ax && ay == s.ay &&
           age == s.age && vage == s.vage;
  }

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(x, y, vx, vy, ax, ay, age, vage);
  }
};

class FlyingStarsFx::FlyingStarsImpl
{
public:
  FlyingStarsImpl() noexcept;
  explicit FlyingStarsImpl(const std::shared_ptr<const PluginInfo>&) noexcept;
  FlyingStarsImpl(const FlyingStarsImpl&) = delete;
  FlyingStarsImpl& operator=(const FlyingStarsImpl&) = delete;

  void setBuffSettings(const FXBuffSettings&);

  void updateBuffers(PixelBuffer& prevBuff, PixelBuffer& currentBuff);

  void log(const StatsLogValueFunc& logVal) const;

  bool operator==(const FlyingStarsImpl&) const;

private:
  std::shared_ptr<const PluginInfo> goomInfo{};

  ColorMaps colorMaps{};
  const WeightedColorMaps lowColorMaps{Weights<ColorMapGroup>{{
      {ColorMapGroup::perceptuallyUniformSequential, 0},
      {ColorMapGroup::sequential, 0},
      {ColorMapGroup::sequential2, 0},
      {ColorMapGroup::cyclic, 5},
      {ColorMapGroup::diverging, 10},
      {ColorMapGroup::diverging_black, 20},
      {ColorMapGroup::qualitative, 1},
      {ColorMapGroup::misc, 1},
  }}};

  StarModes fx_mode = StarModes::fireworks;
  static constexpr size_t maxStarsLimit = 1024;
  size_t maxStars = maxStarsLimit;
  std::vector<Star> stars{};
  uint32_t maxStarAge = 15;

  // Fireworks Largest Bombs
  float min_age = 1.0f - 99.0F / 100.0F;
  // Fireworks Smallest Bombs
  float max_age = 1.0f - 80.0F / 100.0f;

  FXBuffSettings buffSettings{};
  bool useSingleBufferOnly = true;
  GoomDraw draw{};
  StarsStats stats{};

  void soundEventOccured();
  static void updateStar(Star*);
  Pixel getLowColor(const size_t starNum, const float tmix);
  bool isStarDead(const Star&) const;
  void addABomb(const utils::ColorMapGroup colorGroup,
                const utils::ColorMap* lowColorMap,
                const uint32_t mx,
                const uint32_t my,
                const float radius,
                float vage,
                const float gravity);

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

FlyingStarsFx::~FlyingStarsFx() noexcept
{
}

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
}

void FlyingStarsFx::log(const StatsLogValueFunc& logVal) const
{
  fxImpl->log(logVal);
}

std::string FlyingStarsFx::getFxName() const
{
  return "Flying Stars FX";
}

void FlyingStarsFx::apply(PixelBuffer& prevBuff, PixelBuffer& currentBuff)
{
  if (!enabled)
  {
    return;
  }

  fxImpl->updateBuffers(prevBuff, currentBuff);
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
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(fx_mode), CEREAL_NVP(maxStars), CEREAL_NVP(stars),
     CEREAL_NVP(maxStarAge), CEREAL_NVP(min_age), CEREAL_NVP(max_age), CEREAL_NVP(buffSettings),
     CEREAL_NVP(useSingleBufferOnly), CEREAL_NVP(draw));
}

template<class Archive>
void FlyingStarsFx::FlyingStarsImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(fx_mode), CEREAL_NVP(maxStars), CEREAL_NVP(stars),
     CEREAL_NVP(maxStarAge), CEREAL_NVP(min_age), CEREAL_NVP(max_age), CEREAL_NVP(buffSettings),
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
         fx_mode == f.fx_mode && maxStars == f.maxStars && stars == f.stars &&
         maxStarAge == f.maxStarAge && min_age == f.min_age && max_age == f.max_age &&
         buffSettings == f.buffSettings && useSingleBufferOnly == f.useSingleBufferOnly &&
         draw == f.draw;
}

FlyingStarsFx::FlyingStarsImpl::FlyingStarsImpl() noexcept
{
}

FlyingStarsFx::FlyingStarsImpl::FlyingStarsImpl(
    const std::shared_ptr<const PluginInfo>& info) noexcept
  : goomInfo{info}, draw{goomInfo->getScreenInfo().width, goomInfo->getScreenInfo().height}
{
  stars.reserve(maxStarsLimit);
}

void FlyingStarsFx::FlyingStarsImpl::setBuffSettings(const FXBuffSettings& settings)
{
  draw.setBuffIntensity(settings.buffIntensity);
  draw.setAllowOverexposed(settings.allowOverexposed);
}

void FlyingStarsFx::FlyingStarsImpl::log(const StatsLogValueFunc& logVal) const
{
  stats.log(logVal);
}

void FlyingStarsFx::FlyingStarsImpl::updateBuffers(PixelBuffer& prevBuff, PixelBuffer& currentBuff)
{
  maxStars = getRandInRange(100U, maxStarsLimit);

  // look for events
  if (goomInfo->getSoundInfo().getTimeSinceLastGoom() < 1)
  {
    soundEventOccured();
    if (getNRand(20) == 1)
    {
      // Give a slight weight towards noFx mode by using numFX + 2.
      const uint32_t newVal = getNRand(numFx + 2);
      fx_mode = newVal >= numFx ? StarModes::noFx : static_cast<StarModes>(newVal);
    }
  }

  // update particules
  stats.updateStars();

  for (size_t i = 0; i < stars.size(); ++i)
  {
    updateStar(&stars[i]);

    // dead particule
    if (stars[i].age >= maxStarAge)
    {
      stats.deadStar();
      continue;
    }

    // choose the color of the particule
    const float tAge = stars[i].age / static_cast<float>(maxStarAge);
    const Pixel color = stars[i].currentColorMap->getColor(tAge);
    const Pixel lowColor = getLowColor(i, tAge);
    const float thicknessFactor = getRandInRange(1.0F, 3.0F);

    // draws the particule
    static GammaCorrection gammaCorrect{4.2, 0.1};
    const int x0 = static_cast<int>(stars[i].x);
    const int y0 = static_cast<int>(stars[i].y);
    int x1 = x0;
    int y1 = y0;
    constexpr size_t numParts = 7;
    for (size_t j = 1; j <= numParts; j++)
    {
      const float t = static_cast<float>(j - 1) / static_cast<float>(numParts - 1);
      const Pixel mixedColor =
          gammaCorrect.getCorrection(tAge, ColorMap::colorMix(color, lowColor, t));
      const int x2 = x0 - static_cast<int>(stars[i].vx * j);
      const int y2 = y0 - static_cast<int>(stars[i].vy * j);
      const uint8_t thickness = static_cast<uint8_t>(
          std::clamp(1u, 2u, static_cast<uint32_t>(thicknessFactor * (1.0 - t))));
      //      1u, 5u, static_cast<uint32_t>((1.0 - std::fabs(t - 0.5)) * getRandInRange(1.0F, 5.0F))));

      if (useSingleBufferOnly)
      {
        draw.line(currentBuff, x1, y1, x2, y2, mixedColor, thickness);
      }
      else
      {
        const std::vector<Pixel> colors = {mixedColor, lowColor};
        std::vector<PixelBuffer*> buffs{&currentBuff, &prevBuff};
        draw.line(buffs, x1, y1, x2, y2, colors, thickness);
      }

      x1 = x2;
      y1 = y2;
    }
  }

  // remove all dead particules
  const auto isDead = [&](const Star& s) { return isStarDead(s); };
  std::erase_if(stars, isDead);
  // stars.erase(std::remove_if(stars.begin(), stars.end(), isDead),
  //                     stars.end());
}

inline bool FlyingStarsFx::FlyingStarsImpl::isStarDead(const Star& star) const
{
  return (star.x > static_cast<float>(goomInfo->getScreenInfo().width + 64)) ||
         ((star.vy >= 0) && (star.y - 16 * star.vy > goomInfo->getScreenInfo().height)) ||
         (star.x < -64) || (star.age >= maxStarAge);
}

/**
 * Met a jour la position et vitesse d'une particule.
 */
void FlyingStarsFx::FlyingStarsImpl::updateStar(Star* s)
{
  s->x += s->vx;
  s->y += s->vy;
  s->vx += s->ax;
  s->vy += s->ay;
  s->age += s->vage;
}

inline Pixel FlyingStarsFx::FlyingStarsImpl::getLowColor(const size_t starNum, const float tmix)
{
  if (probabilityOfMInN(1, 2))
  {
    // clang-format off
    static constexpr size_t numLowColors = 15;
    static constexpr std::array<uint32_t, numLowColors> starLowColors{
      0x1416181a, 0x1419181a, 0x141f181a, 0x1426181a, 0x142a181a,
      0x142f181a, 0x1436181a, 0x142f1819, 0x14261615, 0x13201411,
      0x111a100a, 0x0c180508, 0x08100304, 0x00050101, 0x0
    };
    // clang-format on
    return Pixel{starLowColors[size_t(tmix * static_cast<float>(numLowColors - 1))]};
  }

  const float brightness = getRandInRange(0.2f, 0.6f);
  return getBrighterColor(brightness, stars[starNum].currentLowColorMap->getColor(tmix),
                          buffSettings.allowOverexposed);
}

/**
 * Ajoute de nouvelles particules au moment d'un evenement sonore.
 */
void FlyingStarsFx::FlyingStarsImpl::soundEventOccured()
{
  stats.soundEventOccurred();

  const uint32_t halfWidth = goomInfo->getScreenInfo().width / 2;
  const uint32_t halfHeight = goomInfo->getScreenInfo().height / 2;

  maxStarAge = minStarAge + getNRand(maxStarExtraAge);
  useSingleBufferOnly = probabilityOfMInN(1, 10);

  float radius = (1.0f + goomInfo->getSoundInfo().getGoomPower()) *
                 static_cast<float>(getNRand(150) + 50) / 300.0F;
  float gravity = 0.02f;

  uint32_t mx;
  uint32_t my;
  float vage;

  switch (fx_mode)
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
        mx = getNRand(goomInfo->getScreenInfo().width);
        my = getNRand(goomInfo->getScreenInfo().height);
        const double dx = mx - halfWidth;
        const double dy = my - halfHeight;
        if ((dx * dx) + (dy * dy) >= rsq)
        {
          break;
        }
      }
      vage = max_age * (1.0f - goomInfo->getSoundInfo().getGoomPower());
    }
    break;
    case StarModes::rain:
      stats.rainFxChosen();
      mx = getNRand(goomInfo->getScreenInfo().width);
      mx = (mx <= halfWidth) ? 0 : goomInfo->getScreenInfo().width;
      my = -(goomInfo->getScreenInfo().height / 3) - getNRand(goomInfo->getScreenInfo().width / 3);
      radius *= 1.5;
      vage = 0.002f;
      break;
    case StarModes::fountain:
      stats.fountainFxChosen();
      maxStarAge *= 2.0 / 3.0;
      my = goomInfo->getScreenInfo().height + 2;
      mx = halfWidth;
      vage = 0.001f;
      radius += 1.0f;
      gravity = 0.04f;
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

  ColorMapGroup colorGroup = colorMaps.getRandomGroup();
  const ColorMap* lowColorMap = &lowColorMaps.getRandomColorMap();

  for (size_t i = 0; i < maxStarsInBomb; i++)
  {
    addABomb(colorGroup, lowColorMap, mx, my, radius, vage, gravity);
  }
}

/**
 * Cree une nouvelle 'bombe', c'est a dire une particule appartenant a une fusee d'artifice.
 */
void FlyingStarsFx::FlyingStarsImpl::addABomb(const ColorMapGroup colorGroup,
                                              const ColorMap* lowColorMap,
                                              const uint32_t mx,
                                              const uint32_t my,
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

  const unsigned int i = stars.size() - 1;
  stars[i].x = mx;
  stars[i].y = my;

  // TODO Get colormap based on current mode.
  stars[i].currentColorMap = &colorMaps.getRandomColorMap(colorGroup);
  stars[i].currentLowColorMap = lowColorMap;

  float ro = radius * static_cast<float>(getNRand(100)) / 100.0f;
  ro *= static_cast<float>(getNRand(100)) / 100.0f + 1.0f;
  const uint32_t theta = getNRand(256);

  stars[i].vx = ro * cos256[theta];
  stars[i].vy = -0.2f + ro * sin256[theta];

  stars[i].ax = 0;
  stars[i].ay = gravity;

  stars[i].age = 0;
  if (vage < min_age)
  {
    vage = min_age;
  }
  stars[i].vage = vage;
}

} // namespace goom
