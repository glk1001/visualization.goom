#include "convolve_fx.h"

#include "colorutils.h"
#include "goom_config.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/goomrand.h"
#include "goomutils/mathutils.h"

#include <cereal/archives/json.hpp>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <istream>
#include <ostream>
#include <string>

namespace goom
{

using namespace goom::utils;

struct ConvolveData
{
  ConvolveData(const uint32_t screenWidth, const uint32_t screenHeight);
  const uint32_t screenWidth;
  const uint32_t screenHeight;
  float screenBrightness = 100;
  float flashIntensity = 30;
  float factor = 0.5;
  FXBuffSettings buffSettings{};

  void createOutputWithBrightness(const Pixel* src, uint32_t* dest, const uint32_t flashInt);

  template<class Archive>
  void serialize(Archive&);
};

ConvolveData::ConvolveData(const uint32_t w, const uint32_t h) : screenWidth{w}, screenHeight{h}
{
}

template<class Archive>
void ConvolveData::serialize(Archive& ar)
{
  ar(buffSettings, screenBrightness, flashIntensity, factor);
}

ConvolveFx::ConvolveFx(const PluginInfo* info)
  : goomInfo{info},
    fxData{new ConvolveData{goomInfo->getScreenInfo().width, goomInfo->getScreenInfo().height}}
{
}

ConvolveFx::~ConvolveFx() noexcept
{
}

void ConvolveFx::setBuffSettings(const FXBuffSettings& settings)
{
  fxData->buffSettings = settings;
}

void ConvolveFx::start()
{
}

void ConvolveFx::finish()
{
  std::ofstream f("/tmp/convolve.json");
  saveState(f);
  f << std::endl;
  f.close();
}

void ConvolveFx::log(const StatsLogValueFunc&) const
{
}

std::string ConvolveFx::getFxName() const
{
  return "Convolve FX";
}

void ConvolveFx::saveState(std::ostream& f)
{
  cereal::JSONOutputArchive archiveOut(f);
  archiveOut(*fxData);
}

void ConvolveFx::loadState(std::istream& f)
{
  cereal::JSONInputArchive archive_in(f);
  archive_in(*fxData);
  }

  void ConvolveFx::convolve(const Pixel* currentBuff, uint32_t* outputBuff)
  {
    if (!enabled)
    {
      return;
    }

    const float flash =
        (fxData->factor * fxData->flashIntensity + fxData->screenBrightness) / 100.0F;
    const uint32_t flashInt = static_cast<uint32_t>(std::round(flash * 256 + 0.0001F));
    constexpr float increaseRate = 1.3;
    constexpr float decayRate = 0.955;
    if (goomInfo->getSoundInfo().getTimeSinceLastGoom() == 0)
    {
      fxData->factor += goomInfo->getSoundInfo().getGoomPower() * increaseRate;
    }
    fxData->factor *= decayRate;

    if (std::fabs(1.0 - flash) > 0.02)
    {
      fxData->createOutputWithBrightness(currentBuff, outputBuff, flashInt);
    }
    else
    {
      static_assert(sizeof(Pixel) == sizeof(uint32_t));
      memcpy(outputBuff, currentBuff, goomInfo->getScreenInfo().size * sizeof(Pixel));
    }
  }

  void ConvolveData::createOutputWithBrightness(const Pixel* src,
                                                uint32_t* dest,
                                                const uint32_t flashInt)
  {
    size_t i = 0; // < screenWidth * screenHeight
    for (uint32_t y = 0; y < screenHeight; y++)
    {
      for (uint32_t x = 0; x < screenWidth; x++)
      {
        dest[i] = getBrighterColorInt(flashInt, src[i], buffSettings.allowOverexposed).rgba();
        i++;
      }
    }
  }

} // namespace goom
