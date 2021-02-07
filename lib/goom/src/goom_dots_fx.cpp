#include "goom_dots_fx.h"

#include "goom_config.h"
#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormaps.h"
#include "goomutils/colorutils.h"
#include "goomutils/image_bitmaps.h"
#include "goomutils/logging_control.h"
#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "goomutils/random_colormaps.h"
#include "goomutils/random_colormaps_manager.h"
#include "goomutils/small_image_bitmaps.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace GOOM
{

using namespace GOOM::UTILS;
using COLOR_DATA::ColorMapName;

inline auto ChangeDotColorsEvent() -> bool
{
  return ProbabilityOfMInN(1, 3);
}

class GoomDotsFx::GoomDotsFxImpl
{
public:
  explicit GoomDotsFxImpl(const std::shared_ptr<const PluginInfo>& info) noexcept;
  ~GoomDotsFxImpl() noexcept = default;
  GoomDotsFxImpl(const GoomDotsFxImpl&) noexcept = delete;
  GoomDotsFxImpl(GoomDotsFxImpl&&) noexcept = delete;
  auto operator=(const GoomDotsFxImpl&) -> GoomDotsFxImpl& = delete;
  auto operator=(GoomDotsFxImpl&&) -> GoomDotsFxImpl& = delete;

  [[nodiscard]] auto GetResourcesDirectory() const -> const std::string&;
  void SetResourcesDirectory(const std::string& dirName);
  void SetSmallImageBitmaps(const SmallImageBitmaps& smallBitmaps);
  void SetBuffSettings(const FXBuffSettings& settings);

  void Start();

  void Apply(PixelBuffer& currentBuff);

private:
  const std::shared_ptr<const PluginInfo> m_goomInfo;
  const uint32_t m_pointWidth;
  const uint32_t m_pointHeight;

  const float m_pointWidthDiv2;
  const float m_pointHeightDiv2;
  const float m_pointWidthDiv3;
  const float m_pointHeightDiv3;

  std::string m_resourcesDirectory{};
  const SmallImageBitmaps* m_smallBitmaps{};
  SmallImageBitmaps::ImageNames m_currentBitmapName{};
  auto GetImageBitmap(size_t size) -> const ImageBitmap&;

  static constexpr size_t MIN_DOT_SIZE = 3;
  static constexpr size_t MAX_DOT_SIZE = 21;
  static_assert(MAX_DOT_SIZE <= SmallImageBitmaps::MAX_IMAGE_SIZE, "Max dot size mismatch.");

  GoomDraw m_draw;
  FXBuffSettings m_buffSettings{};

  const std::shared_ptr<WeightedColorMaps> m_colorMaps{
      std::make_shared<WeightedColorMaps>(Weights<ColorMapGroup>{{
          {ColorMapGroup::PERCEPTUALLY_UNIFORM_SEQUENTIAL, 10},
          {ColorMapGroup::SEQUENTIAL, 20},
          {ColorMapGroup::SEQUENTIAL2, 20},
          {ColorMapGroup::CYCLIC, 0},
          {ColorMapGroup::DIVERGING, 0},
          {ColorMapGroup::DIVERGING_BLACK, 0},
          {ColorMapGroup::QUALITATIVE, 0},
          {ColorMapGroup::MISC, 0},
      }})};
  RandomColorMapsManager m_colorMapsManager{};
  uint32_t m_colorMap1Id{};
  uint32_t m_colorMap2Id{};
  uint32_t m_colorMap3Id{};
  uint32_t m_colorMap4Id{};
  uint32_t m_colorMap5Id{};
  Pixel m_middleColor{};
  bool m_useSingleBufferOnly = true;
  bool m_useGrayScale = false;

  uint32_t m_loopVar = 0; // mouvement des points

  GammaCorrection m_gammaCorrect{5.0, 0.01};

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
};

GoomDotsFx::GoomDotsFx(const std::shared_ptr<const PluginInfo>& info) noexcept
  : m_fxImpl{new GoomDotsFxImpl{info}}
{
}

GoomDotsFx::~GoomDotsFx() noexcept = default;

auto GoomDotsFx::GetResourcesDirectory() const -> const std::string&
{
  return m_fxImpl->GetResourcesDirectory();
}

void GoomDotsFx::SetResourcesDirectory(const std::string& dirName)
{
  m_fxImpl->SetResourcesDirectory(dirName);
}

void GoomDotsFx::SetBuffSettings(const FXBuffSettings& settings)
{
  m_fxImpl->SetBuffSettings(settings);
}

void GoomDotsFx::SetSmallImageBitmaps(const SmallImageBitmaps& smallBitmaps)
{
  m_fxImpl->SetSmallImageBitmaps(smallBitmaps);
}

void GoomDotsFx::Start()
{
  m_fxImpl->Start();
}

void GoomDotsFx::Log([[maybe_unused]] const StatsLogValueFunc& l) const
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

GoomDotsFx::GoomDotsFxImpl::GoomDotsFxImpl(const std::shared_ptr<const PluginInfo>& info) noexcept
  : m_goomInfo(info),
    m_pointWidth{(m_goomInfo->GetScreenInfo().width * 2) / 5},
    m_pointHeight{(m_goomInfo->GetScreenInfo().height * 2) / 5},
    m_pointWidthDiv2{static_cast<float>(m_pointWidth / 2.0F)},
    m_pointHeightDiv2{static_cast<float>(m_pointHeight / 2.0F)},
    m_pointWidthDiv3{static_cast<float>(m_pointWidth / 3.0F)},
    m_pointHeightDiv3{static_cast<float>(m_pointHeight / 3.0F)},
    m_draw{m_goomInfo->GetScreenInfo().width, m_goomInfo->GetScreenInfo().height}
{
  m_colorMap1Id =
      m_colorMapsManager.AddColorMapInfo({m_colorMaps, ColorMapName::_NULL, RandomColorMaps::ALL});
  m_colorMap2Id =
      m_colorMapsManager.AddColorMapInfo({m_colorMaps, ColorMapName::_NULL, RandomColorMaps::ALL});
  m_colorMap3Id =
      m_colorMapsManager.AddColorMapInfo({m_colorMaps, ColorMapName::_NULL, RandomColorMaps::ALL});
  m_colorMap4Id =
      m_colorMapsManager.AddColorMapInfo({m_colorMaps, ColorMapName::_NULL, RandomColorMaps::ALL});
  m_colorMap5Id =
      m_colorMapsManager.AddColorMapInfo({m_colorMaps, ColorMapName::_NULL, RandomColorMaps::ALL});
}

void GoomDotsFx::GoomDotsFxImpl::Start()
{
  ChangeColors();
}

auto GoomDotsFx::GoomDotsFxImpl::GetResourcesDirectory() const -> const std::string&
{
  return m_resourcesDirectory;
}

void GoomDotsFx::GoomDotsFxImpl::SetResourcesDirectory(const std::string& dirName)
{
  m_resourcesDirectory = dirName;
}

inline void GoomDotsFx::GoomDotsFxImpl::SetSmallImageBitmaps(const SmallImageBitmaps& smallBitmaps)
{
  m_smallBitmaps = &smallBitmaps;
}

inline auto GoomDotsFx::GoomDotsFxImpl::GetImageBitmap(const size_t size) -> const ImageBitmap&
{
  return m_smallBitmaps->GetImageBitmap(
      m_currentBitmapName,
      stdnew::clamp(size % 2 != 0 ? size : size + 1, MIN_DOT_SIZE, MAX_DOT_SIZE));
}

void GoomDotsFx::GoomDotsFxImpl::ChangeColors()
{
  m_colorMapsManager.ChangeAllColorMapsNow();

  m_middleColor = RandomColorMaps::GetRandomColor(
      *m_colorMaps->GetRandomColorMapPtr(ColorMapGroup::MISC, RandomColorMaps::ALL), 0.1, 1.0);

  m_useSingleBufferOnly = ProbabilityOfMInN(0, 2);
  m_useGrayScale = ProbabilityOfMInN(0, 10);
}

void GoomDotsFx::GoomDotsFxImpl::SetBuffSettings(const FXBuffSettings& settings)
{
  m_buffSettings = settings;
  m_draw.SetBuffIntensity(m_buffSettings.buffIntensity);
  m_draw.SetAllowOverexposed(m_buffSettings.allowOverexposed);
}

void GoomDotsFx::GoomDotsFxImpl::Apply(PixelBuffer& currentBuff)
{
  uint32_t radius = MIN_DOT_SIZE / 2;
  if ((m_goomInfo->GetSoundInfo().GetTimeSinceLastGoom() == 0) || ChangeDotColorsEvent())
  {
    ChangeColors();
    radius = GetRandInRange(radius, MAX_DOT_SIZE / 2 + 1);
    m_currentBitmapName = static_cast<SmallImageBitmaps::ImageNames>(
        GetRandInRange(0U, static_cast<size_t>(SmallImageBitmaps::ImageNames::_SIZE)));
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
    m_loopVar += speedvarMult50Plus1;

    const uint32_t loopvarDivI = m_loopVar / i;
    const float iMult10 = 10.0F * i;

    const Pixel colors1 = m_colorMapsManager.GetColorMap(m_colorMap1Id).GetColor(t);
    //    const Pixel colors1 = GetColor(
    //        m_middleColor, m_colorMapsManager.GetColorMap(m_colorMap1Id).GetColor(t), brightness);
    const float color1T3 = i * 152.0F;
    const float color1T4 = 128.0F;
    const uint32_t color1Cycle = m_loopVar + i * 2032;

    const Pixel colors2 = m_colorMapsManager.GetColorMap(m_colorMap2Id).GetColor(t);
    //    const Pixel colors2 = GetColor(
    //        m_middleColor, m_colorMapsManager.GetColorMap(m_colorMap2Id).GetColor(t), brightness);
    const float color2T1 = pointWidthDiv2MultLarge / i + iMult10;
    const float color2T2 = pointHeightDiv2MultLarge / i + iMult10;
    const float color2T3 = 96.0F;
    const float color2T4 = i * 80.0F;
    const uint32_t color2Cycle = loopvarDivI;

    const Pixel colors3 = m_colorMapsManager.GetColorMap(m_colorMap3Id).GetColor(t);
    //    const Pixel colors3 = GetColor(
    //        m_middleColor, m_colorMapsManager.GetColorMap(m_colorMap3Id).GetColor(t), brightness);
    const float color3T1 = pointWidthDiv3MultLarge / i + iMult10;
    const float color3T2 = pointHeightDiv3MultLarge / i + iMult10;
    const float color3T3 = i + 122.0F;
    const float color3T4 = 134.0F;
    const uint32_t color3Cycle = loopvarDivI;

    const Pixel colors4 = m_colorMapsManager.GetColorMap(m_colorMap4Id).GetColor(t);
    //    const Pixel colors4 = GetColor(
    //        m_middleColor, m_colorMapsManager.GetColorMap(m_colorMap4Id).GetColor(t), brightness);
    const float color4T3 = 58.0F;
    const float color4T4 = i * 66.0F;
    const uint32_t color4Cycle = loopvarDivI;

    const Pixel colors5 = m_colorMapsManager.GetColorMap(m_colorMap5Id).GetColor(t);
    //    const Pixel colors5 = GetColor(
    //        m_middleColor, m_colorMapsManager.GetColorMap(m_colorMap5Id).GetColor(t), brightness);
    const float color5T1 = (pointWidthMultLarge + iMult10) / i;
    const float color5T2 = (pointHeightMultLarge + iMult10) / i;
    const float color5T3 = 66.0F;
    const float color5T4 = 74.0F;
    const uint32_t color5Cycle = m_loopVar + i * 500;

    DotFilter(currentBuff, colors1, color1T1, color1T2, color1T3, color1T4, color1Cycle, radius);
    DotFilter(currentBuff, colors2, color2T1, color2T2, color2T3, color2T4, color2Cycle, radius);
    DotFilter(currentBuff, colors3, color3T1, color3T2, color3T3, color3T4, color3Cycle, radius);
    DotFilter(currentBuff, colors4, color4T1, color4T2, color4T3, color4T4, color4Cycle, radius);
    DotFilter(currentBuff, colors5, color5T1, color5T2, color5T3, color5T4, color5Cycle, radius);

    t += t_step;
  }
}

auto GoomDotsFx::GoomDotsFxImpl::GetColor(const Pixel& color0,
                                          const Pixel& color1,
                                          const float brightness) -> Pixel
{
  constexpr float T_MIN = 0.9999;
  constexpr float T_MAX = 1.0;
  const float tMix = GetRandInRange(T_MIN, T_MAX);
  Pixel color{};
  if (!m_useGrayScale)
  {
    color = IColorMap::GetColorMix(color0, color1, tMix);
  }
  else
  {
    color =
        Pixel{/*.channels*/ {/*.r = */ static_cast<uint8_t>(tMix * channel_limits<uint32_t>::max()),
                             /*.g = */ static_cast<uint8_t>(tMix * channel_limits<uint32_t>::max()),
                             /*.b = */ static_cast<uint8_t>(tMix * channel_limits<uint32_t>::max()),
                             /*.a = */ 0xff}};
  }

  return m_gammaCorrect.GetCorrection(brightness, color);
}

auto GoomDotsFx::GoomDotsFxImpl::GetLargeSoundFactor(const SoundInfo& soundInfo) -> float
{
  float largeFactor = soundInfo.GetSpeed() / 150.0F + soundInfo.GetVolume() / 1.5F;
  if (largeFactor > 1.5F)
  {
    largeFactor = 1.5F;
  }
  return largeFactor;
}

void GoomDotsFx::GoomDotsFxImpl::DotFilter(PixelBuffer& currentBuff,
                                           const Pixel& color,
                                           const float t1,
                                           const float t2,
                                           const float t3,
                                           const float t4,
                                           const uint32_t cycle,
                                           uint32_t radius)
{
  const auto xOffset = static_cast<uint32_t>(t1 * std::cos(static_cast<float>(cycle) / t3));
  const auto yOffset = static_cast<uint32_t>(t2 * std::sin(static_cast<float>(cycle) / t4));
  const auto x0 = static_cast<int>(m_goomInfo->GetScreenInfo().width / 2 + xOffset);
  const auto y0 = static_cast<int>(m_goomInfo->GetScreenInfo().height / 2 + yOffset);

  const uint32_t diameter = 2 * radius + 1; // must be odd
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
  constexpr float BRIGHTNESS = 20.0F;
  const auto getColor = [&]([[maybe_unused]] const size_t x, [[maybe_unused]] const size_t y,
                            const Pixel& b) -> Pixel {
    // const Pixel newColor = x == xMid && y == yMid ? m_middleColor : color;
    return m_gammaCorrect.GetCorrection(
        BRIGHTNESS, GetColorMultiply(b, color, m_buffSettings.allowOverexposed));
  };

  m_draw.Bitmap(currentBuff, xMid, yMid, GetImageBitmap(diameter), getColor);
}

} // namespace GOOM
