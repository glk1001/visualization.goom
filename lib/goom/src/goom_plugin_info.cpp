#include "goom_plugin_info.h"

#include "drawmethods.h"
#include "goom.h"
#include "goom_fx.h"
#include "goomutils/logging_control.h"
// #undef NO_LOGGING
#include "goomutils/logging.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <unordered_set>

static void setOptimizedMethods(PluginInfo* p)
{
  /* set default methods */
  p->methods.draw_line = draw_line;
  p->methods.zoom_filter = zoom_filter_c;
}

void plugin_info_init(PluginInfo* pp, size_t nbVisuals)
{
  logDebug("Starting plugin_info_init.");

  pp->sound.timeSinceLastBigGoom = 0;
  pp->sound.timeSinceLastGoom = 0;
  pp->sound.goomPower = 0;
  pp->sound.volume = 0;

  pp->sound.goom_limit = 1;
  pp->sound.bigGoomLimit = 1;
  pp->sound.accelvar = 0;
  pp->sound.speedvar = 0;
  pp->sound.allTimesMax = std::numeric_limits<int16_t>::min();
  pp->sound.allTimesMin = std::numeric_limits<int16_t>::max();
  pp->sound.allTimesPositiveMax = 1;
  pp->sound.totalgoom = 0;
  pp->sound.prov_max = 0;
  pp->sound.cycle = 0;

  pp->sound.volume_p = secure_f_feedback("Sound Volume");
  pp->sound.accel_p = secure_f_feedback("Sound Acceleration");
  pp->sound.speed_p = secure_f_feedback("Sound Speed");
  pp->sound.goom_limit_p = secure_f_feedback("Goom Limit");
  pp->sound.last_goom_p = secure_f_feedback("Goom Detection");
  pp->sound.last_biggoom_p = secure_f_feedback("Big Goom Detection");
  pp->sound.goom_power_p = secure_f_feedback("Goom Power");

  pp->sound.biggoom_speed_limit_p = secure_i_param("Big Goom Speed Limit");
  IVAL(pp->sound.biggoom_speed_limit_p) = 10;
  IMIN(pp->sound.biggoom_speed_limit_p) = 0;
  IMAX(pp->sound.biggoom_speed_limit_p) = 100;
  ISTEP(pp->sound.biggoom_speed_limit_p) = 1;

  pp->sound.biggoom_factor_p = secure_i_param("Big Goom Factor");
  IVAL(pp->sound.biggoom_factor_p) = 10;
  IMIN(pp->sound.biggoom_factor_p) = 0;
  IMAX(pp->sound.biggoom_factor_p) = 100;
  ISTEP(pp->sound.biggoom_factor_p) = 1;

  pp->sound.params = plugin_parameters("Sound", 11);

  pp->nbParams = 0;
  pp->nbVisuals = nbVisuals;
  pp->visuals = (VisualFX**)malloc(sizeof(VisualFX*) * (size_t)nbVisuals);

  pp->sound.params.params[0] = &pp->sound.biggoom_speed_limit_p;
  pp->sound.params.params[1] = &pp->sound.biggoom_factor_p;
  pp->sound.params.params[2] = 0;
  pp->sound.params.params[3] = &pp->sound.volume_p;
  pp->sound.params.params[4] = &pp->sound.accel_p;
  pp->sound.params.params[5] = &pp->sound.speed_p;
  pp->sound.params.params[6] = 0;
  pp->sound.params.params[7] = &pp->sound.goom_limit_p;
  pp->sound.params.params[8] = &pp->sound.goom_power_p;
  pp->sound.params.params[9] = &pp->sound.last_goom_p;
  pp->sound.params.params[10] = &pp->sound.last_biggoom_p;

  pp->maxStateSelect = 510;
  // clang-format off
  using GD = GoomDrawable;
  pp->states = {
    { .minSel =   0, .drawables = {GD::IFS,                            GD::stars,            GD::scope, GD::farScope}},
    { .minSel = 101, .drawables = {GD::IFS,             GD::tentacles, GD::stars,                       GD::farScope}},
    { .minSel = 141, .drawables = {GD::IFS,                            GD::stars, GD::lines, GD::scope, GD::farScope}},
    { .minSel = 201, .drawables = {         GD::points, GD::tentacles, GD::stars, GD::lines, GD::scope, GD::farScope}},
    { .minSel = 261, .drawables = {         GD::points, GD::tentacles, GD::stars,            GD::scope              }},
    { .minSel = 331, .drawables = {         GD::points, GD::tentacles, GD::stars, GD::lines, GD::scope, GD::farScope}},
    { .minSel = 401, .drawables = {                     GD::tentacles, GD::stars, GD::lines,            GD::farScope}},
    { .minSel = 451, .drawables = {                                    GD::stars, GD::lines, GD::scope, GD::farScope}},
  };
  // clang-format on
  pp->numStates = pp->states.size();
  pp->states[pp->states.size() - 1].maxSel = pp->maxStateSelect;
  for (size_t i = 0; i < pp->states.size() - 1; i++)
  {
    pp->states[i].maxSel = pp->states[i + 1].minSel - 1;
  }
  pp->curGStateIndex = pp->numStates;
  pp->curGState = nullptr;
  pp->curGDrawables = std::unordered_set<GoomDrawable>{};

  /* data for the update loop */
  pp->update.lockvar = 0;
  pp->update.goomvar = 0;
  pp->update.loopvar = 0;
  pp->update.stop_lines = 0;
  pp->update.ifs_incr = 1; /* dessiner l'ifs (0 = non: > = increment) */
  pp->update.decay_ifs = 0; /* disparition de l'ifs */
  pp->update.recay_ifs = 0; /* dedisparition de l'ifs */
  pp->update.cyclesSinceLastChange = 0;
  pp->update.drawLinesDuration = 80;
  pp->update.lineMode = pp->update.drawLinesDuration;

  pp->update.switchMultAmount = (29.0f / 30.0f);
  pp->update.switchIncrAmount = 0x7f;
  pp->update.switchMult = 1.0f;
  pp->update.switchIncr = pp->update.switchIncrAmount;

  pp->update.stateSelectionRand = 0;
  pp->update.stateSelectionBlocker = 0;
  pp->update.previousZoomSpeed = 128;
  pp->update.timeOfTitleDisplay = 0;

  pp->update_message.affiche = 0;

  pp->update.zoomFilterData = {
      .mode = ZoomFilterMode::crystalBallMode,
      .vitesse = 127,
      .pertedec = 8,
      .middleX = 16,
      .middleY = 1,
      .reverse = true,
      .hPlaneEffect = 0,
      .vPlaneEffect = 0,
      .waveEffect = false,
      .hypercosEffect = false,
      .perlinNoisify = false,
      .noisify = false,
  };

  setOptimizedMethods(pp);
}

void plugin_info_add_visual(PluginInfo* p, size_t i, VisualFX* visual)
{
  p->visuals[i] = visual;
  if (i == p->nbVisuals - 1)
  {
    ++i;
    p->nbParams = 1;
    while (i--)
    {
      if (p->visuals[i]->params)
      {
        p->nbParams++;
      }
    }
    p->params = (PluginParameters*)malloc(sizeof(PluginParameters) * p->nbParams);
    i = p->nbVisuals;
    p->nbParams = 1;
    p->params[0] = p->sound.params;
    while (i--)
    {
      if (p->visuals[i]->params)
      {
        p->params[p->nbParams++] = *(p->visuals[i]->params);
      }
    }
  }
}
