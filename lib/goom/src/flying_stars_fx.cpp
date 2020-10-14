#include "flying_stars_fx.h"

#include "colorutils.h"
#include "goom_draw.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"
#include "goomutils/colormap_enums.h"
#include "goomutils/goomrand.h"
#include "goomutils/mathutils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <stdexcept>
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
};

struct FSData
{
  PluginParam enabled_bp;

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

  uint32_t maxAge = 15;

  StarModes fx_mode;
  size_t numStars;

  size_t maxStars;
  Star* stars;

  float min_age;
  float max_age;

  FXBuffSettings buffSettings;
  bool useSingleBufferOnly;
  std::unique_ptr<GoomDraw> draw;

  PluginParam min_age_p;
  PluginParam max_age_p;
  PluginParam nbStars_p;
  PluginParam nbStars_limit_p;
  PluginParam fx_mode_p;

  PluginParameters params;
};

static StarsStats stats{};

static void fs_init(VisualFX* _this, PluginInfo* goomInfo)
{
  stats.reset();

  FSData* data = new FSData;

  data->currentColorGroup = data->colorMaps.getRandomGroup();

  data->fx_mode = StarModes::fireworks;
  data->maxStars = 6000;
  data->stars = new Star[data->maxStars];
  data->numStars = 0;
  data->buffSettings = defaultFXBuffSettings;
  data->useSingleBufferOnly = true;
  data->draw = std::make_unique<GoomDraw>(goomInfo->screen.width, goomInfo->screen.height);

  data->max_age_p = secure_i_param("Fireworks Smallest Bombs");
  IVAL(data->max_age_p) = 80;
  IMIN(data->max_age_p) = 0;
  IMAX(data->max_age_p) = 100;
  ISTEP(data->max_age_p) = 1;

  data->min_age_p = secure_i_param("Fireworks Largest Bombs");
  IVAL(data->min_age_p) = 99;
  IMIN(data->min_age_p) = 0;
  IMAX(data->min_age_p) = 100;
  ISTEP(data->min_age_p) = 1;

  data->nbStars_limit_p = secure_i_param("Max Number of Particules");
  IVAL(data->nbStars_limit_p) = 512;
  IMIN(data->nbStars_limit_p) = 0;
  IMAX(data->nbStars_limit_p) = static_cast<int>(data->maxStars);
  ISTEP(data->nbStars_limit_p) = 64;

  data->fx_mode_p = secure_i_param("FX Mode");
  IVAL(data->fx_mode_p) = static_cast<int>(data->fx_mode);
  IMIN(data->fx_mode_p) = 0;
  IMAX(data->fx_mode_p) = numFx - 1;
  ISTEP(data->fx_mode_p) = 1;

  data->nbStars_p = secure_f_feedback("Number of Particules (% of Max)");

  data->params = plugin_parameters("Particule System", 8);
  data->params.params[0] = &data->fx_mode_p;
  data->params.params[1] = &data->nbStars_limit_p;
  data->params.params[2] = 0;
  data->params.params[3] = &data->min_age_p;
  data->params.params[4] = &data->max_age_p;
  data->params.params[5] = 0;
  data->params.params[6] = &data->nbStars_p;

  data->enabled_bp = secure_b_param("Flying Stars", 1);
  data->params.params[7] = &data->enabled_bp;

  _this->params = &data->params;
  _this->fx_data = static_cast<void*>(data);
}

static void fs_free(VisualFX* _this)
{
  FSData* data = static_cast<FSData*>(_this->fx_data);

  delete[] data->stars;
  free(data->params.params);

  delete data;
}

/**
 * Cree une nouvelle 'bombe', c'est a dire une particule appartenant a une fusee d'artifice.
 */
