#include "goomutils/goomrand.h"

#include "goomutils/splitmix.hpp"
#include "goomutils/xoshiro.hpp"

#undef NDEBUG

#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

namespace goom::utils
{

const uint32_t randMax = (xoshiro256plus64::max() > std::numeric_limits<uint32_t>::max())
                             ? std::numeric_limits<uint32_t>::max()
                             : xoshiro256plus64::max();
thread_local uint64_t randSeed = std::random_device{}();

uint64_t getRandSeed()
{
  return randSeed;
}

void setRandSeed(const uint64_t seed)
{
  randSeed = seed;
}

// NOTE: C++ std::uniform_int_distribution is too expensive (about double) so we use Xoshiro
//   and multiplication/shift technique. For timings, see tests/test_goomrand.cpp.
inline uint32_t randXoshiroFunc(const uint32_t n0, const uint32_t n1)
{
  // thread_local xoshiro256starstar64 eng { getRandSeed() };
  thread_local xoshiro256plus64 eng{getRandSeed()};
  const uint32_t x = eng();
  const uint64_t m = (static_cast<uint64_t>(x) * static_cast<uint64_t>(n1 - n0)) >> 32;
  return n0 + static_cast<uint32_t>(m);
}

inline uint32_t randSplitmixFunc(const uint32_t n0, const uint32_t n1)
{
  // thread_local splitmix32 eng { getRandSeed() };
  thread_local splitmix64 eng{getRandSeed()};
  const uint32_t x = eng();
  const uint64_t m = (static_cast<uint64_t>(x) * static_cast<uint64_t>(n1 - n0)) >> 32;
  return n0 + static_cast<uint32_t>(m);
}

uint32_t getRandInRange(const uint32_t n0, const uint32_t n1)
{
#ifndef NDEBUG
  if (n0 >= n1)
  {
    throw std::logic_error("uint n0 >= n1");
  }
#endif
  return randXoshiroFunc(n0, n1);
}

int32_t getRandInRange(const int32_t n0, const int32_t n1)
{
  if ((n0 < 0) && (n1 < 0))
  {
    return -static_cast<int32_t>(
        getRandInRange(static_cast<uint32_t>(-n1), static_cast<uint32_t>(-n0)));
  }
  if ((n0 >= 0) && (n1 >= 0))
  {
    return static_cast<int32_t>(
        getRandInRange(static_cast<uint32_t>(n0), static_cast<uint32_t>(n1)));
  }
  if (n0 >= n1)
  {
    throw std::logic_error("int n0 >= n1");
  }
  return n0 + static_cast<int32_t>(getNRand(static_cast<uint32_t>(n1)));
}

float getRandInRange(const float x0, const float x1)
{
#ifndef NDEBUG
  if (x0 >= x1)
  {
    throw std::logic_error("float x0 >= x1");
  }
#endif

  thread_local xoshiro256plus64 eng{std::random_device{}()};
  static const float eng_max = static_cast<float>(eng.max());
  const float t = static_cast<float>(eng()) / eng_max;
  return std::lerp(x0, x1, t);
  //  thread_local std::uniform_real_distribution<> dis(0, 1);
  //  return std::lerp(x0, x1, static_cast<float>(dis(eng)));
}

} // namespace goom::utils
