#ifndef _GOOM_CONFIG_H
#define _GOOM_CONFIG_H

#include <functional>
#include <string>

using StatsLogValueFunc = std::function<void(
    const std::string& module, const std::string& name, const uint32_t value)>;

#define NUM_AUDIO_SAMPLES 2
#define AUDIO_SAMPLE_LEN 512

#ifdef WORDS_BIGENDIAN
#define COLOR_ARGB
#else
#define COLOR_BGRA
#endif

#endif
