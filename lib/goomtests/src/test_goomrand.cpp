#include "catch2/catch.hpp"
#include "goomrand.h"

#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <tuple>
#include <vector>

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

TEST_CASE("repeatable random sequence", "[repeatableRandomSequence]")
{
  const uint64_t seed = 1000;

  setRandSeed(seed);
  REQUIRE(seed == getRandSeed());
  std::vector<uint32_t> seq1(1000);
  std::vector<float> fseq1(1000);
  for (size_t i = 0; i < 1000; i++)
  {
    seq1[i] = getRand();
    fseq1[i] = getRandInRange(0.0F, 1.0F);
  }

  setRandSeed(seed);
  REQUIRE(seed == getRandSeed());
  std::vector<uint32_t> seq2(1000);
  std::vector<float> fseq2(1000);
  for (size_t i = 0; i < 1000; i++)
  {
    seq2[i] = getRand();
    fseq2[i] = getRandInRange(0.0F, 1.0F);
  }

  setRandSeed(seed + 1);
  REQUIRE(seed + 1 == getRandSeed());
  std::vector<uint32_t> seq3(1000);
  std::vector<float> fseq3(1000);
  for (size_t i = 0; i < 1000; i++)
  {
    seq3[i] = getRand();
    fseq3[i] = getRandInRange(0.0F, 1.0F);
  }

  REQUIRE(seq1 == seq2);
  REQUIRE(seq1 != seq3);

  REQUIRE(fseq1 == fseq2);
  REQUIRE(fseq1 != fseq3);
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
  // After a big enough loop, a good random distribution should have
  // covered the entire range: nMin <= n < nMax
  constexpr size_t numLoop = 100000;

  constexpr uint32_t nMin1 = 999;
  constexpr uint32_t nMax1 = 10001;
  const auto [min1, max1] = getMinMax(numLoop, nMin1, nMax1);
  REQUIRE(min1 == nMin1);
  REQUIRE(max1 == nMax1 - 1);

  constexpr uint32_t nMin2 = 0;
  constexpr uint32_t nMax2 = 120;
  const auto [min2, max2] = getMinMax(numLoop, nMin2, nMax2);
  REQUIRE(min2 == nMin2);
  REQUIRE(max2 == nMax2 - 1);

  REQUIRE_NOTHROW(getRandInRange(5U, 6U));
  REQUIRE_THROWS_WITH(getRandInRange(5U, 1U), "uint n0 >= n1");
}

TEST_CASE("int32_t min max get random", "[intMinMaxGetRandom]")
{
  // After a big enough loop, a good random distribution should have
  // covered the entire range: nMin <= n < nMax
  constexpr size_t numLoop = 100000;

  constexpr int32_t nMin1 = -999;
  constexpr int32_t nMax1 = 10001;
  const auto [min1, max1] = getMinMax(numLoop, nMin1, nMax1);
  REQUIRE(min1 == nMin1);
  REQUIRE(max1 == nMax1 - 1);

  constexpr int32_t nMin2 = -999;
  constexpr int32_t nMax2 = -50;
  const auto [min2, max2] = getMinMax(numLoop, nMin2, nMax2);
  REQUIRE(min2 == nMin2);
  REQUIRE(max2 == nMax2 - 1);

  constexpr int32_t nMin3 = 1;
  constexpr int32_t nMax3 = 999;
  const auto [min3, max3] = getMinMax(numLoop, nMin3, nMax3);
  REQUIRE(min3 == nMin3);
  REQUIRE(max3 == nMax3 - 1);

  constexpr int32_t nMin4 = 0;
  constexpr int32_t nMax4 = 635;
  const auto [min4, max4] = getMinMax(numLoop, nMin4, nMax4);
  REQUIRE(min4 == nMin4);
  REQUIRE(max4 == nMax4 - 1);

  REQUIRE_NOTHROW(getRandInRange(5, 6));
  REQUIRE_NOTHROW(getRandInRange(-6, -5));
  REQUIRE_NOTHROW(getRandInRange(-6, 10));
  REQUIRE_THROWS_WITH(getRandInRange(-5, -6), "int n0 >= n1");
  REQUIRE_THROWS_WITH(getRandInRange(5, 1), "int n0 >= n1");
  REQUIRE_THROWS_WITH(getRandInRange(5, -1), "int n0 >= n1");
}

TEST_CASE("float min max get random", "[fltMinMaxGetRandom]")
{
  // After a big enough loop, a good random distribution should have
  // covered the entire range: nMin <= n < nMax
  constexpr size_t numLoop = 1000000;

  constexpr float nMin1 = 0;
  constexpr float nMax1 = 1;
  const auto [min1, max1] = getMinMax(numLoop, nMin1, nMax1);
  REQUIRE(std::fabs(min1 - nMin1) < 0.0001);
  REQUIRE(std::fabs(max1 - nMax1) < 0.0001);

  constexpr float nMin2 = -1;
  constexpr float nMax2 = 0;
  const auto [min2, max2] = getMinMax(numLoop, nMin2, nMax2);
  REQUIRE(std::fabs(min2 - nMin2) < 0.0001);
  REQUIRE(std::fabs(max2 - nMax2) < 0.0001);

  constexpr float nMin3 = -10;
  constexpr float nMax3 = +10;
  const auto [min3, max3] = getMinMax(numLoop, nMin3, nMax3);
  REQUIRE(std::fabs(min3 - nMin3) < 0.0001);
  REQUIRE(std::fabs(max3 - nMax3) < 0.0001);

  REQUIRE_NOTHROW(getRandInRange(5.0F, 6.0F));
  REQUIRE_NOTHROW(getRandInRange(-6.0F, -5.0F));
  REQUIRE_NOTHROW(getRandInRange(-6.0F, 10.0F));
  REQUIRE_THROWS_WITH(getRandInRange(-5.0F, -6.0F), "float x0 >= x1");
  REQUIRE_THROWS_WITH(getRandInRange(5.0F, 1.0F), "float x0 >= x1");
  REQUIRE_THROWS_WITH(getRandInRange(5.0F, -1.0F), "float x0 >= x1");
}
