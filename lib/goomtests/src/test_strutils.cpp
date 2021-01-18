#include "catch2/catch.hpp"
#include "strutils.h"

#include <string>
#include <vector>

using namespace GOOM::UTILS;

TEST_CASE("splitString", "[splitString]")
{
  const std::string testString1 = "line1: word1, word2\nline2: word3, word4\n";

  const std::vector<std::string> test1 = splitString(testString1, ",");
  REQUIRE(test1[0] == "line1: word1");
  REQUIRE(test1[1] == " word2\nline2: word3");
  REQUIRE(test1[2] == " word4\n");

  const std::vector<std::string> test2 = splitString(testString1, "\n");
  REQUIRE(test2[0] == "line1: word1, word2");
  REQUIRE(test2[1] == "line2: word3, word4");

  const std::string testString2 = "word1; word2; word3; word4";
  const std::vector<std::string> test3 = splitString(testString2, "; ");
  REQUIRE(test3[0] == "word1");
  REQUIRE(test3[1] == "word2");
  REQUIRE(test3[2] == "word3");
  REQUIRE(test3[3] == "word4");

  const std::string testString3 = "word1 \nword2\nword3 \nword4 ";
  const std::vector<std::string> test4 = splitString(testString3, "\n");
  REQUIRE(test4[0] == "word1 ");
  REQUIRE(test4[1] == "word2");
  REQUIRE(test4[2] == "word3 ");
  REQUIRE(test4[3] == "word4 ");
}

TEST_CASE("enumToString", "[enumToString]")
{
  enum class EnumTester
  {
    _null = -1,
    test1,
    test2,
    test3,
    _size
  };

  EnumTester test = EnumTester::_null;
  REQUIRE(enumToString(test) == "_null");
  test = EnumTester::test1;
  REQUIRE(enumToString(test) == "test1");
  test = EnumTester::test2;
  REQUIRE(enumToString(test) == "test2");
  test = EnumTester::_size;
  REQUIRE(enumToString(test) == "_size");

  REQUIRE(enumToString(EnumTester::test3) == "test3");
}