static void addABomb(FSData* data,
                     const uint32_t mx,
                     const uint32_t my,
                     const float radius,
                     float vage,
                     const float gravity)
{
  if (data->numStars >= data->maxStars)
  {
    stats.addBombButTooManyStars();
    return;
  }
  stats.addBomb();

  data->numStars++;

  const unsigned int i = data->numStars - 1;
  data->stars[i].x = mx;
  data->stars[i].y = my;

  // TODO Get colormap based on current mode.
  data->stars[i].currentColorMap = &data->colorMaps.getRandomColorMap(data->currentColorGroup);
  data->stars[i].currentLowColorMap = &data->lowColorMaps.getRandomColorMap();

  float ro = radius * static_cast<float>(getNRand(100)) / 100.0f;
  ro *= static_cast<float>(getNRand(100)) / 100.0f + 1.0f;
  const uint32_t theta = getNRand(256);

  data->stars[i].vx = ro * cos256[theta];
  data->stars[i].vy = -0.2f + ro * sin256[theta];

  data->stars[i].ax = 0;
  data->stars[i].ay = gravity;

  data->stars[i].age = 0;
  if (vage < data->min_age)
  {
    vage = data->min_age;
  }
  data->stars[i].vage = vage;
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

/**
 * Ajoute de nouvelles particules au moment d'un evenement sonore.
 */
static void fs_sound_event_occured(VisualFX* _this, PluginInfo* goomInfo)
{
  stats.soundEventOccurred();

  FSData* data = static_cast<FSData*>(_this->fx_data);

  const uint32_t halfWidth = goomInfo->screen.width / 2;
  const uint32_t halfHeight = goomInfo->screen.height / 2;
  data->currentColorGroup = data->colorMaps.getRandomGroup();
  data->maxAge = minStarAge + getNRand(maxStarExtraAge);
  data->useSingleBufferOnly = probabilityOfMInN(1, 10);

  size_t max = 100 + static_cast<size_t>((1.0f + goomInfo->sound->getGoomPower()) * getNRand(150));
  float radius =
      (1.0f + goomInfo->sound->getGoomPower()) * static_cast<float>(getNRand(150) + 50) / 300.0F;
  float gravity = 0.02f;

  uint32_t mx;
  uint32_t my;
  float vage;

  switch (data->fx_mode)
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
        mx = getNRand(goomInfo->screen.width);
        my = getNRand(goomInfo->screen.height);
        const double dx = mx - halfWidth;
        const double dy = my - halfHeight;
        if ((dx * dx) + (dy * dy) >= rsq)
        {
          break;
        }
      }
      vage = data->max_age * (1.0f - goomInfo->sound->getGoomPower());
    }
    break;
    case StarModes::rain:
      stats.rainFxChosen();
      mx = getNRand(goomInfo->screen.width);
      mx = (mx <= halfWidth) ? 0 : goomInfo->screen.width;
      my = -(goomInfo->screen.height / 3) - getNRand(goomInfo->screen.width / 3);
      radius *= 1.5;
      vage = 0.002f;
      break;
    case StarModes::fountain:
      stats.fountainFxChosen();
      data->maxAge *= 2.0 / 3.0;
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
    addABomb(data, mx, my, radius, vage, gravity);
  }
}

/**
 * Main methode of the FX.
 */

inline uint32_t getLowColor(const FSData* data, const size_t starNum, const float tmix)
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
  return getBrighterColor(brightness, data->stars[starNum].currentLowColorMap->getColor(tmix),
                          data->buffSettings.allowOverexposed);
}

