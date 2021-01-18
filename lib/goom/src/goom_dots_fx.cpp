#include "goom_dots_fx.h"

#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"
#include "goomutils/colorutils.h"
#include "goomutils/logging_control.h"
#undef NO_LOGGING
#include "goomutils/logging.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

CEREAL_REGISTER_TYPE(GOOM::GoomDotsFx)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GOOM::IVisualFx, GOOM::GoomDotsFx)

namespace GOOM
{

using namespace GOOM::UTILS;

inline auto ChangeDotColorsEvent() -> bool
{
  return probabilityOfMInN(1, 3);
}

class GoomDotsFx::GoomDotsImpl
{
public:
  GoomDotsImpl() noexcept;
  explicit GoomDotsImpl(const std::shared_ptr<const PluginInfo>& info) noexcept;
  ~GoomDotsImpl() noexcept = default;
  GoomDotsImpl(const GoomDotsImpl&) noexcept = delete;
  GoomDotsImpl(GoomDotsImpl&&) noexcept = delete;
  auto operator=(const GoomDotsImpl&) -> GoomDotsImpl& = delete;
  auto operator=(GoomDotsImpl&&) -> GoomDotsImpl& = delete;

  void SetBuffSettings(const FXBuffSettings& settings);

  void Apply(PixelBuffer& currentBuff);
  void Apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff);

  auto operator==(const GoomDotsImpl& d) const -> bool;

private:
  std::shared_ptr<const PluginInfo> m_goomInfo{};
  uint32_t m_pointWidth = 0;
  uint32_t m_pointHeight = 0;

  float m_pointWidthDiv2 = 0;
  float m_pointHeightDiv2 = 0;
  float m_pointWidthDiv3 = 0;
  float m_pointHeightDiv3 = 0;

  GoomDraw m_draw{};
  FXBuffSettings m_buffSettings{};

  UTILS::WeightedColorMaps m_colorMaps{};
  const UTILS::IColorMap* m_colorMap1 = nullptr;
  const UTILS::IColorMap* m_colorMap2 = nullptr;
  const UTILS::IColorMap* m_colorMap3 = nullptr;
  const UTILS::IColorMap* m_colorMap4 = nullptr;
  const UTILS::IColorMap* m_colorMap5 = nullptr;
  Pixel m_middleColor{};
  bool m_useSingleBufferOnly = true;
  bool m_useGrayScale = false;
  uint32_t m_loopvar = 0; // mouvement des points

  GammaCorrection m_gammaCorrect{4.2, 0.1};

  void ChangeColors();

  [[nodiscard]] auto GetColor(const Pixel& color0, const Pixel& color1, float brightness) -> Pixel;

  [[nodiscard]] static auto GetLargeSoundFactor(const SoundInfo& soundInfo) -> float;

  void DotFilter(PixelBuffer& currentBuff,
                 const Pixel& color,
                 float t1,
                 float t2,
                 float t3,
                 float t4,
                 uint32_t cycle,
                 uint32_t radius);

  friend class cereal::access;
  template<class Archive>
  void save(Archive& ar) const;
  template<class Archive>
  void load(Archive& ar);
};

GoomDotsFx::GoomDotsFx() noexcept : m_fxImpl{new GoomDotsImpl{}}
{
}

GoomDotsFx::GoomDotsFx(const std::shared_ptr<const PluginInfo>& info) noexcept
  : m_fxImpl{new GoomDotsImpl{info}}
{
}

GoomDotsFx::~GoomDotsFx() noexcept = default;

auto GoomDotsFx::operator==(const GoomDotsFx& d) const -> bool
{
  return m_fxImpl->operator==(*d.m_fxImpl);
}

void GoomDotsFx::SetBuffSettings(const FXBuffSettings& settings)
{
  m_fxImpl->SetBuffSettings(settings);
}

void GoomDotsFx::Start()
{
}

void GoomDotsFx::Finish()
{
}

auto GoomDotsFx::GetFxName() const -> std::string
{
  return "goom dots";
}

void GoomDotsFx::Apply(PixelBuffer& currentBuff)
{
  if (!m_enabled)
  {
    return;
  }
  m_fxImpl->Apply(currentBuff);
}

void GoomDotsFx::Apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff)
{
  if (!m_enabled)
  {
    return;
  }
  m_fxImpl->Apply(currentBuff, nextBuff);
}

template<class Archive>
void GoomDotsFx::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_enabled), CEREAL_NVP(m_fxImpl));
}

