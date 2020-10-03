#ifndef _GOOM_CONFIG_H
#define _GOOM_CONFIG_H

#include <cstdint>
#include <functional>
#include <string>
#include <variant>

namespace goom
{

using StatsLogValue = std::variant<std::string, uint32_t, int32_t, float>;
using StatsLogValueFunc =
    std::function<void(const std::string& module, const std::string& name, const StatsLogValue&)>;

#define NUM_AUDIO_SAMPLES 2
#define AUDIO_SAMPLE_LEN 512

#ifdef WORDS_BIGENDIAN
#define COLOR_ARGB
#else
#define COLOR_BGRA
#endif

} // namespace goom
#endif
