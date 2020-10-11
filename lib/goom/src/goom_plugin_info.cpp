#include "goom_plugin_info.h"

#include "goomutils/logging_control.h"
#include "sound_info.h"
// #undef NO_LOGGING
#include "goomutils/logging.h"

#include <cstddef>
#include <memory>

namespace goom
{

void plugin_info_init(PluginInfo* pp, size_t nbVisuals)
{
  logDebug("Starting plugin_info_init.");

  pp->sound = std::make_unique<SoundInfo>();

  pp->nbParams = 0;
  pp->nbVisuals = nbVisuals;
  pp->visuals = new VisualFX*[nbVisuals];

  /* data for the update loop */
  pp->update.lockvar = 0;
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
      .noisify = false,
      .noiseFactor = 1,
      .blockyWavy = false,
      .waveFreqFactor = ZoomFilterData::defaultWaveFreqFactor,
      .waveAmplitude = ZoomFilterData::defaultWaveAmplitude,
      .waveEffectType = ZoomFilterData::defaultWaveEffectType,
      .scrunchAmplitude = ZoomFilterData::defaultScrunchAmplitude,
      .speedwayAmplitude = ZoomFilterData::defaultSpeedwayAmplitude,
      .amuletteAmplitude = ZoomFilterData::defaultAmuletteAmplitude,
      .crystalBallAmplitude = ZoomFilterData::defaultCrystalBallAmplitude,
      .hypercosFreq = ZoomFilterData::defaultHypercosFreq,
      .hypercosAmplitude = ZoomFilterData::defaultHypercosAmplitude,
      .hPlaneEffectAmplitude = ZoomFilterData::defaultHPlaneEffectAmplitude,
      .vPlaneEffectAmplitude = ZoomFilterData::defaultVPlaneEffectAmplitude,
  };
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
    p->params[0] = p->sound->getParams();
    while (i--)
    {
      if (p->visuals[i]->params)
      {
        p->params[p->nbParams++] = *(p->visuals[i]->params);
      }
    }
  }
}

} // namespace goom
