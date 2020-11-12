#include "convolve_fx.h"

#include "goom_config.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colorutils.h"
#include "goomutils/goomrand.h"
#include "goomutils/mathutils.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cmath>
#include <cstdint>
#include <string>

CEREAL_REGISTER_TYPE(goom::ConvolveFx);
CEREAL_REGISTER_POLYMORPHIC_RELATION(goom::VisualFx, goom::ConvolveFx);

namespace goom
{

using namespace goom::utils;

class ConvolveFx::ConvolveImpl
{
public:
  ConvolveImpl() noexcept;
  explicit ConvolveImpl(const std::shared_ptr<const PluginInfo>&) noexcept;

  void setBuffSettings(const FXBuffSettings&);

  void convolve(const Pixel* currentBuff, uint32_t* outputBuff);

  bool operator==(const ConvolveImpl&) const;

private:
  std::shared_ptr<const PluginInfo> goomInfo{};
  float screenBrightness = 100;
  float flashIntensity = 30;
  float factor = 0.5;
  FXBuffSettings buffSettings{};

  void createOutputWithBrightness(const Pixel* src, uint32_t* dest, const uint32_t flashInt);

  friend class cereal::access;
  template<class Archive>
  void save(Archive&) const;
  template<class Archive>
  void load(Archive&);
};

ConvolveFx::ConvolveFx() noexcept : fxImpl{new ConvolveImpl{}}
{
}

ConvolveFx::ConvolveFx(const std::shared_ptr<const PluginInfo>& info) noexcept
  : fxImpl{new ConvolveImpl{info}}
{
}

ConvolveFx::~ConvolveFx() noexcept
{
}

bool ConvolveFx::operator==(const ConvolveFx& c) const
{
  return fxImpl->operator==(*c.fxImpl);
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

void ConvolveFx::convolve(const Pixel* currentBuff, uint32_t* outputBuff)
{
  if (!enabled)
  {
    return;
  }

  fxImpl->convolve(currentBuff, outputBuff);
}

template<class Archive>
void ConvolveFx::serialize(Archive& ar)
{
  ar(CEREAL_NVP(enabled), CEREAL_NVP(fxImpl));
}

// Need to explicitly instantiate template functions for serialization.
template void ConvolveFx::serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);
template void ConvolveFx::serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template void ConvolveFx::ConvolveImpl::save<cereal::JSONOutputArchive>(
    cereal::JSONOutputArchive&) const;
template void ConvolveFx::ConvolveImpl::load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template<class Archive>
void ConvolveFx::ConvolveImpl::save(Archive& ar) const
{
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(buffSettings), CEREAL_NVP(screenBrightness),
     CEREAL_NVP(flashIntensity), CEREAL_NVP(factor));
}

template<class Archive>
void ConvolveFx::ConvolveImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(buffSettings), CEREAL_NVP(screenBrightness),
     CEREAL_NVP(flashIntensity), CEREAL_NVP(factor));
}

bool ConvolveFx::ConvolveImpl::operator==(const ConvolveImpl& c) const
{
  if (goomInfo == nullptr && c.goomInfo != nullptr)
  {
    return false;
  }
  if (goomInfo != nullptr && c.goomInfo == nullptr)
  {
    return false;
  }

  return ((goomInfo == nullptr && c.goomInfo == nullptr) || (*goomInfo == *c.goomInfo)) &&
         std::fabs(screenBrightness - c.screenBrightness) < 0.000001 &&
         std::fabs(flashIntensity - c.flashIntensity) < 0.000001 &&
         std::fabs(factor - c.factor) < 0.000001 && buffSettings == c.buffSettings;
}

ConvolveFx::ConvolveImpl::ConvolveImpl() noexcept
{
}

ConvolveFx::ConvolveImpl::ConvolveImpl(const std::shared_ptr<const PluginInfo>& info) noexcept
  : goomInfo{info}
{
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
