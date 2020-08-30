#include "drawmethods.h"
#include "goom_fx.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "goomutils/colormap.h"
#include "goomutils/colormap_enums.h"
#include "goomutils/mathutils.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

/* TODO:-- FAIRE PROPREMENT... BOAH... */
// clang-format off
constexpr size_t numLowColors = 15;
constexpr std::array<uint32_t, numLowColors> starLowColors{
  0x1416181a, 0x1419181a, 0x141f181a, 0x1426181a, 0x142a181a,
  0x142f181a, 0x1436181a, 0x142f1819, 0x14261615, 0x13201411,
  0x111a100a, 0x0c180508, 0x08100304, 0x00050101, 0x0
};
// clang-format on

/* The different modes of the visual FX.
 * Put this values on fx_mode */
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
};

struct FSData
{
  PluginParam enabled_bp;

  ColorMaps colorMaps;
  ColorMapGroup currentColorGroup;

  int maxAge = 15;

  StarModes fx_mode;
  size_t numStars;

  size_t maxStars;
  Star* stars;

  float min_age;
  float max_age;

  PluginParam min_age_p;
  PluginParam max_age_p;
  PluginParam nbStars_p;
  PluginParam nbStars_limit_p;
  PluginParam fx_mode_p;

  PluginParameters params;
};

