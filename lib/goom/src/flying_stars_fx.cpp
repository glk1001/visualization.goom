#include "drawmethods.h"
#include "goom_fx.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "goomutils/colormap.h"
#include "goomutils/colormap_enums.h"
#include "goomutils/mathutils.h"

#include <array>

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
constexpr int FIREWORKS_FX = 0;
constexpr int RAIN_FX = 1;
constexpr int FOUNTAIN_FX = 2;
constexpr int LAST_FX = 3;

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

  int fx_mode;
  unsigned int nbStars;

  unsigned int maxStars;
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void fs_init(VisualFX* _this, PluginInfo* info)
{
  FSData* data = new FSData;

  data->currentColorGroup = data->colorMaps.getRandomGroup();

  data->fx_mode = FIREWORKS_FX;
  data->maxStars = 4096;
  data->stars = new Star[data->maxStars];
  data->nbStars = 0;

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
  IVAL(data->fx_mode_p) = data->fx_mode;
  IMIN(data->fx_mode_p) = 1;
  IMAX(data->fx_mode_p) = 3;
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
  _this->fx_data = (void*)data;
}

#pragma GCC diagnostic pop

static void fs_free(VisualFX* _this)
{
  FSData* data = (FSData*)_this->fx_data;
  delete[] data->stars;
  free(data->params.params);
  delete data;
}

/**
 * Cree une nouvelle 'bombe', c'est a dire une particule appartenant a une fusee d'artifice.
 */
static void addABomb(
    FSData* data, int mx, int my, float radius, float vage, float gravity, PluginInfo* info)
{
  if (data->nbStars >= data->maxStars)
  {
    return;
  }
  data->nbStars++;

  const unsigned int i = data->nbStars - 1;
  data->stars[i].x = mx;
  data->stars[i].y = my;

  // TODO Get colormap based on current mode.
  data->stars[i].currentColorMap = &data->colorMaps.getRandomColorMap(data->currentColorGroup);

  float ro = radius * (float)goom_irand(info->gRandom, 100) / 100.0f;
  ro *= (float)goom_irand(info->gRandom, 100) / 100.0f + 1.0f;
  const unsigned int theta = goom_irand(info->gRandom, 256);

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
  FSData* data = (FSData*)_this->fx_data;

  data->currentColorGroup = data->colorMaps.getRandomGroup();

  data->maxAge = 10 + int(goom_irand(info->gRandom, 10));

  int max = (int)((1.0f + info->sound.goomPower) * goom_irand(info->gRandom, 150)) + 100;
  float radius =
      (1.0f + info->sound.goomPower) * (float)(goom_irand(info->gRandom, 150) + 50) / 300;
  float vage;
  float gravity = 0.02f;
  unsigned int mx;
  int my;

  switch (data->fx_mode)
  {
    case FIREWORKS_FX:
    {
      double dx;
      double dy;
      do
      {
        mx = goom_irand(info->gRandom, (uint32_t)info->screen.width);
        my = (int)goom_irand(info->gRandom, (uint32_t)info->screen.height);
        dx = ((int)mx - info->screen.width / 2);
        dy = (my - info->screen.height / 2);
      } while (dx * dx + dy * dy < (info->screen.height / 2) * (info->screen.height / 2));
      vage = data->max_age * (1.0f - info->sound.goomPower);
    }
    break;
    case RAIN_FX:
      mx = goom_irand(info->gRandom, (uint32_t)info->screen.width);
      if (mx > (uint32_t)info->screen.width / 2)
      {
        mx = (uint32_t)info->screen.width;
      }
      else
      {
        mx = 0;
      }
      my = -(info->screen.height / 3) -
           (int)goom_irand(info->gRandom, (uint32_t)info->screen.width / 3);
      radius *= 1.5;
      vage = 0.002f;
      break;
    case FOUNTAIN_FX:
      my = info->screen.height + 2;
      vage = 0.001f;
      radius += 1.0f;
      mx = (uint32_t)info->screen.width / 2;
      gravity = 0.04f;
      break;
    default:
      return;
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
    addABomb(data, (int)mx, my, radius, vage, gravity, info);
  }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/**
 * Main methode of the FX.
 */
static void fs_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* info)
{
  FSData* data = (FSData*)_this->fx_data;

  if (!BVAL(data->enabled_bp))
  {
    return;
  }

  /* Get the new parameters values */
  data->min_age = 1.0f - (float)IVAL(data->min_age_p) / 100.0f;
  data->max_age = 1.0f - (float)IVAL(data->max_age_p) / 100.0f;
  FVAL(data->nbStars_p) = (float)data->nbStars / (float)data->maxStars;
  data->nbStars_p.change_listener(&data->nbStars_p);
  data->maxStars = (unsigned int)IVAL(data->nbStars_limit_p);
  data->fx_mode = IVAL(data->fx_mode_p);

  /* look for events */
  if (info->sound.timeSinceLastGoom < 1)
  {
    fs_sound_event_occured(_this, info);
    if (goom_irand(info->gRandom, 20) == 1)
    {
      IVAL(data->fx_mode_p) = (int)goom_irand(info->gRandom, (LAST_FX * 3));
      data->fx_mode_p.change_listener(&data->fx_mode_p);
    }
  }

  /* update particules */
  for (unsigned i = 0; i < data->nbStars; ++i)
  {
    updateStar(&data->stars[i]);

    /* dead particule */
    if (data->stars[i].age >= data->maxAge)
    {
      continue;
    }

    /* choose the color of the particule */
    const float t = data->stars[i].age / float(data->maxAge);
    const uint32_t color = data->stars[i].currentColorMap->getColor(t);
    const uint32_t colorLow = starLowColors[size_t(t * float(numLowColors - 1))];

    /* draws the particule */
    const int x0 = int(data->stars[i].x);
    const int y0 = int(data->stars[i].y);
    int x1 = x0;
    int y1 = y0;
    constexpr size_t numParts = 7;
    for (size_t j = 1; j <= numParts; j++)
    {
      const float t = float(j - 1) / float(numParts - 1);
      const uint32_t col = ColorMap::colorMix(color, colorLow, t);
      const int x2 = x0 - int(data->stars[i].vx * j);
      const int y2 = y0 - int(data->stars[i].vy * j);
      draw_line(dest, x1, y1, x2, y2, col, info->screen.width, info->screen.height);
      x1 = x2;
      y1 = y2;
    }
  }

  /* look for dead particules */
  for (unsigned i = 0; i < data->nbStars;)
  {
    if ((data->stars[i].x > info->screen.width + 64) ||
        ((data->stars[i].vy >= 0) &&
         (data->stars[i].y - 16 * data->stars[i].vy > info->screen.height)) ||
        (data->stars[i].x < -64) || (data->stars[i].age >= data->maxAge))
    {
      data->stars[i] = data->stars[data->nbStars - 1];
      data->nbStars--;
    }
    else
    {
      ++i;
    }
  }
}

#pragma GCC diagnostic pop

VisualFX flying_star_create(void)
{
  VisualFX vfx;
  vfx.init = fs_init;
  vfx.free = fs_free;
  vfx.apply = fs_apply;
  vfx.save = NULL;
  vfx.restore = NULL;
  vfx.fx_data = 0;

  return vfx;
}
