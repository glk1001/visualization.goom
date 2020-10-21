#include "flying_stars_fx.h"

#include "colorutils.h"
#include "goom_draw.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"
#include "goomutils/goomrand.h"
#include "goomutils/mathutils.h"

#include <algorithm>
#include <array>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

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
  float x;
  float y;
  float vx;
  float vy;
  float ax;
  float ay;
  float age;
  float vage;
  const ColorMap* currentColorMap = nullptr;
  const ColorMap* currentLowColorMap = nullptr;
  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(x, y, vx, vy, ax, ay, age, vage);
  }
};

struct FlyingStarsData
{
  explicit FlyingStarsData(const PluginInfo*);

  bool enabled = true;

  ColorMaps colorMaps;
  ColorMapGroup currentColorGroup;
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
  static constexpr size_t maxStarsLimit = 2048;
  size_t maxStars = maxStarsLimit;
  std::vector<Star> stars;
  size_t numStars = 0;
  uint32_t maxStarAge = 15;

  // Fireworks Largest Bombs
  float min_age = 1.0f - 99.0F / 100.0F;
  // Fireworks Smallest Bombs
  float max_age = 1.0f - 80.0F / 100.0f;

  FXBuffSettings buffSettings = defaultFXBuffSettings;
  bool useSingleBufferOnly = true;
  GoomDraw draw;
  StarsStats stats;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(currentColorGroup, fx_mode, numStars, maxStars, stars, maxStarAge, min_age, max_age,
       buffSettings, useSingleBufferOnly, draw);
  }
};

FlyingStarsData::FlyingStarsData(const PluginInfo* goomInfo)
  : colorMaps{},
    currentColorGroup{colorMaps.getRandomGroup()},
    stars(maxStarsLimit),
    draw{goomInfo->screen.width, goomInfo->screen.height},
    stats{}
{
}

/**
 * Met a jour la position et vitesse d'une particule.
 */
static void updateStar(Star* s)
{
  s->x += s->vx;
  s->y += s->vy;
  s->vx += s->ax;
  s->vy += s->ay;
  s->age += s->vage;
}

FlyingStarsFx::FlyingStarsFx(PluginInfo* info)
  : goomInfo{info}, fxData{new FlyingStarsData{goomInfo}}
{
}

void FlyingStarsFx::setBuffSettings(const FXBuffSettings& settings)
{
  fxData->buffSettings = settings;
  fxData->draw.setBuffIntensity(fxData->buffSettings.buffIntensity);
  fxData->draw.setAllowOverexposed(fxData->buffSettings.allowOverexposed);
}

void FlyingStarsFx::start()
{
}

void FlyingStarsFx::finish()
{
  std::ofstream f("/tmp/flying-stars.json");
  saveState(f);
  f << std::endl;
  f.close();
}

void FlyingStarsFx::log(const StatsLogValueFunc& logVal) const
{
  fxData->stats.log(logVal);
}

std::string FlyingStarsFx::getFxName() const
{
  return "Flying Stars FX";
}

void FlyingStarsFx::saveState(std::ostream& f)
{
  cereal::JSONOutputArchive archiveOut(f);
  fxData->stars.resize(fxData->numStars);
  archiveOut(*fxData);
}

void FlyingStarsFx::loadState(std::istream& f)
{
  cereal::JSONInputArchive archive_in(f);
  archive_in(*fxData);
  fxData->stars.resize(fxData->maxStars);
}

