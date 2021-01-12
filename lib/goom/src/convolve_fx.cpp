#include "convolve_fx.h"

#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colorutils.h"
#include "goomutils/parallel_utils.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

CEREAL_REGISTER_TYPE(goom::ConvolveFx)
CEREAL_REGISTER_POLYMORPHIC_RELATION(goom::IVisualFx, goom::ConvolveFx)

namespace goom
{

using namespace goom::utils;

class ConvolveFx::ConvolveImpl
{
public:
  ConvolveImpl() noexcept;
  explicit ConvolveImpl(Parallel&, std::shared_ptr<const PluginInfo>) noexcept;
  ConvolveImpl(const ConvolveImpl&) = delete;
  ConvolveImpl& operator=(const ConvolveImpl&) = delete;

  void setBuffSettings(const FXBuffSettings&);

  void convolve(const PixelBuffer& currentBuff, PixelBuffer& outputBuff);

  bool operator==(const ConvolveImpl&) const;

private:
  Parallel* parallel = nullptr;

  std::shared_ptr<const PluginInfo> goomInfo{};
  float screenBrightness = 100;
  float flashIntensity = 30;
  float factor = 0.5;
  FXBuffSettings buffSettings{};

  void createOutputWithBrightness(const PixelBuffer& src, PixelBuffer& dest, uint32_t flashInt);

  friend class cereal::access;
  template<class Archive>
  void save(Archive&) const;
  template<class Archive>
  void load(Archive&);
};

ConvolveFx::ConvolveFx() noexcept : fxImpl{new ConvolveImpl{}}
{
}

ConvolveFx::ConvolveFx(Parallel& p, const std::shared_ptr<const PluginInfo>& info) noexcept
  : fxImpl{new ConvolveImpl{p, info}}
{
}

ConvolveFx::~ConvolveFx() noexcept = default;

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

void ConvolveFx::apply(PixelBuffer&)
{
  throw std::logic_error("ConvolveFx::apply should never be called.");
}

void ConvolveFx::apply(PixelBuffer&, PixelBuffer&)
{
  throw std::logic_error("ConvolveFx::apply should never be called.");
}

void ConvolveFx::convolve(const PixelBuffer& currentBuff, PixelBuffer& outputBuff)
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

ConvolveFx::ConvolveImpl::ConvolveImpl() noexcept = default;

ConvolveFx::ConvolveImpl::ConvolveImpl(Parallel& p, std::shared_ptr<const PluginInfo> info) noexcept
  : parallel{&p}, goomInfo{std::move(info)}
{
}

inline void ConvolveFx::ConvolveImpl::setBuffSettings(const FXBuffSettings& settings)
{
  buffSettings = settings;
}

void ConvolveFx::ConvolveImpl::convolve(const PixelBuffer& currentBuff, PixelBuffer& outputBuff)
{
  const float flash = (factor * flashIntensity + screenBrightness) / 100.0F;
  const auto flashInt = static_cast<uint32_t>(std::round(flash * 256 + 0.0001F));
  constexpr float increaseRate = 1.3;
  constexpr float decayRate = 0.955;
  if (goomInfo->getSoundInfo().getTimeSinceLastGoom() == 0)
  {
    factor += goomInfo->getSoundInfo().getGoomPower() * increaseRate;
  }
  factor *= decayRate;

  if (std::fabs(1.0 - flash) < 0.02)
  {
    currentBuff.copyTo(outputBuff, goomInfo->getScreenInfo().size);
  }
  else
  {
    createOutputWithBrightness(currentBuff, outputBuff, flashInt);
  }
}

void ConvolveFx::ConvolveImpl::createOutputWithBrightness(const PixelBuffer& src,
                                                          PixelBuffer& dest,
                                                          const uint32_t flashInt)
{
  const auto setDestPixel = [&](const uint32_t i) {
    dest(i) = getBrighterColorInt(flashInt, src(i), buffSettings.allowOverexposed);
  };

  parallel->forLoop(goomInfo->getScreenInfo().size, setDestPixel);
}

} // namespace goom
