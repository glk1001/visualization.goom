#include "catch2/catch.hpp"
#include "colorutils.h"
#include "goom_graphic.h"

#include <algorithm>

using namespace GOOM;

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
  REQUIRE(colorChannelAdd(200, 120) == 320);
  REQUIRE(colorChannelAdd(0, 120) == 120);
  REQUIRE(colorChannelAdd(0, 0) == 0);
}

TEST_CASE("Colors are added", "[color-add]")
{
  const Pixel c1{{.r = 100, .g = 50, .b = 20}};
  const Pixel c2{{.r = 120, .g = 250, .b = 70}};
  const Pixel c3 = getColorAdd(c1, c2, true);

  REQUIRE(static_cast<uint32_t>(c3.R()) == 220);
  REQUIRE(static_cast<uint32_t>(c3.G()) == 255);
  REQUIRE(static_cast<uint32_t>(c3.B()) == 90);

  const Pixel c4 = getColorAdd(c1, c2, false);
  REQUIRE(static_cast<uint32_t>(c4.R()) == 220 * 255 / 300);
  REQUIRE(static_cast<uint32_t>(c4.G()) == 255);
  REQUIRE(static_cast<uint32_t>(c4.B()) == 90 * 255 / 300);
}

TEST_CASE("Color channels are brightened", "[color-channel-bright]")
{
  REQUIRE(getBrighterChannelColor(100, 2) == 100 * 2 / 255);
  REQUIRE(getBrighterChannelColor(11, 20) == 11 * 20 / 255);
  REQUIRE(getBrighterChannelColor(0, 20) == 0);
  REQUIRE(getBrighterChannelColor(100, 20) == std::clamp(0U, 100U * 20U / 255U, 255U));
}

TEST_CASE("Colors are brightened", "[color-bright]")
{
  const Pixel c{{.r = 100, .g = 50, .b = 20}};

  Pixel cb = getBrighterColor(1.0f, c, false);
  REQUIRE(cb.R() == 100);
  REQUIRE(cb.G() == 50);
  REQUIRE(cb.B() == 20);

  cb = getBrighterColor(0.5f, c, false);
  REQUIRE(cb.R() == 50);
  REQUIRE(cb.G() == 25);
  REQUIRE(cb.B() == 10);

  cb = getBrighterColor(0.01f, c, false);
  REQUIRE(cb.R() == 1);
  REQUIRE(cb.G() == 0);
  REQUIRE(cb.B() == 0);
}

TEST_CASE("Half intensity color", "[color-half-intensity]")
{
  const Pixel c{{.r = 100, .g = 50, .b = 20}};
  const Pixel ch = getHalfIntensityColor(c);

  REQUIRE(ch.R() == 50);
  REQUIRE(ch.G() == 25);
  REQUIRE(ch.B() == 10);
}

TEST_CASE("Lighten", "[color-lighten]")
{
  const Pixel c{{.r = 100, .g = 0, .b = 0}};

  const Pixel cl = getLightenedColor(c, 10.0);
  REQUIRE(static_cast<uint32_t>(cl.R()) == 50);
  REQUIRE(static_cast<uint32_t>(cl.G()) == 0);
  REQUIRE(static_cast<uint32_t>(cl.B()) == 0);
}

TEST_CASE("Lightened color", "[color-half-lightened]")
{
  const Pixel c{{.r = 100, .g = 50, .b = 20}};

  Pixel cl = getLightenedColor(c, 0.5);
  REQUIRE(cl.R() == 0);
  REQUIRE(cl.G() == 0);
  REQUIRE(cl.B() == 0);

  cl = getLightenedColor(c, 1.0);
  REQUIRE(cl.R() == 0);
  REQUIRE(cl.G() == 0);
  REQUIRE(cl.B() == 0);

  cl = getLightenedColor(c, 2.0);
  REQUIRE(cl.R() == 15);
  REQUIRE(cl.G() == 7);
  REQUIRE(cl.B() == 3);

  cl = getLightenedColor(c, 5.0);
  REQUIRE(cl.R() == 34);
  REQUIRE(cl.G() == 17);
  REQUIRE(cl.B() == 6);

  cl = getLightenedColor(c, 10.0);
  REQUIRE(cl.R() == 50);
  REQUIRE(cl.G() == 25);
  REQUIRE(cl.B() == 10);

  const Pixel c2{{.r = 255, .g = 255, .b = 255}};
  cl = getLightenedColor(c2, 1.0);
  REQUIRE(cl.R() == 0);
  REQUIRE(cl.G() == 0);
  REQUIRE(cl.B() == 0);

  cl = getLightenedColor(c2, 2.0);
  REQUIRE(cl.R() == 38);
  REQUIRE(cl.G() == 38);
  REQUIRE(cl.B() == 38);

  cl = getLightenedColor(c2, 5.0);
  REQUIRE(cl.R() == 89);
  REQUIRE(cl.G() == 89);
  REQUIRE(cl.B() == 89);

  cl = getLightenedColor(c2, 10.0);
  REQUIRE(cl.R() == 127);
  REQUIRE(cl.G() == 127);
  REQUIRE(cl.B() == 127);
}

TEST_CASE("Evolved color", "[color-evolve]")
{
  const Pixel c{{.r = 100, .g = 50, .b = 20}};
  Pixel cl;

  cl = getEvolvedColor(c);
  REQUIRE(cl.R() == 67);
  REQUIRE(cl.G() == 33);
  REQUIRE(cl.B() == 13);

  cl = getEvolvedColor(cl);
  REQUIRE(cl.R() == 44);
  REQUIRE(cl.G() == 22);
  REQUIRE(cl.B() == 8);

  cl = getEvolvedColor(cl);
  REQUIRE(cl.R() == 29);
  REQUIRE(cl.G() == 14);
  REQUIRE(cl.B() == 5);

  cl = getEvolvedColor(cl);
  REQUIRE(cl.R() == 19);
  REQUIRE(cl.G() == 9);
  REQUIRE(cl.B() == 3);
}