static void fs_init(VisualFX* _this, PluginInfo*)
{
  FSData* data = new FSData;

  data->currentColorGroup = data->colorMaps.getRandomGroup();

  data->fx_mode = StarModes::fireworks;
  data->maxStars = 4096;
  data->stars = new Star[data->maxStars];
  data->numStars = 0;

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
  IMAX(data->nbStars_limit_p) = (int)data->maxStars;
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
                     const float gravity,
                     PluginInfo* info)
{
  if (data->numStars >= data->maxStars)
  {
    return;
  }
  data->numStars++;

  const unsigned int i = data->numStars - 1;
  data->stars[i].x = mx;
  data->stars[i].y = my;

  // TODO Get colormap based on current mode.
  data->stars[i].currentColorMap = &data->colorMaps.getRandomColorMap(data->currentColorGroup);

  float ro = radius * static_cast<float>(goom_irand(info->gRandom, 100)) / 100.0f;
  ro *= static_cast<float>(goom_irand(info->gRandom, 100)) / 100.0f + 1.0f;
  const uint32_t theta = goom_irand(info->gRandom, 256);

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
static void fs_sound_event_occured(VisualFX* _this, PluginInfo* info)
{
  FSData* data = static_cast<FSData*>(_this->fx_data);

  data->currentColorGroup = data->colorMaps.getRandomGroup();

  data->maxAge = 10 + int(goom_irand(info->gRandom, 10));

  int max = static_cast<int>((1.0f + info->sound.goomPower) * goom_irand(info->gRandom, 150)) + 100;
  float radius = (1.0f + info->sound.goomPower) *
                 static_cast<float>(goom_irand(info->gRandom, 150) + 50) / 300;
  float gravity = 0.02f;
  uint32_t mx;
  uint32_t my;
  float vage;

  switch (data->fx_mode)
  {
    case StarModes::noFx:
      return;
    case StarModes::fireworks:
    {
      double dx;
      double dy;
      do
      {
        mx = goom_irand(info->gRandom, info->screen.width);
        my = goom_irand(info->gRandom, info->screen.height);
        dx = (mx - info->screen.width / 2);
        dy = (my - info->screen.height / 2);
      } while (dx * dx + dy * dy < (info->screen.height / 2) * (info->screen.height / 2));
      vage = data->max_age * (1.0f - info->sound.goomPower);
    }
    break;
    case StarModes::rain:
      mx = goom_irand(info->gRandom, info->screen.width);
      mx = (mx <= info->screen.width / 2) ? 0 : info->screen.width;
      my = -(info->screen.height / 3) - goom_irand(info->gRandom, info->screen.width / 3);
      radius *= 1.5;
      vage = 0.002f;
      break;
    case StarModes::fountain:
      my = info->screen.height + 2;
      vage = 0.001f;
      radius += 1.0f;
      mx = info->screen.width / 2;
      gravity = 0.04f;
      break;
    default:
      throw std::logic_error("Unknown StarModes enum.");
      /* my = i R A N D (info->screen.height); vage = 0.01f; */
  }

  radius *= info->screen.height / 200.0f; /* why 200 ? because the FX was developed on 320x200 */
  max *= info->screen.height / 200.0f;

  if (info->sound.timeSinceLastBigGoom < 1)
  {
    radius *= 1.5;
    max *= 2;
  }
  for (int i = 0; i < max; ++i)
  {
    addABomb(data, mx, my, radius, vage, gravity, info);
  }
}

/**
 * Main methode of the FX.
 */
static void fs_apply(VisualFX* _this, [[maybe_unused]] Pixel* src, Pixel* dest, PluginInfo* info)
{
  FSData* data = static_cast<FSData*>(_this->fx_data);

  if (!BVAL(data->enabled_bp))
  {
    return;
  }

  /* Get the new parameters values */
  data->min_age = 1.0f - static_cast<float>(IVAL(data->min_age_p)) / 100.0f;
  data->max_age = 1.0f - static_cast<float>(IVAL(data->max_age_p)) / 100.0f;
  FVAL(data->nbStars_p) = static_cast<float>(data->numStars) / static_cast<float>(data->maxStars);
  data->nbStars_p.change_listener(&data->nbStars_p);
  data->maxStars = static_cast<size_t>(IVAL(data->nbStars_limit_p));
  data->fx_mode = static_cast<StarModes>(IVAL(data->fx_mode_p));

  /* look for events */
  if (info->sound.timeSinceLastGoom < 1)
  {
    fs_sound_event_occured(_this, info);
    if (goom_irand(info->gRandom, 20) == 1)
    {
      // Give a sleight weight towards noFx mode by using numFX + 2.
      const uint32_t newVal = goom_irand(info->gRandom, numFx + 2);
      const StarModes newMode = newVal >= numFx ? StarModes::noFx : static_cast<StarModes>(newVal);
      IVAL(data->fx_mode_p) = static_cast<int>(newMode);
      data->fx_mode_p.change_listener(&data->fx_mode_p);
    }
  }

  /* update particules */
  for (size_t i = 0; i < data->numStars; ++i)
  {
    updateStar(&data->stars[i]);

    /* dead particule */
    if (data->stars[i].age >= data->maxAge)
    {
      continue;
    }

    /* choose the color of the particule */
    const float t = data->stars[i].age / static_cast<float>(data->maxAge);
    const uint32_t color = data->stars[i].currentColorMap->getColor(t);
    const uint32_t colorLow = starLowColors[size_t(t * static_cast<float>(numLowColors - 1))];

    /* draws the particule */
    const int x0 = static_cast<int>(data->stars[i].x);
    const int y0 = static_cast<int>(data->stars[i].y);
    int x1 = x0;
    int y1 = y0;
    constexpr size_t numParts = 7;
    for (size_t j = 1; j <= numParts; j++)
    {
      const float t = static_cast<float>(j - 1) / static_cast<float>(numParts - 1);
      const uint32_t col = ColorMap::colorMix(color, colorLow, t);
      const int x2 = x0 - static_cast<int>(data->stars[i].vx * j);
      const int y2 = y0 - static_cast<int>(data->stars[i].vy * j);
      draw_line(dest, x1, y1, x2, y2, col, info->screen.width, info->screen.height);
      x1 = x2;
      y1 = y2;
    }
  }

  /* look for dead particules */
  for (size_t i = 0; i < data->numStars;)
  {
    if ((data->stars[i].x > info->screen.width + 64) ||
        ((data->stars[i].vy >= 0) &&
         (data->stars[i].y - 16 * data->stars[i].vy > info->screen.height)) ||
        (data->stars[i].x < -64) || (data->stars[i].age >= data->maxAge))
    {
      data->stars[i] = data->stars[data->numStars - 1];
      data->numStars--;
    }
    else
    {
      ++i;
    }
  }
}

VisualFX flying_star_create(void)
{
  return VisualFX{.init = fs_init,
                  .free = fs_free,
                  .apply = fs_apply,
                  .save = nullptr,
                  .restore = nullptr,
                  .fx_data = nullptr,
                  .params = nullptr};
}
