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

inline bool allowOverexposedEvent()
{
  return probabilityOfMInN(1, 100);
}

struct ConvolveData
{
  FXBuffSettings buffSettings = defaultFXBuffSettings;
  bool allowOverexposed = false;
  uint32_t countSinceOverexposed = 0;
  static constexpr uint32_t maxCountSinceOverexposed = 100;
  float screenBrightness = 100;
  float flashIntensity = 30;
  float factor = 0.5;

  template<class Archive>
  void serialize(Archive&);
};

template<class Archive>
void ConvolveData::serialize(Archive& ar)
{
  ar(buffSettings, allowOverexposed, countSinceOverexposed, screenBrightness, flashIntensity);
}

static void createOutputWithBrightness(const Pixel* src,
                                       Pixel* dest,
                                       const PluginInfo* goomInfo,
                                       const uint32_t iff,
                                       const bool allowOverexposed)
{
  int i = 0; // info->screen.height * info->screen.width - 1;
  for (uint32_t y = 0; y < goomInfo->screen.height; y++)
  {
    for (uint32_t x = 0; x < goomInfo->screen.width; x++)
    {
      dest[i] = getBrighterColorInt(iff, src[i], allowOverexposed);
      i++;
    }
  }
}

ConvolveFx::ConvolveFx(PluginInfo* info) : goomInfo{info}, fxData{new ConvolveData{}}
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

void ConvolveFx::log(const StatsLogValueFunc logVal) const
{
}

void ConvolveFx::apply(Pixel* prevBuff, Pixel* currentBuff)
{
  if (!enabled)
  {
    return;
  }

  const float ff = (fxData->factor * fxData->flashIntensity + fxData->screenBrightness) / 100.0f;
  const uint32_t iff = static_cast<uint32_t>(std::round(ff * 256 + 0.0001f));
  constexpr float increaseRate = 1.3;
  constexpr float decayRate = 0.955;
  if (goomInfo->sound->getTimeSinceLastGoom() == 0)
  {
    fxData->factor += goomInfo->sound->getGoomPower() * increaseRate;
  }
  fxData->factor *= decayRate;

  if (fxData->allowOverexposed)
  {
    fxData->countSinceOverexposed++;
    if (fxData->countSinceOverexposed > ConvolveData::maxCountSinceOverexposed)
    {
      fxData->allowOverexposed = false;
    }
  }
  else if (allowOverexposedEvent())
  {
    fxData->countSinceOverexposed = 0;
    fxData->allowOverexposed = true;
  }

  fxData->allowOverexposed = true;

  if (std::fabs(1.0 - ff) < 0.02)
  {
    memcpy(currentBuff, prevBuff, goomInfo->screen.size * sizeof(Pixel));
  }
  else
  {
    createOutputWithBrightness(prevBuff, currentBuff, goomInfo, iff, fxData->allowOverexposed);
  }
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

} // namespace goom
