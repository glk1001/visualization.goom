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

class ConvolveFx::ConvolveImpl
{
public:
  ConvolveImpl(const PluginInfo*);

  void setBuffSettings(const FXBuffSettings&);

  void convolve(const Pixel* currentBuff, uint32_t* outputBuff);

  template<class Archive>
  void serialize(Archive&);

private:
  const PluginInfo* const goomInfo;
  float screenBrightness = 100;
  float flashIntensity = 30;
  float factor = 0.5;
  FXBuffSettings buffSettings{};

  void createOutputWithBrightness(const Pixel* src, uint32_t* dest, const uint32_t flashInt);
};

ConvolveFx::ConvolveFx(const PluginInfo* info) : fxImpl{new ConvolveImpl{info}}
{
}

ConvolveFx::~ConvolveFx() noexcept
{
}

void ConvolveFx::setBuffSettings(const FXBuffSettings& settings)
{
  fxImpl->setBuffSettings(settings);
}

void ConvolveFx::start()
{
}

void ConvolveFx::finish()
{
}

void ConvolveFx::log(const StatsLogValueFunc&) const
{
}

std::string ConvolveFx::getFxName() const
{
  return "Convolve FX";
}

void ConvolveFx::saveState(std::ostream& f) const
{
  cereal::JSONOutputArchive archiveOut(f);
  archiveOut(*fxImpl);
}

void ConvolveFx::loadState(std::istream& f)
{
  cereal::JSONInputArchive archive_in(f);
  archive_in(*fxImpl);
}

void ConvolveFx::convolve(const Pixel* currentBuff, uint32_t* outputBuff)
{
  if (!enabled)
  {
    return;
  }

  fxImpl->convolve(currentBuff, outputBuff);
}

ConvolveFx::ConvolveImpl::ConvolveImpl(const PluginInfo* info) : goomInfo{info}
{
}

template<class Archive>
void ConvolveFx::ConvolveImpl::serialize(Archive& ar)
{
  ar(buffSettings, screenBrightness, flashIntensity, factor);
}

inline void ConvolveFx::ConvolveImpl::setBuffSettings(const FXBuffSettings& settings)
{
  buffSettings = settings;
}

void ConvolveFx::ConvolveImpl::convolve(const Pixel* currentBuff, uint32_t* outputBuff)
{
  const float flash = (factor * flashIntensity + screenBrightness) / 100.0F;
  const uint32_t flashInt = static_cast<uint32_t>(std::round(flash * 256 + 0.0001F));
  constexpr float increaseRate = 1.3;
  constexpr float decayRate = 0.955;
  if (goomInfo->getSoundInfo().getTimeSinceLastGoom() == 0)
  {
    factor += goomInfo->getSoundInfo().getGoomPower() * increaseRate;
  }
  factor *= decayRate;

  if (std::fabs(1.0 - flash) > 0.02)
  {
    createOutputWithBrightness(currentBuff, outputBuff, flashInt);
  }
  else
  {
    static_assert(sizeof(Pixel) == sizeof(uint32_t));
    memcpy(outputBuff, currentBuff, goomInfo->getScreenInfo().size * sizeof(Pixel));
  }
}

void ConvolveFx::ConvolveImpl::createOutputWithBrightness(const Pixel* src,
                                                          uint32_t* dest,
                                                          const uint32_t flashInt)
{
  size_t i = 0; // < screenWidth * screenHeight
  for (uint32_t y = 0; y < goomInfo->getScreenInfo().height; y++)
  {
    for (uint32_t x = 0; x < goomInfo->getScreenInfo().width; x++)
    {
      dest[i] = getBrighterColorInt(flashInt, src[i], buffSettings.allowOverexposed).rgba();
      i++;
    }
  }
}

} // namespace goom
