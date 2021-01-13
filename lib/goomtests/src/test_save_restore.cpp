#include "catch2/catch.hpp"
#include "goom/convolve_fx.h"
#include "goom/filters.h"
#include "goom/flying_stars_fx.h"
#include "goom/goom_control.h"
#include "goom/goom_dots_fx.h"
#include "goom/goom_draw.h"
#include "goom/goom_graphic.h"
#include "goom/goom_plugin_info.h"
#include "goom/ifs_dancers_fx.h"
#include "goom/lines_fx.h"
#include "goom/sound_info.h"
#include "goom/tentacles_fx.h"
#include "goomutils/parallel_utils.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

using namespace goom;

constexpr uint32_t screenWidth = 100;
constexpr uint32_t screenHeight = 100;
static const std::shared_ptr<PluginInfo> goomInfo{new PluginInfo{screenWidth, screenHeight}};
inline std::shared_ptr<const PluginInfo> getGoomInfo()
{
  return std::const_pointer_cast<const PluginInfo>(goomInfo);
}

inline PixelBuffer* getNewBuffer()
{
  return new PixelBuffer{screenWidth + 1, screenHeight + 1};
}

TEST_CASE("save/restore plugin info", "[saveRestorePluginInfo]")
{
  PluginInfo testGoominfo{*goomInfo};

  std::stringstream ss;
  {
    cereal::JSONOutputArchive archive(ss);
    archive(testGoominfo);
  }
  //  std::cout << ss.str() << std::endl;

  PluginInfo testGoominfoRestored{screenWidth + 1, screenHeight + 1};
  REQUIRE(testGoominfo != testGoominfoRestored);
  {
    cereal::JSONInputArchive archive(ss);
    archive(testGoominfoRestored);
  }

  REQUIRE(testGoominfo == testGoominfoRestored);
}

TEST_CASE("save/restore goom draw", "[saveRestoreGoomDraw]")
{
  GoomDraw draw{screenWidth, screenHeight};
  draw.SetAllowOverexposed(not draw.GetAllowOverexposed());
  draw.SetBuffIntensity(0.5 * draw.GetBuffIntensity());
  std::stringstream ss;
  {
    cereal::JSONOutputArchive archive(ss);
    archive(draw);
  }
  //  std::cout << ss.str() << std::endl;

  GoomDraw drawRestored{1280, 720};
  REQUIRE(draw != drawRestored);
  {
    cereal::JSONInputArchive archive(ss);
    archive(drawRestored);
  }

  REQUIRE(draw == drawRestored);
}

TEST_CASE("save/restore convolve object", "[saveRestoreConvolve]")
{
  std::unique_ptr<PixelBuffer> prevBuff{getNewBuffer()};
  std::unique_ptr<PixelBuffer> currentBuff{getNewBuffer()};
  utils::Parallel parallel{};

  ConvolveFx convolveFx{parallel, getGoomInfo()};
  for (size_t i = 0; i < 100; i++)
  {
    convolveFx.Convolve(*prevBuff, *currentBuff);
  }
  std::stringstream ss;
  {
    cereal::JSONOutputArchive archive(ss);
    archive(convolveFx);
  }
  //  std::cout << ss.str() << std::endl;

  ConvolveFx convolveFxRestored{};
  REQUIRE(convolveFx != convolveFxRestored);
  {
    cereal::JSONInputArchive archive(ss);
    archive(convolveFxRestored);
  }

  REQUIRE(convolveFx == convolveFxRestored);
}

