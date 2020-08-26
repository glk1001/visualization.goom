#ifndef _SOUND_TESTER_H
#define _SOUND_TESTER_H

#include "goom_config.h"
#include "goom_plugin_info.h"

#include <cstdint>

// change les donnees du SoundInfo
void evaluate_sound(const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN], SoundInfo* sndInfo);

#endif
