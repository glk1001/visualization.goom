#ifndef _GOOM_PLUGIN_INFO_H
#define _GOOM_PLUGIN_INFO_H

#include "filters.h"
#include "goom_config.h"
#include "goom_config_param.h"
#include "goom_graphic.h"
#include "goom_visual_fx.h"
#include "lines.h"

#include <cstddef>
#include <cstdint>
#include <unordered_set>

namespace goom
{

enum class GoomDrawable
{
  IFS = 0,
  dots,
  tentacles,
  stars,
  lines,
  scope,
  farScope,
  _size // must be last - gives number of enums
};

/**
 * Gives information about the sound.
 */
struct SoundInfo
{
  // Note: a Goom is just a sound event...
  int16_t samples[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN];

  uint32_t timeSinceLastBigGoom; // >= 0
  uint32_t timeSinceLastGoom; // >= 0
  float goomPower; // power of the last Goom [0..1]
  float volume; // [0..1]

  // other "internal" data for the sound_tester
  float goom_limit; // auto-updated limit of goom_detection
  float bigGoomLimit;
  float accelvar; // acceleration of the sound - [0..1]
  float speedvar; // speed of the sound - [0..100]
  int16_t allTimesMax;
  int16_t allTimesMin;
  int16_t allTimesPositiveMax;
  uint32_t totalgoom; // number of goom since last reset (a reset every 64 cycles)

  float prov_max; // accel max since last reset
  int cycle;

  // private data
  PluginParam volume_p;
  PluginParam speed_p;
  PluginParam accel_p;
  PluginParam goom_limit_p;
  PluginParam goom_power_p;
  PluginParam last_goom_p;
  PluginParam last_biggoom_p;
  PluginParam biggoom_speed_limit_p;
  PluginParam biggoom_factor_p;

  PluginParameters params; // contains the previously defined parameters.
};

/**
 * Allows FXs to know the current state of the plugin.
 */
struct PluginInfo
{
  // public data
  size_t nbParams;
  PluginParameters* params;

  // private data
  struct Screen
  {
    uint32_t width;
    uint32_t height;
    uint32_t size; // == screen.height * screen.width.
  };
  Screen screen;

  SoundInfo sound;

  size_t nbVisuals;
  VisualFX** visuals; // pointers on all the visual fx

  // The known FX
  VisualFX convolve_fx;
  VisualFX star_fx;
  VisualFX zoomFilter_fx;
  VisualFX tentacles_fx;
  VisualFX ifs_fx;

  // image buffers
  uint32_t* pixel;
  uint32_t* back;
  Pixel* p1;
  Pixel* p2;
  Pixel* conv;
  Pixel* outputBuf;

  // state of goom
  uint32_t cycle;
  std::unordered_set<GoomDrawable> curGDrawables;

  // effet de ligne.
  GMLine* gmline1;
  GMLine* gmline2;

  // INTERNALS

  /** goom_update internals.
    * I took all static variables from goom_update and put them here.. for the moment.
    */
  struct GoomUpdate
  {
    int lockvar; // pour empecher de nouveaux changements
    int goomvar; // boucle des gooms
    uint32_t loopvar; // mouvement des points
    int stop_lines;
    int ifs_incr; // dessiner l'ifs (0 = non: > = increment)
    int decay_ifs; // disparition de l'ifs
    int recay_ifs; // dedisparition de l'ifs
    int cyclesSinceLastChange; // nombre de Cycle Depuis Dernier Changement
    int drawLinesDuration; // duree de la transition entre afficher les lignes ou pas
    int lineMode; // l'effet lineaire a dessiner
    float switchMultAmount; // SWITCHMULT (29.0f/30.0f)
    int switchIncrAmount; // 0x7f
    float switchMult; // 1.0f
    int switchIncr; //  = SWITCHINCR;
    uint32_t stateSelectionBlocker;
    int32_t previousZoomSpeed;
    int timeOfTitleDisplay;
    char titleText[1024];
    ZoomFilterData zoomFilterData;
  };
  GoomUpdate update;

  struct
  {
    std::string message;
    uint32_t numberOfLinesInMessage;
    uint32_t affiche;
  } update_message;
};

void plugin_info_init(PluginInfo* p, size_t nbVisual);

// i = [0..p->nbVisual-1]
void plugin_info_add_visual(PluginInfo* p, size_t i, VisualFX* visual);

} // namespace goom
#endif