TEST_CASE("save/restore filter", "[saveRestoreFilter]")
{
  std::unique_ptr<PixelBuffer> prevBuff{getNewBuffer()};
  std::unique_ptr<PixelBuffer> currentBuff{getNewBuffer()};
  utils::Parallel parallel{};

  ZoomFilterFx filterFx{parallel, getGoomInfo()};
  const ZoomFilterData* zf = nullptr;
  const int switchIncr = 1;
  const float switchMult = 1;
  for (size_t i = 0; i < 100; i++)
  {
    filterFx.ZoomFilterFastRgb(*prevBuff, *currentBuff, zf, switchIncr, switchMult);
  }
  std::stringstream ss;
  {
    cereal::JSONOutputArchive archive(ss);
    archive(filterFx);
  }
  //  std::cout << ss.str() << std::endl;

  ZoomFilterFx filterFxRestored{};
  std::stringstream ssCheck;
  {
    cereal::JSONOutputArchive archive(ssCheck);
    archive(filterFxRestored);
  }
  //  std::cout << "Restored filter\n";
  //  std::cout << ssCheck.str() << std::endl;
  REQUIRE(filterFx != filterFxRestored);
  {
    cereal::JSONInputArchive archive(ss);
    archive(filterFxRestored);
  }

  REQUIRE(filterFx == filterFxRestored);
}

TEST_CASE("save/restore flying stars", "[saveRestoreFlyingStars]")
{
  std::unique_ptr<PixelBuffer> prevBuff{getNewBuffer()};
  std::unique_ptr<PixelBuffer> currentBuff{getNewBuffer()};

  FlyingStarsFx flyingStarsFx{getGoomInfo()};
  for (size_t i = 0; i < 100; i++)
  {
    flyingStarsFx.apply(*prevBuff, *currentBuff);
  }
  std::stringstream ss;
  {
    cereal::JSONOutputArchive archive(ss);
    archive(flyingStarsFx);
  }
  //  std::cout << ss.str() << std::endl;

  FlyingStarsFx flyingStarsFxRestored{};
  REQUIRE(flyingStarsFx != flyingStarsFxRestored);
  {
    cereal::JSONInputArchive archive(ss);
    archive(flyingStarsFxRestored);
  }
  std::stringstream ssCheck;
  {
    cereal::JSONOutputArchive archive(ssCheck);
    archive(flyingStarsFxRestored);
  }
  //  std::cout << "Restored ifs\n";
  //  std::cout << ssCheck.str() << std::endl;

  REQUIRE(flyingStarsFx == flyingStarsFxRestored);
}

TEST_CASE("save/restore goom dots", "[saveRestoreGoomDots]")
{
  std::unique_ptr<PixelBuffer> prevBuff{getNewBuffer()};
  std::unique_ptr<PixelBuffer> currentBuff{getNewBuffer()};

  GoomDotsFx dotsFx{getGoomInfo()};
  for (size_t i = 0; i < 100; i++)
  {
    dotsFx.apply(*prevBuff, *currentBuff);
  }
  std::stringstream ss;
  {
    cereal::JSONOutputArchive archive(ss);
    archive(dotsFx);
  }
  //  std::cout << ss.str() << std::endl;

  GoomDotsFx dotsFxRestored{};
  REQUIRE(dotsFx != dotsFxRestored);
  {
    cereal::JSONInputArchive archive(ss);
    archive(dotsFxRestored);
  }
  std::stringstream ssCheck;
  {
    cereal::JSONOutputArchive archive(ssCheck);
    archive(dotsFxRestored);
  }
  //  std::cout << "Restored ifs\n";
  //  std::cout << ssCheck.str() << std::endl;

  REQUIRE(dotsFx == dotsFxRestored);
}

TEST_CASE("save/restore ifs", "[saveRestoreIfs]")
{
  std::unique_ptr<PixelBuffer> prevBuff{getNewBuffer()};
  std::unique_ptr<PixelBuffer> currentBuff{getNewBuffer()};

  IfsDancersFx ifsFx{getGoomInfo()};
  for (size_t i = 0; i < 100; i++)
  {
    ifsFx.apply(*prevBuff, *currentBuff);
  }
  std::stringstream ss;
  {
    cereal::JSONOutputArchive archive(ss);
    archive(ifsFx);
  }
  //  std::cout << ss.str() << std::endl;

  IfsDancersFx ifsFxRestored{};
  REQUIRE(ifsFx != ifsFxRestored);
  {
    cereal::JSONInputArchive archive(ss);
    archive(ifsFxRestored);
  }
  std::stringstream ssCheck;
  {
    cereal::JSONOutputArchive archive(ssCheck);
    archive(ifsFxRestored);
  }
  //  std::cout << "Restored ifs\n";
  //  std::cout << ssCheck.str() << std::endl;

  REQUIRE(ifsFx == ifsFxRestored);
}

