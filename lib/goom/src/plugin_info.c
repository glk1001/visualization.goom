#include "drawmethods.h"
#include "goom.h"
#include "goom_fx.h"
#include "goom_plugin_info.h"

#include <math.h>
#include <stdio.h>

static void setOptimizedMethods(PluginInfo* p)
{
  /* set default methods */
  p->methods.draw_line = draw_line;
  p->methods.zoom_filter = zoom_filter_c;
  /*    p->methods.create_output_with_brightness = create_output_with_brightness;*/
}

void plugin_info_init(PluginInfo* pp, int nbVisuals)
{
  PluginInfo p;

  p.sound.speedvar = p.sound.accelvar = p.sound.totalgoom = 0;
  p.sound.prov_max = 0;
  p.sound.goom_limit = 1;
  p.sound.allTimesMax = 1;

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

  pp->statesNumber = STATES_MAX_NB;
  pp->statesRangeMax = 510;
  GoomState states[STATES_MAX_NB] = {{1, 0, 1, 1, 4,   0, 100},
                                     {1, 0, 1, 0, 1, 101, 140},
                                     {1, 0, 1, 1, 2, 141, 200},
                                     {0, 1, 1, 1, 2, 201, 260},
                                     {0, 1, 1, 1, 0, 261, 330},
                                     {0, 1, 1, 1, 4, 331, 400},
                                     {0, 0, 1, 0, 5, 401, 450},
                                     {0, 0, 1, 1, 1, 451, 510}
                                    };
  for (int i = 0; i < STATES_MAX_NB; ++i) {
    pp->states[i] = states[i];
  }
  pp->curGStateIndex = 6;
  pp->curGState = &(pp->states[pp->curGStateIndex]);

  /* data for the update loop */
  pp->update.lockvar = 0;
  pp->update.goomvar = 0;
  pp->update.loopvar = 0;
  pp->update.stop_lines = 0;
  pp->update.ifs_incr = 1;  /* dessiner l'ifs (0 = non: > = increment) */
  pp->update.decay_ifs = 0; /* disparition de l'ifs */
  pp->update.recay_ifs = 0; /* dedisparition de l'ifs */
  pp->update.cyclesSinceLastChange = 0;
  pp->update.drawLinesDuration = 80;
  pp->update.lineMode = pp->update.drawLinesDuration;

  pp->update.switchMultAmount = (29.0f / 30.0f);
  pp->update.switchIncrAmount = 0x7f;
  pp->update.switchMult = 1.0f;
  pp->update.switchIncr = pp->update.switchIncrAmount;

  pp->update.stateSelectionRnd = 0;
  pp->update.stateSelectionBlocker = 0;
  pp->update.previousZoomSpeed = 128;
  pp->update.timeOfTitleDisplay = 0;

  pp->update_message.affiche = 0;

  ZoomFilterData zfd = {127, 8, 16, 1, 1, 0, NORMAL_MODE, 0, 0, 0, 0};
  pp->update.zoomFilterData = zfd;

  setOptimizedMethods(pp);

  for (int i = 0; i < 0xffff; i++) {
    pp->sintable[i] =
        (int)(1024 * sin((double)i * 360 / (sizeof(pp->sintable) / sizeof(pp->sintable[0]) - 1) *
                         3.141592 / 180) +
              .5);
    /* sintable [us] = (int)(1024.0f * sin (us*2*3.31415f/0xffff)) ; */
  }
}

void plugin_info_add_visual(PluginInfo* p, int i, VisualFX* visual)
{
  p->visuals[i] = visual;
  if (i == p->nbVisuals - 1) {
    ++i;
    p->nbParams = 1;
    while (i--) {
      if (p->visuals[i]->params) {
        p->nbParams++;
      }
    }
    p->params = (PluginParameters*)malloc(sizeof(PluginParameters) * (size_t)p->nbParams);
    i = p->nbVisuals;
    p->nbParams = 1;
    p->params[0] = p->sound.params;
    while (i--) {
      if (p->visuals[i]->params) {
        p->params[p->nbParams++] = *(p->visuals[i]->params);
      }
    }
  }
}
