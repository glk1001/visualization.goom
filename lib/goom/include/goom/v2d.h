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
};

inline auto V2dFlt::operator+=(const V2dFlt& v) -> V2dFlt&
{
  x += v.x;
  y += v.y;
  return *this;
}

} // namespace GOOM
#endif
