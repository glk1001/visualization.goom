#include "drawmethods.h"
#include "goom.h"
#include "goom_fx.h"
#include "goom_plugin_info.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

static void setOptimizedMethods(PluginInfo* p)
{
  /* set default methods */
  p->methods.draw_line = draw_line;
  p->methods.zoom_filter = zoom_filter_c;
}

void plugin_info_init(PluginInfo* pp, size_t nbVisuals)
{
  PluginInfo p;

  p.sound.speedvar = p.sound.accelvar = p.sound.totalgoom = 0;
  p.sound.prov_max = 0;
  p.sound.goom_limit = 1;
  p.sound.allTimesMax = std::numeric_limits<int16_t>::min();
  p.sound.allTimesMin = std::numeric_limits<int16_t>::max();
  p.sound.allTimesPositiveMax = 1;

  p.sound.volume_p = secure_f_feedback("Sound Volume");
  p.sound.accel_p = secure_f_feedback("Sound Acceleration");
  p.sound.speed_p = secure_f_feedback("Sound Speed");
  p.sound.goom_limit_p = secure_f_feedback("Goom Limit");
  p.sound.last_goom_p = secure_f_feedback("Goom Detection");
  p.sound.last_biggoom_p = secure_f_feedback("Big Goom Detection");
  p.sound.goom_power_p = secure_f_feedback("Goom Power");

  p.sound.biggoom_speed_limit_p = secure_i_param("Big Goom Speed Limit");
  IVAL(p.sound.biggoom_speed_limit_p) = 10;
  IMIN(p.sound.biggoom_speed_limit_p) = 0;
  IMAX(p.sound.biggoom_speed_limit_p) = 100;
  ISTEP(p.sound.biggoom_speed_limit_p) = 1;

  p.sound.biggoom_factor_p = secure_i_param("Big Goom Factor");
  IVAL(p.sound.biggoom_factor_p) = 10;
  IMIN(p.sound.biggoom_factor_p) = 0;
  IMAX(p.sound.biggoom_factor_p) = 100;
  ISTEP(p.sound.biggoom_factor_p) = 1;

  p.sound.params = plugin_parameters("Sound", 11);

  p.nbParams = 0;
  p.nbVisuals = nbVisuals;
  p.visuals = (VisualFX**)malloc(sizeof(VisualFX*) * (size_t)nbVisuals);

  *pp = p;
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

  pp->numStates = maxNumStates;
  pp->maxStateSelect = 510;
  // clang-format off
  GoomState states[maxNumStates] = {
    {.drawIFS = 1, .drawPoints = 0, .drawTentacle = 1, .drawScope = 1, .farScope = 1, .minSelect =   0},
    {.drawIFS = 1, .drawPoints = 0, .drawTentacle = 0, .drawScope = 0, .farScope = 1, .minSelect = 101},
    {.drawIFS = 1, .drawPoints = 0, .drawTentacle = 0, .drawScope = 1, .farScope = 1, .minSelect = 141},
    {.drawIFS = 0, .drawPoints = 1, .drawTentacle = 1, .drawScope = 1, .farScope = 1, .minSelect = 201},
    {.drawIFS = 0, .drawPoints = 1, .drawTentacle = 1, .drawScope = 1, .farScope = 0, .minSelect = 261},
    {.drawIFS = 0, .drawPoints = 1, .drawTentacle = 1, .drawScope = 1, .farScope = 1, .minSelect = 331},
    {.drawIFS = 0, .drawPoints = 0, .drawTentacle = 1, .drawScope = 0, .farScope = 1, .minSelect = 401},
    {.drawIFS = 0, .drawPoints = 0, .drawTentacle = 0, .drawScope = 1, .farScope = 1, .minSelect = 451},
  };
  // clang-format on
  states[maxNumStates - 1].maxSelect = pp->maxStateSelect;
  for (size_t i = 0; i < maxNumStates - 1; i++)
  {
    states[i].maxSelect = states[i + 1].minSelect - 1;
  }
  for (size_t i = 0; i < maxNumStates; ++i)
  {
    pp->states[i] = states[i];
  }
  pp->curGStateIndex = 6;
  pp->curGState = &(pp->states[pp->curGStateIndex]);

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

  ZoomFilterData zfd{
      .mode = ZoomFilterMode::normalMode,
      .vitesse = 127,
      .pertedec = 8,
      .middleX = 16,
      .middleY = 1,
      .reverse = true,
      .hPlaneEffect = 0,
      .vPlaneEffect = 0,
      .waveEffect = false,
      .hypercosEffect = false,
      .noisify = 0,
  };
  pp->update.zoomFilterData = zfd;

  setOptimizedMethods(pp);

  for (size_t i = 0; i < 0xffff; i++)
  {
    pp->sintable[i] = static_cast<int>(
        1024 * sin(static_cast<double>(i) * 360 /
                   (sizeof(pp->sintable) / sizeof(pp->sintable[0]) - 1) * M_PI / 180.0) +
        0.5);
    /* sintable [us] = (int)(1024.0f * sin (us*2*3.31415f/0xffff)) ; */
  }
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