// Need to explicitly instantiate template functions for serialization.
template void GoomDotsFx::serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);
template void GoomDotsFx::serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template void GoomDotsFx::GoomDotsImpl::save<cereal::JSONOutputArchive>(
    cereal::JSONOutputArchive&) const;
template void GoomDotsFx::GoomDotsImpl::load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template<class Archive>
void GoomDotsFx::GoomDotsImpl::save(Archive& ar) const
{
  ar(CEREAL_NVP(m_goomInfo), CEREAL_NVP(m_pointWidth), CEREAL_NVP(m_pointHeight),
     CEREAL_NVP(m_pointWidthDiv2), CEREAL_NVP(m_pointHeightDiv2), CEREAL_NVP(m_pointWidthDiv3),
     CEREAL_NVP(m_pointHeightDiv3), CEREAL_NVP(m_draw), CEREAL_NVP(m_buffSettings),
     CEREAL_NVP(m_middleColor), CEREAL_NVP(m_useSingleBufferOnly), CEREAL_NVP(m_useGrayScale),
     CEREAL_NVP(m_loopvar));
}

template<class Archive>
void GoomDotsFx::GoomDotsImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(m_goomInfo), CEREAL_NVP(m_pointWidth), CEREAL_NVP(m_pointHeight),
     CEREAL_NVP(m_pointWidthDiv2), CEREAL_NVP(m_pointHeightDiv2), CEREAL_NVP(m_pointWidthDiv3),
     CEREAL_NVP(m_pointHeightDiv3), CEREAL_NVP(m_draw), CEREAL_NVP(m_buffSettings),
     CEREAL_NVP(m_middleColor), CEREAL_NVP(m_useSingleBufferOnly), CEREAL_NVP(m_useGrayScale),
     CEREAL_NVP(m_loopvar));
}

auto GoomDotsFx::GoomDotsImpl::operator==(const GoomDotsImpl& d) const -> bool
{
  if (m_goomInfo == nullptr && d.m_goomInfo != nullptr)
  {
    return false;
  }
  if (m_goomInfo != nullptr && d.m_goomInfo == nullptr)
  {
    return false;
  }

  return ((m_goomInfo == nullptr && d.m_goomInfo == nullptr) || (*m_goomInfo == *d.m_goomInfo)) &&
         m_pointWidth == d.m_pointWidth && m_pointHeight == d.m_pointHeight &&
         m_pointWidthDiv2 == d.m_pointWidthDiv2 && m_pointHeightDiv2 == d.m_pointHeightDiv2 &&
         m_pointWidthDiv3 == d.m_pointWidthDiv3 && m_pointHeightDiv3 == d.m_pointHeightDiv3 &&
         m_draw == d.m_draw && m_buffSettings == d.m_buffSettings &&
         m_middleColor == d.m_middleColor && m_useSingleBufferOnly == d.m_useSingleBufferOnly &&
         m_useGrayScale == d.m_useGrayScale && m_loopvar == d.m_loopvar;
}

GoomDotsFx::GoomDotsImpl::GoomDotsImpl() noexcept = default;

GoomDotsFx::GoomDotsImpl::GoomDotsImpl(const std::shared_ptr<const PluginInfo>& info) noexcept
  : m_goomInfo(info),
    m_pointWidth{(m_goomInfo->GetScreenInfo().width * 2) / 5},
    m_pointHeight{(m_goomInfo->GetScreenInfo().height * 2) / 5},
    m_pointWidthDiv2{static_cast<float>(m_pointWidth / 2.0F)},
    m_pointHeightDiv2{static_cast<float>(m_pointHeight / 2.0F)},
    m_pointWidthDiv3{static_cast<float>(m_pointWidth / 3.0F)},
    m_pointHeightDiv3{static_cast<float>(m_pointHeight / 3.0F)},
    m_draw{m_goomInfo->GetScreenInfo().width, m_goomInfo->GetScreenInfo().height},
    m_colorMaps{Weights<ColorMapGroup>{{
        {ColorMapGroup::PERCEPTUALLY_UNIFORM_SEQUENTIAL, 10},
        {ColorMapGroup::SEQUENTIAL, 20},
        {ColorMapGroup::SEQUENTIAL2, 20},
        {ColorMapGroup::CYCLIC, 0},
        {ColorMapGroup::DIVERGING, 0},
        {ColorMapGroup::DIVERGING_BLACK, 0},
        {ColorMapGroup::QUALITATIVE, 0},
        {ColorMapGroup::MISC, 0},
    }}}
{
  ChangeColors();
}

