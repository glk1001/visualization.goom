#include "catch2/catch.hpp"
#include "goomrand.h"

#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <tuple>

using namespace goom::utils;

TEST_CASE("save/restore random state", "[saveRestoreRandState]")
{
  const uint64_t seed = 1000;

  setRandSeed(seed);
  REQUIRE(seed == getRandSeed());

  const uint32_t r1 = getRand();
  const uint32_t r2 = getRand();
  REQUIRE(r1 != r2);

  const std::string saveFile = "/tmp/rand.txt";
  std::ofstream fout{saveFile, std::ofstream::out};
  saveRandState(fout);
  fout.close();
  const uint32_t r_justAfterSave = getRand();

  // Scramble things a bit
  setRandSeed(seed + 10);
  uint32_t r = 0;
  for (size_t i = 0; i < 1000; i++)
  {
    r = getRand();
  }
  REQUIRE(seed != getRandSeed());
  REQUIRE(r != r_justAfterSave);

  std::ifstream fin{saveFile, std::ifstream::in};
  restoreRandState(fin);
  r = getRand();
  REQUIRE(seed == getRandSeed());
  REQUIRE(r == r_justAfterSave);
}

template<typename valtype>
std::tuple<valtype, valtype> getMinMax(const size_t numLoop,
                                       const valtype& nMin,
                                       const valtype& nMax)
{
  valtype min = std::numeric_limits<valtype>::max();
  valtype max = std::numeric_limits<valtype>::min();
  for (size_t i = 0; i < numLoop; i++)
  {
    valtype r = getRandInRange(nMin, nMax);
    if (r < min)
    {
      min = r;
    }
    if (r > max)
    {
      max = r;
    }
  }

  return std::make_tuple(min, max);
}

TEST_CASE("uint32_t min max get random", "[uintMinMaxGetRandom]")
{
  constexpr uint64_t seed = 1000;

  // After a bigger enough loop, a good random distribution should have
  // covered the entire range: nMin <= n < nMax
  constexpr size_t numLoop = 1000000;

  constexpr uint32_t nMin = 999;
  constexpr uint32_t nMax = 100001;
  const auto [min, max] = getMinMax(numLoop, nMin, nMax);
  REQUIRE(min == nMin);
  REQUIRE(max == nMax - 1);
}

TEST_CASE("int32_t min max get random", "[intMinMaxGetRandom]")
{
  constexpr uint64_t seed = 1000;

  // After a bigger enough loop, a good random distribution should have
  // covered the entire range: nMin <= n < nMax
  constexpr size_t numLoop = 1000000;

  constexpr int32_t nMin1 = -999;
  constexpr int32_t nMax1 = 100001;
  const auto [min1, max1] = getMinMax(numLoop, nMin1, nMax1);
  REQUIRE(min1 == nMin1);
  REQUIRE(max1 == nMax1 - 1);

  constexpr int32_t nMin2 = -999;
  constexpr int32_t nMax2 = -50;
  const auto [min2, max2] = getMinMax(numLoop, nMin2, nMax2);
  REQUIRE(min2 == nMin2);
  REQUIRE(max2 == nMax2 + 1);
}
