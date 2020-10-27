#include "catch2/catch.hpp"
#include "colorutils.h"
#include "goom_graphic.h"

#include <algorithm>

using namespace goom;

TEST_CASE("Test max channels", "[channels-max]")
{
  REQUIRE(channel_limits<uint8_t>::min() == 0);
  REQUIRE(channel_limits<uint8_t>::max() == 255);
  REQUIRE(channel_limits<uint32_t>::min() == 0);
  REQUIRE(channel_limits<uint32_t>::max() == 255);
  REQUIRE(channel_limits<int>::min() == 0);
  REQUIRE(channel_limits<int>::max() == 255);
  REQUIRE(channel_limits<float>::min() == 0);
  REQUIRE(channel_limits<float>::max() == 255.0F);
}

TEST_CASE("Color channels are added", "[color-channel-add]")
{
  REQUIRE(colorChannelAdd(100, 120) == 220);
  REQUIRE(colorChannelAdd(200, 120) == 255);
  REQUIRE(colorChannelAdd(0, 120) == 120);
  REQUIRE(colorChannelAdd(0, 0) == 0);
}

TEST_CASE("Colors are added", "[color-add]")
{
  const Pixel c1{{.r = 100, .g = 50, .b = 20}};
  const Pixel c2{{.r = 120, .g = 250, .b = 70}};
  const Pixel c3 = getColorAdd(c1, c2, false);

  REQUIRE(static_cast<uint32_t>(c3.r()) == 220);
  REQUIRE(static_cast<uint32_t>(c3.g()) == 255);
  REQUIRE(static_cast<uint32_t>(c3.b()) == 90);
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
  const Pixel c{{.r = 100, .g = 50, .b = 20}};

  Pixel cb = getBrighterColor(1.0f, c, false);
  REQUIRE(cb.r() == 100);
  REQUIRE(cb.g() == 50);
  REQUIRE(cb.b() == 20);

  cb = getBrighterColor(0.5f, c, false);
  REQUIRE(cb.r() == 50);
  REQUIRE(cb.g() == 25);
  REQUIRE(cb.b() == 10);

  cb = getBrighterColor(0.01f, c, false);
  REQUIRE(cb.r() == 1);
  REQUIRE(cb.g() == 0);
  REQUIRE(cb.b() == 0);
}

TEST_CASE("Half intensity color", "[color-half-intensity]")
{
  const Pixel c{{.r = 100, .g = 50, .b = 20}};
  const Pixel ch = getHalfIntensityColor(c);

  REQUIRE(ch.r() == 50);
  REQUIRE(ch.g() == 25);
  REQUIRE(ch.b() == 10);
}

TEST_CASE("Lighten", "[color-lighten]")
{
  const Pixel c{{.r = 100, .g = 0, .b = 0}};

  const Pixel cl = getLightenedColor(c, 10.0);
  REQUIRE(static_cast<uint32_t>(cl.r()) == 50);
  REQUIRE(static_cast<uint32_t>(cl.g()) == 0);
  REQUIRE(static_cast<uint32_t>(cl.b()) == 0);
}

TEST_CASE("Lightened color", "[color-half-lightened]")
{
  const Pixel c{{.r = 100, .g = 50, .b = 20}};

  Pixel cl = getLightenedColor(c, 0.5);
  REQUIRE(cl.r() == 0);
  REQUIRE(cl.g() == 0);
  REQUIRE(cl.b() == 0);

  cl = getLightenedColor(c, 1.0);
  REQUIRE(cl.r() == 0);
  REQUIRE(cl.g() == 0);
  REQUIRE(cl.b() == 0);

  cl = getLightenedColor(c, 2.0);
  REQUIRE(cl.r() == 15);
  REQUIRE(cl.g() == 7);
  REQUIRE(cl.b() == 3);

  cl = getLightenedColor(c, 5.0);
  REQUIRE(cl.r() == 34);
  REQUIRE(cl.g() == 17);
  REQUIRE(cl.b() == 6);

  cl = getLightenedColor(c, 10.0);
  REQUIRE(cl.r() == 50);
  REQUIRE(cl.g() == 25);
  REQUIRE(cl.b() == 10);

  const Pixel c2{{.r = 255, .g = 255, .b = 255}};
  cl = getLightenedColor(c2, 1.0);
  REQUIRE(cl.r() == 0);
  REQUIRE(cl.g() == 0);
  REQUIRE(cl.b() == 0);

  cl = getLightenedColor(c2, 2.0);
  REQUIRE(cl.r() == 38);
  REQUIRE(cl.g() == 38);
  REQUIRE(cl.b() == 38);

  cl = getLightenedColor(c2, 5.0);
  REQUIRE(cl.r() == 89);
  REQUIRE(cl.g() == 89);
  REQUIRE(cl.b() == 89);

  cl = getLightenedColor(c2, 10.0);
  REQUIRE(cl.r() == 127);
  REQUIRE(cl.g() == 127);
  REQUIRE(cl.b() == 127);
}

TEST_CASE("Evolved color", "[color-evolve]")
{
  const Pixel c{{.r = 100, .g = 50, .b = 20}};
  Pixel cl;

  cl = getEvolvedColor(c);
  REQUIRE(cl.r() == 67);
  REQUIRE(cl.g() == 33);
  REQUIRE(cl.b() == 13);

  cl = getEvolvedColor(cl);
  REQUIRE(cl.r() == 44);
  REQUIRE(cl.g() == 22);
  REQUIRE(cl.b() == 8);

  cl = getEvolvedColor(cl);
  REQUIRE(cl.r() == 29);
  REQUIRE(cl.g() == 14);
  REQUIRE(cl.b() == 5);

  cl = getEvolvedColor(cl);
  REQUIRE(cl.r() == 19);
  REQUIRE(cl.g() == 9);
  REQUIRE(cl.b() == 3);
}
