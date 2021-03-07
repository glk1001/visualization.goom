#ifndef VISUALIZATION_GOOM_GOOM_CONFIG_H
#define VISUALIZATION_GOOM_GOOM_CONFIG_H

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#if __cplusplus > 201402L
#include <variant>
#endif

namespace GOOM
{

#if __cplusplus <= 201402L
struct StatsLogValue
{
  StatsLogValue(std::string s_val) : type{Type::STRING}, str{std::move(s_val)} {}
  StatsLogValue(const uint32_t i) : type{Type::UINT32}, ui32{i} {}
  StatsLogValue(const int32_t i) : type{Type::INT32}, i32{i} {}
  StatsLogValue(const uint64_t i) : type{Type::UINT64}, ui64{i} {}
  StatsLogValue(const float f) : type{Type::FLOAT}, flt{f} {}
  enum class Type
  {
    STRING,
    UINT32,
    INT32,
    UINT64,
    FLOAT
  } type;
  std::string str{};
  uint32_t ui32{};
  int32_t i32{};
  uint64_t ui64{};
  float flt{};
};
#else
using StatsLogValue = std::variant<std::string, uint32_t, int32_t, uint64_t, float>;
#endif

using StatsLogValueFunc =
    std::function<void(const std::string& module, const std::string& name, const StatsLogValue&)>;

#ifdef _WIN32PC
constexpr const char* PATH_SEP = "\\";
#else
constexpr const char* PATH_SEP = "/";
#endif

constexpr auto FONTS_DIR = "fonts";
constexpr auto IMAGES_DIR = "images";
constexpr auto IMAGE_DISPLACEMENT_DIR = "displacements";

#define NUM_AUDIO_SAMPLES 2
#define AUDIO_SAMPLE_LEN 512

#ifdef WORDS_BIGENDIAN
#define COLOR_ARGB
#else
#define COLOR_BGRA
#endif

} // namespace GOOM
#endif