static void fs_apply(VisualFX* _this, PluginInfo* goomInfo, Pixel* src, Pixel* dest)
{
  FSData* data = static_cast<FSData*>(_this->fx_data);

  if (!BVAL(data->enabled_bp))
  {
    return;
  }

  // Get the new parameters values
  data->min_age = 1.0f - static_cast<float>(IVAL(data->min_age_p)) / 100.0f;
  data->max_age = 1.0f - static_cast<float>(IVAL(data->max_age_p)) / 100.0f;
  FVAL(data->nbStars_p) = static_cast<float>(data->numStars) / static_cast<float>(data->maxStars);
  data->nbStars_p.change_listener(&data->nbStars_p);
  data->maxStars = static_cast<size_t>(IVAL(data->nbStars_limit_p));
  data->fx_mode = static_cast<StarModes>(IVAL(data->fx_mode_p));

  // look for events
  if (goomInfo->sound->getTimeSinceLastGoom() < 1)
  {
    fs_sound_event_occured(_this, goomInfo);
    if (getNRand(20) == 1)
    {
      // Give a slight weight towards noFx mode by using numFX + 2.
      const uint32_t newVal = getNRand(numFx + 2);
      const StarModes newMode = newVal >= numFx ? StarModes::noFx : static_cast<StarModes>(newVal);
      IVAL(data->fx_mode_p) = static_cast<int>(newMode);
      data->fx_mode_p.change_listener(&data->fx_mode_p);
    }
  }

  // update particules
  stats.updateStars();

  for (size_t i = 0; i < data->numStars; ++i)
  {
    updateStar(&data->stars[i]);

    // dead particule
    if (data->stars[i].age >= data->maxAge)
    {
      stats.deadStar();
      continue;
    }

    // choose the color of the particule
    const float tAge = data->stars[i].age / static_cast<float>(data->maxAge);
    const uint32_t color = data->stars[i].currentColorMap->getColor(tAge);
    const uint32_t lowColor = getLowColor(data, i, tAge);
    const float thicknessFactor = getRandInRange(1.0F, 3.0F);

    // draws the particule
    const int x0 = static_cast<int>(data->stars[i].x);
    const int y0 = static_cast<int>(data->stars[i].y);
    int x1 = x0;
    int y1 = y0;
    constexpr size_t numParts = 7;
    for (size_t j = 1; j <= numParts; j++)
    {
      const float t = static_cast<float>(j - 1) / static_cast<float>(numParts - 1);
      const uint32_t mixedColor = ColorMap::colorMix(color, lowColor, t);
      const int x2 = x0 - static_cast<int>(data->stars[i].vx * j);
      const int y2 = y0 - static_cast<int>(data->stars[i].vy * j);
      const uint8_t thickness = static_cast<uint8_t>(
          std::clamp(1u, 2u, static_cast<uint32_t>(thicknessFactor * (1.0 - t))));
      //      1u, 5u, static_cast<uint32_t>((1.0 - std::fabs(t - 0.5)) * getRandInRange(1.0F, 5.0F))));

      if (data->useSingleBufferOnly)
      {
        data->draw->line(dest, x1, y1, x2, y2, mixedColor, thickness);
      }
      else
      {
        const std::vector<Pixel> colors = {{.val = mixedColor}, {.val = lowColor}};
        Pixel* buffs[2] = {dest, src};
        data->draw->line(std::size(buffs), buffs, x1, y1, x2, y2, colors, thickness);
      }

      x1 = x2;
      y1 = y2;
    }
  }

  // look for dead particules
  for (size_t i = 0; i < data->numStars;)
  {
    if ((data->stars[i].x > static_cast<float>(goomInfo->screen.width + 64)) ||
        ((data->stars[i].vy >= 0) &&
         (data->stars[i].y - 16 * data->stars[i].vy > goomInfo->screen.height)) ||
        (data->stars[i].x < -64) || (data->stars[i].age >= data->maxAge))
    {
      data->stars[i] = data->stars[data->numStars - 1];
      data->numStars--;
      stats.removedDeadStar();
    }
    else
    {
      ++i;
    }
  }
}

static void fs_setBuffSettings(VisualFX* _this, const FXBuffSettings& settings)
{
  FSData* data = static_cast<FSData*>(_this->fx_data);
  data->buffSettings = settings;
  data->draw->setBuffIntensity(data->buffSettings.buffIntensity);
  data->draw->setAllowOverexposed(data->buffSettings.allowOverexposed);
}

VisualFX flying_star_create(void)
{
  return VisualFX{.init = fs_init,
                  .free = fs_free,
                  .setBuffSettings = fs_setBuffSettings,
                  .apply = fs_apply,
                  .save = nullptr,
                  .restore = nullptr,
                  .fx_data = nullptr,
                  .params = nullptr};
}

void flying_star_log_stats(VisualFX*, const StatsLogValueFunc logVal)
{
  stats.log(logVal);
}

} // namespace goom
