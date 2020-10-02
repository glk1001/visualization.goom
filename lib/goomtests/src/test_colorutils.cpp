#include "catch2/catch.hpp"
#include "colorutils.h"
#include "goom_graphic.h"

#include <algorithm>

using namespace goom;

TEST_CASE("Color channels are added", "[color-channel-add]")
{
  REQUIRE(colorChannelAdd(100, 120) == 220);
  REQUIRE(colorChannelAdd(200, 120) == 255);
  REQUIRE(colorChannelAdd(0, 120) == 120);
  REQUIRE(colorChannelAdd(0, 0) == 0);
}

TEST_CASE("Colors are added", "[color-add]")
{
  const Pixel c1{.channels{.r = 100, .g = 50, .b = 20}};
  const Pixel c2{.channels{.r = 120, .g = 250, .b = 70}};
  const Pixel c3 = getColorAdd(c1, c2);

  REQUIRE(c3.channels.r == 220);
  REQUIRE(c3.channels.g == 255);
  REQUIRE(c3.channels.b == 90);
}

TEST_CASE("Color channels are brightened", "[color-channel-bright]")
{
  REQUIRE(getBrighterChannelColor(100, 2) == 100 * 2 / 255);
  REQUIRE(getBrighterChannelColor(11, 20) == 11 * 20 / 255);
  REQUIRE(getBrighterChannelColor(0, 20) == 0);
  REQUIRE(getBrighterChannelColor(100, 20) == std::clamp(0, 100 * 20 / 255, 255));
}

TEST_CASE("Colors are brightened", "[color-bright]")
{
  const Pixel c{.channels{.r = 100, .g = 50, .b = 20}};

  Pixel cb = getBrighterColor(1.0, c);
  REQUIRE(cb.channels.r == 100);
  REQUIRE(cb.channels.g == 50);
  REQUIRE(cb.channels.b == 20);

  cb = getBrighterColor(0.5, c);
  REQUIRE(cb.channels.r == 50);
  REQUIRE(cb.channels.g == 25);
  REQUIRE(cb.channels.b == 10);

  cb = getBrighterColor(0.01, c);
  REQUIRE(cb.channels.r == 1);
  REQUIRE(cb.channels.g == 0);
  REQUIRE(cb.channels.b == 0);
}

TEST_CASE("Half intensity color", "[color-half-intensity]")
{
  const Pixel c{.channels{.r = 100, .g = 50, .b = 20}};
  const Pixel ch = getHalfIntensityColor(c);

  REQUIRE(ch.channels.r == 50);
  REQUIRE(ch.channels.g == 25);
  REQUIRE(ch.channels.b == 10);
}

TEST_CASE("Lightened color", "[color-half-lightened]")
{
  const Pixel c{.channels{.r = 100, .g = 50, .b = 20}};
  Pixel cl;

  cl.val = getLightenedColor(c.val, 0.5);
  REQUIRE(cl.channels.r == 0);
  REQUIRE(cl.channels.g == 0);
  REQUIRE(cl.channels.b == 0);

  cl.val = getLightenedColor(c.val, 1.0);
  REQUIRE(cl.channels.r == 0);
  REQUIRE(cl.channels.g == 0);
  REQUIRE(cl.channels.b == 0);

  cl.val = getLightenedColor(c.val, 2.0);
  REQUIRE(cl.channels.r == 15);
  REQUIRE(cl.channels.g == 7);
  REQUIRE(cl.channels.b == 3);

  cl.val = getLightenedColor(c.val, 5.0);
  REQUIRE(cl.channels.r == 34);
  REQUIRE(cl.channels.g == 17);
  REQUIRE(cl.channels.b == 6);

  cl.val = getLightenedColor(c.val, 10.0);
  REQUIRE(cl.channels.r == 50);
  REQUIRE(cl.channels.g == 25);
  REQUIRE(cl.channels.b == 10);

  const Pixel c2{.channels{.r = 255, .g = 255, .b = 255}};
  cl.val = getLightenedColor(c2.val, 1.0);
  REQUIRE(cl.channels.r == 0);
  REQUIRE(cl.channels.g == 0);
  REQUIRE(cl.channels.b == 0);

  cl.val = getLightenedColor(c2.val, 2.0);
  REQUIRE(cl.channels.r == 38);
  REQUIRE(cl.channels.g == 38);
  REQUIRE(cl.channels.b == 38);

  cl.val = getLightenedColor(c2.val, 5.0);
  REQUIRE(cl.channels.r == 89);
  REQUIRE(cl.channels.g == 89);
  REQUIRE(cl.channels.b == 89);

  cl.val = getLightenedColor(c2.val, 10.0);
  REQUIRE(cl.channels.r == 127);
  REQUIRE(cl.channels.g == 127);
  REQUIRE(cl.channels.b == 127);
}

TEST_CASE("Evolved color", "[color-evolve]")
{
  const Pixel c{.channels{.r = 100, .g = 50, .b = 20}};
  Pixel cl;

  cl.val = getEvolvedColor(c.val);
  REQUIRE(cl.channels.r == 67);
  REQUIRE(cl.channels.g == 33);
  REQUIRE(cl.channels.b == 13);

  cl.val = getEvolvedColor(cl.val);
  REQUIRE(cl.channels.r == 44);
  REQUIRE(cl.channels.g == 22);
  REQUIRE(cl.channels.b == 8);

  cl.val = getEvolvedColor(cl.val);
  REQUIRE(cl.channels.r == 29);
  REQUIRE(cl.channels.g == 14);
  REQUIRE(cl.channels.b == 5);

  cl.val = getEvolvedColor(cl.val);
  REQUIRE(cl.channels.r == 19);
  REQUIRE(cl.channels.g == 9);
  REQUIRE(cl.channels.b == 3);
}
