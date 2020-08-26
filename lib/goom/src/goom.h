#ifndef _GOOM_H
#define _GOOM_H

#include "filters.h"
#include "goom_config.h"
#include "goom_plugin_info.h"

#include <cstdint>

constexpr size_t NB_FX = static_cast<size_t>(ZoomFilterMode::_size);

extern PluginInfo* goom_init(const uint16_t resx, const uint16_t resy, int seed);
extern void goom_set_resolution(PluginInfo* goomInfo, const uint16_t resx, const uint16_t resy);

/*
 * forceMode == 0 : do nothing
 * forceMode == -1 : lock the FX
 * forceMode == 1..NB_FX : force a switch to FX n# forceMode
 *
 * songTitle = pointer to the title of the song...
 *      - NULL if it is not the start of the song
 *      - only have a value at the start of the song
 */
extern uint32_t* goom_update(PluginInfo* goomInfo,
                             const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN],
                             const int forceMode,
                             const float fps,
                             const char* songTitle,
                             const char* message);

// returns 0 if the buffer wasn't accepted
int goom_set_screenbuffer(PluginInfo* goomInfo, void* buffer);

void goom_close(PluginInfo* goomInfo);

#endif