void GoomDotsFx::GoomDotsImpl::SetBuffSettings(const FXBuffSettings& settings)
{
  m_buffSettings = settings;
  m_draw.SetBuffIntensity(m_buffSettings.buffIntensity);
  m_draw.SetAllowOverexposed(m_buffSettings.allowOverexposed);
}

void GoomDotsFx::GoomDotsImpl::ChangeColors()
{
  m_colorMap1 = &m_colorMaps.GetRandomColorMap();
  m_colorMap2 = &m_colorMaps.GetRandomColorMap();
  m_colorMap3 = &m_colorMaps.GetRandomColorMap();
  m_colorMap4 = &m_colorMaps.GetRandomColorMap();
  m_colorMap5 = &m_colorMaps.GetRandomColorMap();
  m_middleColor = m_colorMaps.GetRandomColorMap(ColorMapGroup::MISC).GetRandomColor(0.1, 1);

  m_useSingleBufferOnly = probabilityOfMInN(0, 2);
  m_useGrayScale = probabilityOfMInN(0, 10);
}

void GoomDotsFx::GoomDotsImpl::Apply(PixelBuffer& currentBuff)
{
  uint32_t radius = 3;
  if ((m_goomInfo->GetSoundInfo().GetTimeSinceLastGoom() == 0) || ChangeDotColorsEvent())
  {
    ChangeColors();
    radius = getRandInRange(5U, 7U);
  }

  const float largeFactor = GetLargeSoundFactor(m_goomInfo->GetSoundInfo());
  const uint32_t speedvarMult80Plus15 = m_goomInfo->GetSoundInfo().GetSpeed() * 80 + 15;
  const uint32_t speedvarMult50Plus1 = m_goomInfo->GetSoundInfo().GetSpeed() * 50 + 1;

  const float pointWidthDiv2MultLarge = m_pointWidthDiv2 * largeFactor;
  const float pointHeightDiv2MultLarge = m_pointHeightDiv2 * largeFactor;
  const float pointWidthDiv3MultLarge = (m_pointWidthDiv3 + 5.0F) * largeFactor;
  const float pointHeightDiv3MultLarge = (m_pointHeightDiv3 + 5.0F) * largeFactor;
  const float pointWidthMultLarge = m_pointWidth * largeFactor;
  const float pointHeightMultLarge = m_pointHeight * largeFactor;

  const float color1T1 = (m_pointWidth - 6.0F) * largeFactor + 5.0F;
  const float color1T2 = (m_pointHeight - 6.0F) * largeFactor + 5.0F;
  const float color4T1 = m_pointHeightDiv3 * largeFactor + 20.0F;
  const float color4T2 = color4T1;

  const size_t speedvarMult80Plus15Div15 = speedvarMult80Plus15 / 15;
  constexpr float T_MIN = 0.5;
  constexpr float T_MAX = 1.0;
  const float t_step = (T_MAX - T_MIN) / static_cast<float>(speedvarMult80Plus15Div15);

  float t = T_MIN;
  for (uint32_t i = 1; i <= speedvarMult80Plus15Div15; i++)
  {
    m_loopvar += speedvarMult50Plus1;

    const uint32_t loopvarDivI = m_loopvar / i;
    const float iMult10 = 10.0F * i;
    const float brightness = 1.5F + 1.0F - t;

    const Pixel colors1 = GetColor(m_middleColor, m_colorMap1->GetColor(t), brightness);
    const float color1T3 = i * 152.0F;
    const float color1T4 = 128.0F;
    const uint32_t color1Cycle = m_loopvar + i * 2032;

    const Pixel colors2 = GetColor(m_middleColor, m_colorMap2->GetColor(t), brightness);
    const float color2T1 = pointWidthDiv2MultLarge / i + iMult10;
    const float color2T2 = pointHeightDiv2MultLarge / i + iMult10;
    const float color2T3 = 96.0F;
    const float color2T4 = i * 80.0F;
    const uint32_t color2Cycle = loopvarDivI;

    const Pixel colors3 = GetColor(m_middleColor, m_colorMap3->GetColor(t), brightness);
    const float color3T1 = pointWidthDiv3MultLarge / i + iMult10;
    const float color3T2 = pointHeightDiv3MultLarge / i + iMult10;
    const float color3T3 = i + 122.0F;
    const float color3T4 = 134.0F;
    const uint32_t color3Cycle = loopvarDivI;

    const Pixel colors4 = GetColor(m_middleColor, m_colorMap4->GetColor(t), brightness);
    const float color4T3 = 58.0F;
    const float color4T4 = i * 66.0F;
    const uint32_t color4Cycle = loopvarDivI;

    const Pixel colors5 = GetColor(m_middleColor, m_colorMap5->GetColor(t), brightness);
    const float color5T1 = (pointWidthMultLarge + iMult10) / i;
    const float color5T2 = (pointHeightMultLarge + iMult10) / i;
    const float color5T3 = 66.0F;
    const float color5T4 = 74.0F;
    const uint32_t color5Cycle = m_loopvar + i * 500;

    DotFilter(currentBuff, colors1, color1T1, color1T2, color1T3, color1T4, color1Cycle, radius);
    DotFilter(currentBuff, colors2, color2T1, color2T2, color2T3, color2T4, color2Cycle, radius);
    DotFilter(currentBuff, colors3, color3T1, color3T2, color3T3, color3T4, color3Cycle, radius);
    DotFilter(currentBuff, colors4, color4T1, color4T2, color4T3, color4T4, color4Cycle, radius);
    DotFilter(currentBuff, colors5, color5T1, color5T2, color5T3, color5T4, color5Cycle, radius);

    t += t_step;
  }
}