TEST_CASE("save/restore lines", "[saveRestoreLines]")
{
  std::unique_ptr<PixelBuffer> prevBuff{getNewBuffer()};
  std::unique_ptr<PixelBuffer> currentBuff{getNewBuffer()};

  static const Pixel lGreen = getGreenLineColor();
  static const Pixel lRed = getRedLineColor();
  static const Pixel lBlack = getBlackLineColor();
  LinesFx linesFx{getGoomInfo(), LinesFx::LineType::hline,  100,
                  lBlack,        LinesFx::LineType::circle, 0.4f * static_cast<float>(100),
                  lGreen};
  linesFx.switchLines(LinesFx::LineType::circle, 1, 2, lRed);
  std::stringstream ss;
  {
    cereal::JSONOutputArchive archive(ss);
    archive(linesFx);
  }
  //  std::cout << ss.str() << std::endl;

  LinesFx linesFxRestored{};
  REQUIRE(linesFx != linesFxRestored);
  {
    cereal::JSONInputArchive archive(ss);
    archive(linesFxRestored);
  }
  std::stringstream ssCheck;
  {
    cereal::JSONOutputArchive archive(ssCheck);
    archive(linesFxRestored);
  }
  //  std::cout << "Restored ifs\n";
  //  std::cout << ssCheck.str() << std::endl;

  REQUIRE(linesFx == linesFxRestored);
}

TEST_CASE("save/restore tentacles", "[saveRestoreTentacles]")
{
  std::unique_ptr<PixelBuffer> prevBuff{getNewBuffer()};
  std::unique_ptr<PixelBuffer> currentBuff{getNewBuffer()};

  TentaclesFx tentaclesFx{getGoomInfo()};
  for (size_t i = 0; i < 100; i++)
  {
    tentaclesFx.apply(*prevBuff, *currentBuff);
  }
  std::stringstream ss;
  {
    cereal::JSONOutputArchive archive(ss);
    archive(tentaclesFx);
  }
  //  std::cout << ss.str() << std::endl;

  TentaclesFx tentaclesFxRestored{};
  REQUIRE(tentaclesFx != tentaclesFxRestored);
  {
    cereal::JSONInputArchive archive(ss);
    archive(tentaclesFxRestored);
  }
  std::stringstream ssCheck;
  {
    cereal::JSONOutputArchive archive(ssCheck);
    archive(tentaclesFxRestored);
  }
  //  std::cout << "Restored ifs\n";
  //  std::cout << ssCheck.str() << std::endl;

  REQUIRE(tentaclesFx == tentaclesFxRestored);
}

TEST_CASE("save/restore goom control", "[saveRestoreGoomControl]")
{
  std::unique_ptr<PixelBuffer> outputBuff{getNewBuffer()};

  constexpr uint64_t seed = 10;
  GoomControl::setRandSeed(seed);
  GoomControl goomControl{screenWidth, screenHeight};
  goomControl.setScreenBuffer(*outputBuff);
  std::vector<float> soundData(NUM_AUDIO_SAMPLES * AUDIO_SAMPLE_LEN);
  std::fill(soundData.begin(), soundData.end(), 0.5);
  const AudioSamples audioSamples{2, soundData.data()};
  for (size_t i = 0; i < 100; i++)
  {
    goomControl.update(audioSamples, 0, 0.0F, "Hello Test", "");
  }
  std::stringstream ss;
  {
    cereal::JSONOutputArchive archive(ss);
    archive(goomControl);
  }
  //  std::cout << ss.str() << std::endl;

  GoomControl goomControlRestored{};
  REQUIRE(goomControl != goomControlRestored);
  {
    cereal::JSONInputArchive archive(ss);
    archive(goomControlRestored);
    goomControlRestored.setScreenBuffer(*outputBuff);
  }
  std::stringstream ssCheck;
  {
    cereal::JSONOutputArchive archive(ssCheck);
    archive(goomControlRestored);
  }
  //  std::cout << "Restored ifs\n";
  //  std::cout << ssCheck.str() << std::endl;

  REQUIRE(goomControl == goomControlRestored);
}
