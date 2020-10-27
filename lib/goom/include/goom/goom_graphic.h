#ifndef _GOOM_GRAPHIC_H
#define _GOOM_GRAPHIC_H

#include "goom_config.h"

#include <cstdint>

namespace goom
{

template<class T>
struct channel_limits
{
  static constexpr T min() noexcept { return T(); }
  static constexpr T max() noexcept { return T(); }
};
template<>
struct channel_limits<uint8_t>
{
  static constexpr uint8_t min() noexcept { return 0; }
  static constexpr uint8_t max() noexcept { return 255; }
};
template<>
struct channel_limits<uint32_t>
{
  static constexpr uint32_t min() noexcept { return channel_limits<uint8_t>::min(); }
  static constexpr uint32_t max() noexcept { return channel_limits<uint8_t>::max(); }
};
template<>
struct channel_limits<int32_t>
{
  static constexpr int32_t min() noexcept { return channel_limits<uint8_t>::min(); }
  static constexpr int32_t max() noexcept { return channel_limits<uint8_t>::max(); }
};
template<>
struct channel_limits<float>
{
  static constexpr float min() noexcept { return channel_limits<uint8_t>::min(); }
  static constexpr float max() noexcept { return channel_limits<uint8_t>::max(); }
};


#ifdef COLOR_BGRA
#define A_CHANNEL 0x000000FFu

#define ALPHA 3u
#define BLEU 2u
#define VERT 1u
#define ROUGE 0u

#else // not COLOR_BGRA
#define A_CHANNEL 0xFF000000u

#define BLEU 3u
#define VERT 2u
#define ROUGE 1u
#define ALPHA 0u

#endif /* COLOR_BGRA */


class Pixel
{
#ifdef COLOR_BGRA
  struct Channels
  {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;
  };
#else
  struct Channels
  {
    uint8_t a = 0;
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
  };
#endif /* COLOR_BGRA */

public:
  Pixel();
  explicit Pixel(const Channels&);
  explicit Pixel(const uint32_t val);
  explicit Pixel(const uint8_t cop[4]);

  uint8_t r() const;
  uint8_t g() const;
  uint8_t b() const;
  uint8_t a() const;

  void set_r(const uint8_t c);
  void set_g(const uint8_t c);
  void set_b(const uint8_t c);
  void set_a(const uint8_t c);

  uint32_t rgba() const;
  void set_rgba(const uint32_t v);

private:
  union
  {
    Channels channels;
    uint32_t intVal;
  } color;
};

inline Pixel::Pixel() : color{.channels{}}
{
}

inline Pixel::Pixel(const Channels& c) : color{.channels{c}}
{
}

inline Pixel::Pixel(const uint32_t v) : color{.intVal{v}}
{
}

inline uint8_t Pixel::r() const
{
  return color.channels.r;
}

inline void Pixel::set_r(const uint8_t c)
{
  color.channels.r = c;
}

inline uint8_t Pixel::g() const
{
  return color.channels.g;
}

inline void Pixel::set_g(const uint8_t c)
{
  color.channels.g = c;
}

inline uint8_t Pixel::b() const
{
  return color.channels.b;
}

inline void Pixel::set_b(const uint8_t c)
{
  color.channels.b = c;
}

inline uint8_t Pixel::a() const
{
  return color.channels.a;
}

inline void Pixel::set_a(const uint8_t c)
{
  color.channels.a = c;
}

inline uint32_t Pixel::rgba() const
{
  return color.intVal;
}

inline void Pixel::set_rgba(const uint32_t v)
{
  color.intVal = v;
}

} // namespace goom
#endif
