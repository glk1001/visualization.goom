#ifndef VISUALIZATION_GOOM_GOOM_GRAPHIC_H
#define VISUALIZATION_GOOM_GOOM_GRAPHIC_H

#include "goom_config.h"

#include <algorithm>
#include <cereal/archives/json.hpp>
#include <cstdint>
#include <cstring>
#include <vector>

namespace GOOM
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
  explicit Pixel(const Channels& c);
  explicit Pixel(uint32_t val);

  [[nodiscard]] auto R() const -> uint8_t;
  [[nodiscard]] auto G() const -> uint8_t;
  [[nodiscard]] auto B() const -> uint8_t;
  [[nodiscard]] auto A() const -> uint8_t;

  void SetR(uint8_t c);
  void SetG(uint8_t c);
  void SetB(uint8_t c);
  void SetA(uint8_t c);

  [[nodiscard]] auto Rgba() const -> uint32_t;
  void SetRgba(uint32_t v);

  auto operator==(const Pixel& p) const -> bool;

  template<class Archive>
  void serialize(Archive&);

private:
  union Color
  {
    Channels channels;
    uint32_t intVal;
  };
  Color m_color;
};

class PixelBuffer
{
public:
  PixelBuffer(uint32_t width, uint32_t height) noexcept;
  ~PixelBuffer() noexcept = default;

  PixelBuffer(const PixelBuffer&) = delete;
  PixelBuffer(const PixelBuffer&&) = delete;
  auto operator=(const PixelBuffer&) -> PixelBuffer& = delete;
  auto operator=(const PixelBuffer&&) -> PixelBuffer& = delete;

  [[nodiscard]] auto GetWidth() const -> uint32_t;
  [[nodiscard]] auto GetHeight() const -> uint32_t;
  [[nodiscard]] auto GetBuffLen() const -> uint32_t;
  [[nodiscard]] auto GetBuffSize() const -> size_t;

  void Fill(const Pixel& c);
  void CopyTo(PixelBuffer& buff, uint32_t length) const;
  [[nodiscard]] auto GetIntBuff() const -> const uint32_t*;

  auto operator()(size_t pos) const -> const Pixel&;
  auto operator()(size_t pos) -> Pixel&;

  auto operator()(size_t x, size_t y) const -> const Pixel&;
  auto operator()(size_t x, size_t y) -> Pixel&;

private:
  const uint32_t m_width;
  const uint32_t m_height;
  const size_t m_buffSize;
  std::vector<Pixel> m_buff;

  [[nodiscard]] auto GetIntBuff() -> uint32_t*;

  void CopyTo(uint32_t* intBuff, uint32_t length) const;
  void CopyFrom(const uint32_t* intBuff, uint32_t length);
};

template<class Archive>
void Pixel::serialize(Archive& ar)
{
  ar(m_color.intVal);
}

inline Pixel::Pixel() : m_color{.channels{}}
{
}

inline Pixel::Pixel(const Channels& c) : m_color{.channels{c}}
{
}

inline Pixel::Pixel(const uint32_t v) : m_color{.intVal{v}}
{
}

inline auto Pixel::operator==(const Pixel& p) const -> bool
{
  return Rgba() == p.Rgba();
}

inline auto Pixel::R() const -> uint8_t
{
  return m_color.channels.r;
}

inline void Pixel::SetR(uint8_t c)
{
  m_color.channels.r = c;
}

inline auto Pixel::G() const -> uint8_t
{
  return m_color.channels.g;
}

inline void Pixel::SetG(uint8_t c)
{
  m_color.channels.g = c;
}

inline auto Pixel::B() const -> uint8_t
{
  return m_color.channels.b;
}

inline void Pixel::SetB(uint8_t c)
{
  m_color.channels.b = c;
}

inline auto Pixel::A() const -> uint8_t
{
  return m_color.channels.a;
}

inline void Pixel::SetA(uint8_t c)
{
  m_color.channels.a = c;
}

inline auto Pixel::Rgba() const -> uint32_t
{
  return m_color.intVal;
}

inline void Pixel::SetRgba(uint32_t v)
{
  m_color.intVal = v;
}

inline PixelBuffer::PixelBuffer(const uint32_t w, const uint32_t h) noexcept
  : m_width{w},
    m_height{h},
    m_buffSize{static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * sizeof(Pixel)},
    m_buff(m_width * m_height)
{
}

inline auto PixelBuffer::GetWidth() const -> uint32_t
{
  return m_width;
}

inline auto PixelBuffer::GetHeight() const -> uint32_t
{
  return m_height;
}

inline auto PixelBuffer::GetBuffLen() const -> uint32_t
{
  return m_width * m_height;
}

inline auto PixelBuffer::GetBuffSize() const -> size_t
{
  return m_buffSize;
}

inline void PixelBuffer::Fill(const Pixel& c)
{
  std::fill(m_buff.begin(), m_buff.end(), c);
}

inline auto PixelBuffer::GetIntBuff() const -> const uint32_t*
{
  return reinterpret_cast<const uint32_t*>(m_buff.data());
}

inline auto PixelBuffer::GetIntBuff() -> uint32_t*
{
  return reinterpret_cast<uint32_t*>(m_buff.data());
}

inline void PixelBuffer::CopyTo(PixelBuffer& buff, uint32_t length) const
{
  CopyTo(buff.GetIntBuff(), length);
}

inline void PixelBuffer::CopyTo(uint32_t* intBuff, const uint32_t length) const
{
  static_assert(sizeof(Pixel) == sizeof(uint32_t));
  std::memcpy(intBuff, GetIntBuff(), length * sizeof(Pixel));
}

inline void PixelBuffer::CopyFrom(const uint32_t* intBuff, const uint32_t length)
{
  static_assert(sizeof(Pixel) == sizeof(uint32_t));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
  std::memcpy(m_buff.data(), intBuff, length * sizeof(Pixel));
#pragma GCC diagnostic pop
}

inline auto PixelBuffer::operator()(const size_t pos) const -> const Pixel&
{
  return m_buff[pos];
}

inline auto PixelBuffer::operator()(const size_t pos) -> Pixel&
{
  return m_buff[pos];
}

inline auto PixelBuffer::operator()(const size_t x, const size_t y) const -> const Pixel&
{
  return (*this)(y * m_width + x);
}

inline auto PixelBuffer::operator()(const size_t x, const size_t y) -> Pixel&
{
  return (*this)(y * m_width + x);
}

} // namespace GOOM
#endif