void GoomDotsFx::GoomDotsImpl::Apply([[maybe_unused]] PixelBuffer& currentBuff,
                                     [[maybe_unused]] PixelBuffer& nextBuff)
{
  throw std::logic_error("GoomDotsFx::GoomDotsImpl::Apply should never be called.");
}

auto GoomDotsFx::GoomDotsImpl::GetColor(const Pixel& color0,
                                        const Pixel& color1,
                                        const float brightness) -> Pixel
{
  constexpr float T_MIN = 0.5;
  constexpr float T_MAX = 1.0;
  const float t = getRandInRange(T_MIN, T_MAX);
  Pixel color{};
  if (!m_useGrayScale)
  {
    color = IColorMap::GetColorMix(color0, color1, t);
  }
  else
  {
    color = Pixel{.channels{.r = static_cast<uint8_t>(t * channel_limits<uint32_t>::max()),
                            .g = static_cast<uint8_t>(t * channel_limits<uint32_t>::max()),
                            .b = static_cast<uint8_t>(t * channel_limits<uint32_t>::max()),
                            .a = 0xff}};
  }

  return m_gammaCorrect.getCorrection(brightness, color);
}

auto GoomDotsFx::GoomDotsImpl::GetLargeSoundFactor(const SoundInfo& soundInfo) -> float
{
  float largeFactor = soundInfo.GetSpeed() / 150.0F + soundInfo.GetVolume() / 1.5F;
  if (largeFactor > 1.5F)
  {
    largeFactor = 1.5F;
  }
  return largeFactor;
}

void GoomDotsFx::GoomDotsImpl::DotFilter(PixelBuffer& currentBuff,
                                         const Pixel& color,
                                         const float t1,
                                         const float t2,
                                         const float t3,
                                         const float t4,
                                         const uint32_t cycle,
                                         const uint32_t radius)
{
  const auto xOffset = static_cast<uint32_t>(t1 * std::cos(static_cast<float>(cycle) / t3));
  const auto yOffset = static_cast<uint32_t>(t2 * std::sin(static_cast<float>(cycle) / t4));
  const auto x0 = static_cast<int>(m_goomInfo->GetScreenInfo().width / 2 + xOffset);
  const auto y0 = static_cast<int>(m_goomInfo->GetScreenInfo().height / 2 + yOffset);

  const uint32_t diameter = 2 * radius;
  const auto screenWidthLessDiameter =
      static_cast<int>(m_goomInfo->GetScreenInfo().width - diameter);
  const auto screenHeightLessDiameter =
      static_cast<int>(m_goomInfo->GetScreenInfo().height - diameter);

  if ((x0 < static_cast<int>(diameter)) || (y0 < static_cast<int>(diameter)) ||
      (x0 >= screenWidthLessDiameter) || (y0 >= screenHeightLessDiameter))
  {
    return;
  }

  const auto xMid = x0 + static_cast<int32_t>(radius);
  const auto yMid = y0 + static_cast<int32_t>(radius);
  m_draw.FilledCircle(currentBuff, xMid, yMid, static_cast<int>(radius), color);
}

} // namespace GOOM
