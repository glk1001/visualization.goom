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
  ~ConvolveImpl() noexcept = default;
  ConvolveImpl(const ConvolveImpl&) = delete;
  ConvolveImpl(const ConvolveImpl&&) = delete;
  auto operator=(const ConvolveImpl&) -> ConvolveImpl& = delete;
  auto operator=(const ConvolveImpl&&) -> ConvolveImpl& = delete;

  void SetBuffSettings(const FXBuffSettings& settings);

  void Convolve(const PixelBuffer& currentBuff, PixelBuffer& outputBuff);

  auto operator==(const ConvolveImpl& c) const -> bool;

private:
  Parallel* m_parallel = nullptr;

  std::shared_ptr<const PluginInfo> m_goomInfo{};
  float m_screenBrightness = 100;
  float m_flashIntensity = 30;
  float m_factor = 0.5;
  FXBuffSettings buffSettings{};

  void CreateOutputWithBrightness(const PixelBuffer& src, PixelBuffer& dest, uint32_t flashInt);

  friend class cereal::access;
  template<class Archive>
  void save(Archive&) const;
  template<class Archive>
  void load(Archive&);
};

ConvolveFx::ConvolveFx() noexcept : m_fxImpl{new ConvolveImpl{}}
{
}

ConvolveFx::ConvolveFx(Parallel& p, const std::shared_ptr<const PluginInfo>& info) noexcept
  : m_fxImpl{new ConvolveImpl{p, info}}
{
}

ConvolveFx::~ConvolveFx() noexcept = default;

auto ConvolveFx::operator==(const ConvolveFx& c) const -> bool
{
  return m_fxImpl->operator==(*c.m_fxImpl);
}

void ConvolveFx::setBuffSettings(const FXBuffSettings& settings)
{
  m_fxImpl->SetBuffSettings(settings);
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

void ConvolveFx::apply([[maybe_unused]] PixelBuffer& currentBuff)
{
  throw std::logic_error("ConvolveFx::apply should never be called.");
}

void ConvolveFx::apply([[maybe_unused]] PixelBuffer& currentBuff,
                       [[maybe_unused]] PixelBuffer& nextBuff)
{
  throw std::logic_error("ConvolveFx::apply should never be called.");
}

void ConvolveFx::Convolve(const PixelBuffer& currentBuff, PixelBuffer& outputBuff)
{
  if (!m_enabled)
  {
    return;
  }

  m_fxImpl->Convolve(currentBuff, outputBuff);
}

template<class Archive>
void ConvolveFx::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_enabled), CEREAL_NVP(m_fxImpl));
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
  ar(CEREAL_NVP(m_goomInfo), CEREAL_NVP(buffSettings), CEREAL_NVP(m_screenBrightness),
     CEREAL_NVP(m_flashIntensity), CEREAL_NVP(m_factor));
}

template<class Archive>
void ConvolveFx::ConvolveImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(m_goomInfo), CEREAL_NVP(buffSettings), CEREAL_NVP(m_screenBrightness),
     CEREAL_NVP(m_flashIntensity), CEREAL_NVP(m_factor));
}

auto ConvolveFx::ConvolveImpl::operator==(const ConvolveImpl& c) const -> bool
{
  if (m_goomInfo == nullptr && c.m_goomInfo != nullptr)
  {
    return false;
  }
  if (m_goomInfo != nullptr && c.m_goomInfo == nullptr)
  {
    return false;
  }

  return ((m_goomInfo == nullptr && c.m_goomInfo == nullptr) || (*m_goomInfo == *c.m_goomInfo)) &&
         std::fabs(m_screenBrightness - c.m_screenBrightness) < 0.000001 &&
         std::fabs(m_flashIntensity - c.m_flashIntensity) < 0.000001 &&
         std::fabs(m_factor - c.m_factor) < 0.000001 && buffSettings == c.buffSettings;
}

ConvolveFx::ConvolveImpl::ConvolveImpl() noexcept = default;

ConvolveFx::ConvolveImpl::ConvolveImpl(Parallel& p, std::shared_ptr<const PluginInfo> info) noexcept
  : m_parallel{&p}, m_goomInfo{std::move(info)}
{
}

inline void ConvolveFx::ConvolveImpl::SetBuffSettings(const FXBuffSettings& settings)
{
  buffSettings = settings;
}

void ConvolveFx::ConvolveImpl::Convolve(const PixelBuffer& currentBuff, PixelBuffer& outputBuff)
{
  const float flash = (m_factor * m_flashIntensity + m_screenBrightness) / 100.0F;
  const auto flashInt = static_cast<uint32_t>(std::round(flash * 256 + 0.0001F));
  constexpr float INCREASE_RATE = 1.3;
  constexpr float DECAY_RATE = 0.955;
  if (m_goomInfo->getSoundInfo().getTimeSinceLastGoom() == 0)
  {
    m_factor += m_goomInfo->getSoundInfo().getGoomPower() * INCREASE_RATE;
  }
  m_factor *= DECAY_RATE;

  if (std::fabs(1.0 - flash) < 0.02)
  {
    currentBuff.CopyTo(outputBuff, m_goomInfo->getScreenInfo().size);
  }
  else
  {
    CreateOutputWithBrightness(currentBuff, outputBuff, flashInt);
  }
}

void ConvolveFx::ConvolveImpl::CreateOutputWithBrightness(const PixelBuffer& src,
                                                          PixelBuffer& dest,
                                                          const uint32_t flashInt)
{
  const auto setDestPixel = [&](const uint32_t i) {
    dest(i) = getBrighterColorInt(flashInt, src(i), buffSettings.allowOverexposed);
  };

  m_parallel->forLoop(m_goomInfo->getScreenInfo().size, setDestPixel);
}

} // namespace goom