void FlyingStarsFx::apply(Pixel* prevBuff, Pixel* currentBuff)
{
  if (!fxData->enabled)
  {
    return;
  }

  fxData->maxStars = getRandInRange(256U, fxData->maxStarsLimit);

  // look for events
  if (goomInfo->sound->getTimeSinceLastGoom() < 1)
  {
    soundEventOccured();
    if (getNRand(20) == 1)
    {
      // Give a slight weight towards noFx mode by using numFX + 2.
      const uint32_t newVal = getNRand(numFx + 2);
      fxData->fx_mode = newVal >= numFx ? StarModes::noFx : static_cast<StarModes>(newVal);
    }
  }

  // update particules
  fxData->stats.updateStars();

  for (size_t i = 0; i < fxData->numStars; ++i)
  {
    updateStar(&fxData->stars[i]);

    // dead particule
    if (fxData->stars[i].age >= fxData->maxStarAge)
    {
      fxData->stats.deadStar();
      continue;
    }

    // choose the color of the particule
    const float tAge = fxData->stars[i].age / static_cast<float>(fxData->maxStarAge);
    const uint32_t color = fxData->stars[i].currentColorMap->getColor(tAge);
    const uint32_t lowColor = getLowColor(i, tAge);
    const float thicknessFactor = getRandInRange(1.0F, 3.0F);

    // draws the particule
    const int x0 = static_cast<int>(fxData->stars[i].x);
    const int y0 = static_cast<int>(fxData->stars[i].y);
    int x1 = x0;
    int y1 = y0;
    constexpr size_t numParts = 7;
    for (size_t j = 1; j <= numParts; j++)
    {
      const float t = static_cast<float>(j - 1) / static_cast<float>(numParts - 1);
      const Pixel mixedColor{.val = ColorMap::colorMix(color, lowColor, t)};
      const int x2 = x0 - static_cast<int>(fxData->stars[i].vx * j);
      const int y2 = y0 - static_cast<int>(fxData->stars[i].vy * j);
      const uint8_t thickness = static_cast<uint8_t>(
          std::clamp(1u, 2u, static_cast<uint32_t>(thicknessFactor * (1.0 - t))));
      //      1u, 5u, static_cast<uint32_t>((1.0 - std::fabs(t - 0.5)) * getRandInRange(1.0F, 5.0F))));

      if (fxData->useSingleBufferOnly)
      {
        fxData->draw.line(currentBuff, x1, y1, x2, y2, mixedColor, thickness);
      }
      else
      {
        const std::vector<Pixel> colors = {mixedColor, {.val = lowColor}};
        std::vector<Pixel*> buffs{currentBuff, prevBuff};
        fxData->draw.line(buffs, x1, y1, x2, y2, colors, thickness);
      }

      x1 = x2;
      y1 = y2;
    }
  }

  // look for dead particules
  for (size_t i = 0; i < fxData->numStars;)
  {
    if ((fxData->stars[i].x > static_cast<float>(goomInfo->screen.width + 64)) ||
        ((fxData->stars[i].vy >= 0) &&
         (fxData->stars[i].y - 16 * fxData->stars[i].vy > goomInfo->screen.height)) ||
        (fxData->stars[i].x < -64) || (fxData->stars[i].age >= fxData->maxStarAge))
    {
      fxData->stars[i] = fxData->stars[fxData->numStars - 1];
      fxData->numStars--;
      fxData->stats.removedDeadStar();
    }
    else
    {
      ++i;
    }
  }
}

/**
 * Ajoute de nouvelles particules au moment d'un evenement sonore.
 */
