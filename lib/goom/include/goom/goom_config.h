#ifndef VISUALIZATION_GOOM_GOOM_CONFIG_H
#define VISUALIZATION_GOOM_GOOM_CONFIG_H

#include <cstdint>
#include <functional>
#include <string>
#include <variant>

namespace GOOM
{

#if __cplusplus > 201402L
using StatsLogValue = std::variant<std::string, uint32_t, int32_t, uint64_t, float>;
#else
using StatsLogValue = int; // disabled
#endif

using StatsLogValueFunc =
    std::function<void(const std::string& module, const std::string& name, const StatsLogValue&)>;

constexpr auto IMAGES_DIR = "images";

#define NUM_AUDIO_SAMPLES 2
#define AUDIO_SAMPLE_LEN 512

#ifdef WORDS_BIGENDIAN
#define COLOR_ARGB
#else
#define COLOR_BGRA
#endif

} // namespace GOOM
#endif
