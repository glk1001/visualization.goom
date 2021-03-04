#ifndef VISUALIZATION_GOOM_V2D_H
#define VISUALIZATION_GOOM_V2D_H

#include <cstdint>

namespace GOOM
{

struct V2dInt
{
  int32_t x;
  int32_t y;
};

struct V2dFlt
{
  float x;
  float y;
  auto operator+=(const V2dFlt& v) -> V2dFlt&;
  auto operator*=(float a) -> V2dFlt&;
  auto operator*(float a) const -> V2dFlt;
};

auto operator*(float a, const V2dFlt& v) -> V2dFlt;

inline auto V2dFlt::operator+=(const V2dFlt& v) -> V2dFlt&
{
  x += v.x;
  y += v.y;
  return *this;
}

inline auto V2dFlt::operator*=(const float a) -> V2dFlt&
{
  x *= a;
  y *= a;
  return *this;
}

inline auto V2dFlt::operator*(const float a) const -> V2dFlt
{
  return {a * x, a * y};
}

inline auto operator*(const float a, const V2dFlt& v) -> V2dFlt
{
  return v * a;
}

} // namespace GOOM
#endif