void FlyingStarsFx::soundEventOccured()
{
  fxData->stats.soundEventOccurred();

  const uint32_t halfWidth = goomInfo->screen.width / 2;
  const uint32_t halfHeight = goomInfo->screen.height / 2;
  fxData->currentColorGroup = fxData->colorMaps.getRandomGroup();
  fxData->maxStarAge = minStarAge + getNRand(maxStarExtraAge);
  fxData->useSingleBufferOnly = probabilityOfMInN(1, 10);

  size_t max = 100 + static_cast<size_t>((1.0f + goomInfo->sound->getGoomPower()) * getNRand(150));
  float radius =
      (1.0f + goomInfo->sound->getGoomPower()) * static_cast<float>(getNRand(150) + 50) / 300.0F;
  float gravity = 0.02f;

  uint32_t mx;
  uint32_t my;
  float vage;

  switch (fxData->fx_mode)
  {
    case StarModes::noFx:
      fxData->stats.noFxChosen();
      return;
    case StarModes::fireworks:
    {
      fxData->stats.fireworksFxChosen();
      const double rsq = halfHeight * halfHeight;
      while (true)
      {
        mx = getNRand(goomInfo->screen.width);
        my = getNRand(goomInfo->screen.height);
        const double dx = mx - halfWidth;
        const double dy = my - halfHeight;
        if ((dx * dx) + (dy * dy) >= rsq)
        {
          break;
        }
      }
      vage = fxData->max_age * (1.0f - goomInfo->sound->getGoomPower());
    }
    break;
    case StarModes::rain:
      fxData->stats.rainFxChosen();
      mx = getNRand(goomInfo->screen.width);
      mx = (mx <= halfWidth) ? 0 : goomInfo->screen.width;
      my = -(goomInfo->screen.height / 3) - getNRand(goomInfo->screen.width / 3);
      radius *= 1.5;
      vage = 0.002f;
      break;
    case StarModes::fountain:
      fxData->stats.fountainFxChosen();
      fxData->maxStarAge *= 2.0 / 3.0;
      my = goomInfo->screen.height + 2;
      mx = halfWidth;
      vage = 0.001f;
      radius += 1.0f;
      gravity = 0.04f;
      break;
    default:
      throw std::logic_error("Unknown StarModes enum.");
  }

  radius *= goomInfo->screen.height / 200.0f; // Why 200 ? Because the FX was developed on 320x200.
  max *= goomInfo->screen.height / 200.0f;
  if (goomInfo->sound->getTimeSinceLastBigGoom() < 1)
  {
    radius *= 1.5;
    max *= 2;
  }

  for (size_t i = 0; i < max; i++)
  {
    addABomb(mx, my, radius, vage, gravity);
  }
}

inline uint32_t FlyingStarsFx::getLowColor(const size_t starNum, const float tmix)
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
    return starLowColors[size_t(tmix * static_cast<float>(numLowColors - 1))];
  }

  const float brightness = getRandInRange(0.2f, 0.8f);
  return getBrighterColor(brightness, fxData->stars[starNum].currentLowColorMap->getColor(tmix),
                          fxData->buffSettings.allowOverexposed);
}

/**
 * Cree une nouvelle 'bombe', c'est a dire une particule appartenant a une fusee d'artifice.
 */
void FlyingStarsFx::addABomb(
    const uint32_t mx, const uint32_t my, const float radius, float vage, const float gravity)
{
  if (fxData->numStars >= fxData->maxStars)
  {
    fxData->stats.addBombButTooManyStars();
    return;
  }
  fxData->stats.addBomb();

  fxData->numStars++;

  const unsigned int i = fxData->numStars - 1;
  fxData->stars[i].x = mx;
  fxData->stars[i].y = my;

  // TODO Get colormap based on current mode.
  fxData->stars[i].currentColorMap =
      &fxData->colorMaps.getRandomColorMap(fxData->currentColorGroup);
  fxData->stars[i].currentLowColorMap = &fxData->lowColorMaps.getRandomColorMap();

  float ro = radius * static_cast<float>(getNRand(100)) / 100.0f;
  ro *= static_cast<float>(getNRand(100)) / 100.0f + 1.0f;
  const uint32_t theta = getNRand(256);

  fxData->stars[i].vx = ro * cos256[theta];
  fxData->stars[i].vy = -0.2f + ro * sin256[theta];

  fxData->stars[i].ax = 0;
  fxData->stars[i].ay = gravity;

  fxData->stars[i].age = 0;
  if (vage < fxData->min_age)
  {
    vage = fxData->min_age;
  }
  fxData->stars[i].vage = vage;
}

} // namespace goom
