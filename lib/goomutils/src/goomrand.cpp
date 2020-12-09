#include "goomutils/goomrand.h"

#include "goomutils/logging_control.h"
//#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/splitmix.hpp"
#include "goomutils/xoshiro.hpp"

#undef NDEBUG

#include <cmath>
#include <cstdint>
#include <limits>
#include <ostream>
#include <random>
#include <stdexcept>

namespace goom::utils
{

const uint32_t randMax = (xoshiro256plus64::max() > std::numeric_limits<uint32_t>::max())
                             ? std::numeric_limits<uint32_t>::max()
                             : xoshiro256plus64::max();

// NOTE: C++ std::uniform_int_distribution is too expensive (about double) so we use Xoshiro
//   and multiplication/shift technique. For timings, see tests/test_goomrand.cpp.
// thread_local xoshiro256starstar64 eng { getRandSeed() };

thread_local uint64_t randSeed = std::random_device{}();
thread_local xoshiro256plus64 xoshiroEng{getRandSeed()};

uint64_t getRandSeed()
{
  return randSeed;
}

void setRandSeed(const uint64_t seed)
{
  randSeed = seed;
  xoshiroEng = getRandSeed();
}

inline uint32_t randXoshiroFunc(const uint32_t n0, const uint32_t n1)
{
  const uint32_t x = xoshiroEng();
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

void saveRandState(std::ostream& f)
{
  f << randSeed << std::endl;
  f << xoshiroEng << std::endl;
}

void restoreRandState(std::istream& f)
{
  f >> randSeed;
  f >> xoshiroEng;
}

uint32_t getRandInRange(const uint32_t n0, const uint32_t n1)
{
#ifndef NDEBUG
  if (n0 >= n1)
  {
    throw std::logic_error("uint n0 >= n1");
  }
#endif
#ifndef NO_LOGGING
  const uint32_t randVal = randXoshiroFunc(n0, n1);
  logDebug("randVal = {}", randVal);
  return randVal;
#endif

  return randXoshiroFunc(n0, n1);
}

int32_t getRandInRange(const int32_t n0, const int32_t n1)
{
  if (n0 >= n1)
  {
    throw std::logic_error("int n0 >= n1");
  }
  if ((n0 >= 0) && (n1 >= 0))
  {
    return static_cast<int32_t>(
        getRandInRange(static_cast<uint32_t>(n0), static_cast<uint32_t>(n1)));
  }
  return n0 + static_cast<int32_t>(getNRand(static_cast<uint32_t>(-n0 + n1)));
}

float getRandInRange(const float x0, const float x1)
{
#ifndef NDEBUG
  if (x0 >= x1)
  {
    throw std::logic_error("float x0 >= x1");
  }
#endif

  static const auto eng_max = static_cast<float>(randMax);
  const float t = static_cast<float>(randXoshiroFunc(0, randMax)) / eng_max;
  return std::lerp(x0, x1, t);
  //  thread_local std::uniform_real_distribution<> dis(0, 1);
  //  return std::lerp(x0, x1, static_cast<float>(dis(eng)));
}

} // namespace goom::utils
