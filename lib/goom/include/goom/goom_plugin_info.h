#ifndef _GOOM_PLUGIN_INFO_H
#define _GOOM_PLUGIN_INFO_H

#include "filters.h"
#include "goom_config.h"
#include "goom_graphic.h"
#include "sound_info.h"

#include <cstddef>
#include <cstdint>
#include <memory>

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
  struct Screen
  {
    uint32_t width;
    uint32_t height;
    uint32_t size; // == screen.height * screen.width.
  };

  struct GoomUpdate
  {
    int lockvar = 0; // pour empecher de nouveaux changements
    uint32_t loopvar = 0; // mouvement des points
    int stop_lines = 0;
    int ifs_incr = 1; // dessiner l'ifs (0 = non: > = increment)
    int decay_ifs = 0; // disparition de l'ifs
    int recay_ifs = 0; // dedisparition de l'ifs
    int cyclesSinceLastChange = 0; // nombre de Cycle Depuis Dernier Changement
    int drawLinesDuration = 80; // duree de la transition entre afficher les lignes ou pas
    int lineMode = 80; // l'effet lineaire a dessiner
    float switchMultAmount = 29.0 / 30.0;
    int switchIncrAmount = 0x7f;
    int switchIncr = 0x7f;
    float switchMult = 1.0;
    uint32_t stateSelectionBlocker = 0;
    int32_t previousZoomSpeed = 128;
    int timeOfTitleDisplay = 0;
    char titleText[1024];
    ZoomFilterData zoomFilterData{};
  };

  PluginInfo(const uint32_t width, const uint32_t height) noexcept;

  const Screen screen;
  GoomUpdate update;

  const SoundInfo& getSoundInfo() const;
  void processSoundSample(const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN]);

private:
  std::unique_ptr<SoundInfo> soundInfo;
};


inline PluginInfo::PluginInfo(const uint32_t width, const uint32_t height) noexcept
  : screen{width, height, width * height}, soundInfo{std::make_unique<SoundInfo>()}
{
}

inline const SoundInfo& PluginInfo::getSoundInfo() const
{
  return *soundInfo;
}

inline void PluginInfo::processSoundSample(const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN])
{
  soundInfo->processSample(data);
}

} // namespace goom
#endif
