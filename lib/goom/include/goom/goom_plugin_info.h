#ifndef _GOOM_PLUGIN_INFO_H
#define _GOOM_PLUGIN_INFO_H

#include "filters.h"
#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_visual_fx.h"
#include "lines_fx.h"
#include "sound_info.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

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
  _size
};

/**
 * Allows FXs to know the current state of the plugin.
 */
struct PluginInfo
{
  // private data
  struct Screen
  {
    uint32_t width;
    uint32_t height;
    uint32_t size; // == screen.height * screen.width.
  };
  Screen screen;

  std::unique_ptr<SoundInfo> sound;

  std::vector<VisualFX*> visuals;
  std::vector<VisualFx*> newVisuals;
  //  std::vector<std::unique_ptr<VisualFX>> newVisuals;

  // The known FX
  VisualFX star_fx;
  VisualFX zoomFilter_fx;
  VisualFX ifs_fx;

  std::unique_ptr<VisualFx> convolve_fx;
  std::unique_ptr<VisualFx> tentacles_fx;
  std::unique_ptr<VisualFx> goomDots;

  // image buffers
  uint32_t* pixel;
  uint32_t* back;
  Pixel* p1;
  Pixel* p2;
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

void plugin_info_init(PluginInfo* p);

} // namespace goom
#endif
