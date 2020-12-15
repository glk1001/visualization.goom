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

CEREAL_REGISTER_TYPE(goom::GoomDotsFx);
CEREAL_REGISTER_POLYMORPHIC_RELATION(goom::VisualFx, goom::GoomDotsFx);

namespace goom
{

using namespace goom::utils;

inline bool changeDotColorsEvent()
{
  return probabilityOfMInN(1, 3);
}

class GoomDotsFx::GoomDotsImpl
{
public:
  GoomDotsImpl() noexcept;
  explicit GoomDotsImpl(const std::shared_ptr<const PluginInfo>&) noexcept;
  GoomDotsImpl(const GoomDotsImpl&) = delete;
  GoomDotsImpl& operator=(const GoomDotsImpl&) = delete;

  void setBuffSettings(const FXBuffSettings&);

  void apply(PixelBuffer& prevBuff, PixelBuffer& currentBuff);

  bool operator==(const GoomDotsImpl&) const;

private:
  std::shared_ptr<const PluginInfo> goomInfo{};
  uint32_t pointWidth = 0;
  uint32_t pointHeight = 0;

  float pointWidthDiv2 = 0;
  float pointHeightDiv2 = 0;
  float pointWidthDiv3 = 0;
  float pointHeightDiv3 = 0;

  GoomDraw draw{};
  FXBuffSettings buffSettings{};

  utils::WeightedColorMaps colorMaps{};
  const utils::ColorMap* colorMap1 = nullptr;
  const utils::ColorMap* colorMap2 = nullptr;
  const utils::ColorMap* colorMap3 = nullptr;
  const utils::ColorMap* colorMap4 = nullptr;
  const utils::ColorMap* colorMap5 = nullptr;
  Pixel middleColor{};
  bool useSingleBufferOnly = true;
  bool useGrayScale = false;
  uint32_t loopvar = 0; // mouvement des points

  GammaCorrection gammaCorrect{4.2, 0.1};

  void changeColors();

  [[nodiscard]] std::vector<Pixel> getColors(const Pixel& color0,
                                             const Pixel& color1,
                                             float brightness,
                                             size_t numPts);

  [[nodiscard]] float getLargeSoundFactor(const SoundInfo&) const;

  void dotFilter(PixelBuffer& prevBuff,
                 PixelBuffer& currentBuff,
                 const std::vector<Pixel>& colors,
                 float t1,
                 float t2,
                 float t3,
                 float t4,
                 float brightness,
                 uint32_t cycle,
                 uint32_t radius);

  friend class cereal::access;
  template<class Archive>
  void save(Archive&) const;
  template<class Archive>
  void load(Archive&);
};

GoomDotsFx::GoomDotsFx() noexcept : fxImpl{new GoomDotsImpl{}}
{
}

GoomDotsFx::GoomDotsFx(const std::shared_ptr<const PluginInfo>& info) noexcept
  : fxImpl{new GoomDotsImpl{info}}
{
}

GoomDotsFx::~GoomDotsFx() noexcept = default;

bool GoomDotsFx::operator==(const GoomDotsFx& d) const
{
  return fxImpl->operator==(*d.fxImpl);
}

void GoomDotsFx::setBuffSettings(const FXBuffSettings& settings)
{
  fxImpl->setBuffSettings(settings);
}

void GoomDotsFx::start()
{
}

void GoomDotsFx::finish()
{
}

std::string GoomDotsFx::getFxName() const
{
  return "goom dots";
}

void GoomDotsFx::apply(PixelBuffer& prevBuff, PixelBuffer& currentBuff)
{
  if (!enabled)
  {
    return;
  }
  fxImpl->apply(prevBuff, currentBuff);
}

template<class Archive>
void GoomDotsFx::serialize(Archive& ar)
{
  ar(CEREAL_NVP(enabled), CEREAL_NVP(fxImpl));
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
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(pointWidth), CEREAL_NVP(pointHeight),
     CEREAL_NVP(pointWidthDiv2), CEREAL_NVP(pointHeightDiv2), CEREAL_NVP(pointWidthDiv3),
     CEREAL_NVP(pointHeightDiv3), CEREAL_NVP(draw), CEREAL_NVP(buffSettings),
     CEREAL_NVP(middleColor), CEREAL_NVP(useSingleBufferOnly), CEREAL_NVP(useGrayScale),
     CEREAL_NVP(loopvar));
}

template<class Archive>
void GoomDotsFx::GoomDotsImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(pointWidth), CEREAL_NVP(pointHeight),
     CEREAL_NVP(pointWidthDiv2), CEREAL_NVP(pointHeightDiv2), CEREAL_NVP(pointWidthDiv3),
     CEREAL_NVP(pointHeightDiv3), CEREAL_NVP(draw), CEREAL_NVP(buffSettings),
     CEREAL_NVP(middleColor), CEREAL_NVP(useSingleBufferOnly), CEREAL_NVP(useGrayScale),
     CEREAL_NVP(loopvar));
}

bool GoomDotsFx::GoomDotsImpl::operator==(const GoomDotsImpl& d) const
{
  if (goomInfo == nullptr && d.goomInfo != nullptr)
  {
    return false;
  }
  if (goomInfo != nullptr && d.goomInfo == nullptr)
  {
    return false;
  }

  return ((goomInfo == nullptr && d.goomInfo == nullptr) || (*goomInfo == *d.goomInfo)) &&
         pointWidth == d.pointWidth && pointHeight == d.pointHeight &&
         pointWidthDiv2 == d.pointWidthDiv2 && pointHeightDiv2 == d.pointHeightDiv2 &&
         pointWidthDiv3 == d.pointWidthDiv3 && pointHeightDiv3 == d.pointHeightDiv3 &&
         draw == d.draw && buffSettings == d.buffSettings && middleColor == d.middleColor &&
         useSingleBufferOnly == d.useSingleBufferOnly && useGrayScale == d.useGrayScale &&
         loopvar == d.loopvar;
}

GoomDotsFx::GoomDotsImpl::GoomDotsImpl() noexcept = default;

GoomDotsFx::GoomDotsImpl::GoomDotsImpl(const std::shared_ptr<const PluginInfo>& info) noexcept
  : goomInfo(info),
    pointWidth{(goomInfo->getScreenInfo().width * 2) / 5},
    pointHeight{(goomInfo->getScreenInfo().height * 2) / 5},
    pointWidthDiv2{static_cast<float>(pointWidth / 2.0F)},
    pointHeightDiv2{static_cast<float>(pointHeight / 2.0F)},
    pointWidthDiv3{static_cast<float>(pointWidth / 3.0F)},
    pointHeightDiv3{static_cast<float>(pointHeight / 3.0F)},
    draw{goomInfo->getScreenInfo().width, goomInfo->getScreenInfo().height},
    colorMaps{Weights<ColorMapGroup>{{
        {ColorMapGroup::perceptuallyUniformSequential, 10},
        {ColorMapGroup::sequential, 20},
        {ColorMapGroup::sequential2, 20},
        {ColorMapGroup::cyclic, 0},
        {ColorMapGroup::diverging, 0},
        {ColorMapGroup::diverging_black, 0},
        {ColorMapGroup::qualitative, 0},
        {ColorMapGroup::misc, 0},
    }}}
{
  changeColors();
}

void GoomDotsFx::GoomDotsImpl::setBuffSettings(const FXBuffSettings& settings)
{
  buffSettings = settings;
  draw.setBuffIntensity(buffSettings.buffIntensity);
  draw.setAllowOverexposed(buffSettings.allowOverexposed);
}

void GoomDotsFx::GoomDotsImpl::changeColors()
{
  colorMap1 = &colorMaps.getRandomColorMap();
  colorMap2 = &colorMaps.getRandomColorMap();
  colorMap3 = &colorMaps.getRandomColorMap();
  colorMap4 = &colorMaps.getRandomColorMap();
  colorMap5 = &colorMaps.getRandomColorMap();
  middleColor = ColorMap::getRandomColor(colorMaps.getRandomColorMap(ColorMapGroup::misc), 0.1, 1);

  useSingleBufferOnly = probabilityOfMInN(1, 2);
  useGrayScale = probabilityOfMInN(1, 10);
}

void GoomDotsFx::GoomDotsImpl::apply(PixelBuffer& prevBuff, PixelBuffer& currentBuff)
{
  uint32_t radius = 3;
  if ((goomInfo->getSoundInfo().getTimeSinceLastGoom() == 0) || changeDotColorsEvent())
  {
    changeColors();
    radius = 5;
  }

  const float largeFactor = getLargeSoundFactor(goomInfo->getSoundInfo());
  const uint32_t speedvarMult80Plus15 = goomInfo->getSoundInfo().getSpeed() * 80 + 15;
  const uint32_t speedvarMult50Plus1 = goomInfo->getSoundInfo().getSpeed() * 50 + 1;

  const float pointWidthDiv2MultLarge = pointWidthDiv2 * largeFactor;
  const float pointHeightDiv2MultLarge = pointHeightDiv2 * largeFactor;
  const float pointWidthDiv3MultLarge = (pointWidthDiv3 + 5.0f) * largeFactor;
  const float pointHeightDiv3MultLarge = (pointHeightDiv3 + 5.0f) * largeFactor;
  const float pointWidthMultLarge = pointWidth * largeFactor;
  const float pointHeightMultLarge = pointHeight * largeFactor;

  const float color1_t1 = (pointWidth - 6.0f) * largeFactor + 5.0f;
  const float color1_t2 = (pointHeight - 6.0f) * largeFactor + 5.0f;
  const float color4_t1 = pointHeightDiv3 * largeFactor + 20.0f;
  const float color4_t2 = color4_t1;

  const size_t numColors = radius;

  const size_t speedvarMult80Plus15Div15 = speedvarMult80Plus15 / 15;
  constexpr float t_min = 0.5;
  constexpr float t_max = 1.0;
  const float t_step = (t_max - t_min) / static_cast<float>(speedvarMult80Plus15Div15);

  float t = t_min;
  for (uint32_t i = 1; i <= speedvarMult80Plus15Div15; i++)
  {
    loopvar += speedvarMult50Plus1;

    const uint32_t loopvar_div_i = loopvar / i;
    const float i_mult_10 = 10.0f * i;

    const std::vector<Pixel> colors1 = getColors(middleColor, colorMap1->getColor(t), t, numColors);
    const float color1_t3 = i * 152.0f;
    const float color1_t4 = 128.0f;
    const uint32_t color1_cycle = loopvar + i * 2032;

    const std::vector<Pixel> colors2 = getColors(middleColor, colorMap2->getColor(t), t, numColors);
    const float color2_t1 = pointWidthDiv2MultLarge / i + i_mult_10;
    const float color2_t2 = pointHeightDiv2MultLarge / i + i_mult_10;
    const float color2_t3 = 96.0f;
    const float color2_t4 = i * 80.0f;
    const uint32_t color2_cycle = loopvar_div_i;

    const std::vector<Pixel> colors3 = getColors(middleColor, colorMap3->getColor(t), t, numColors);
    const float color3_t1 = pointWidthDiv3MultLarge / i + i_mult_10;
    const float color3_t2 = pointHeightDiv3MultLarge / i + i_mult_10;
    const float color3_t3 = i + 122.0f;
    const float color3_t4 = 134.0f;
    const uint32_t color3_cycle = loopvar_div_i;

    const std::vector<Pixel> colors4 = getColors(middleColor, colorMap4->getColor(t), t, numColors);
    const float color4_t3 = 58.0f;
    const float color4_t4 = i * 66.0f;
    const uint32_t color4_cycle = loopvar_div_i;

    const std::vector<Pixel> colors5 = getColors(middleColor, colorMap5->getColor(t), t, numColors);
    const float color5_t1 = (pointWidthMultLarge + i_mult_10) / i;
    const float color5_t2 = (pointHeightMultLarge + i_mult_10) / i;
    const float color5_t3 = 66.0f;
    const float color5_t4 = 74.0f;
    const uint32_t color5_cycle = loopvar + i * 500;

    dotFilter(prevBuff, currentBuff, colors1, color1_t1, color1_t2, color1_t3, color1_t4, t,
              color1_cycle, radius);
    dotFilter(prevBuff, currentBuff, colors2, color2_t1, color2_t2, color2_t3, color2_t4, t,
              color2_cycle, radius);
    dotFilter(prevBuff, currentBuff, colors3, color3_t1, color3_t2, color3_t3, color3_t4, t,
              color3_cycle, radius);
    dotFilter(prevBuff, currentBuff, colors4, color4_t1, color4_t2, color4_t3, color4_t4, t,
              color4_cycle, radius);
    dotFilter(prevBuff, currentBuff, colors5, color5_t1, color5_t2, color5_t3, color5_t4, t,
              color5_cycle, radius);

    t += t_step;
  }
}

std::vector<Pixel> GoomDotsFx::GoomDotsImpl::getColors(const Pixel& color0,
                                                       const Pixel& color1,
                                                       const float brightness,
                                                       const size_t numPts)
{
  std::vector<Pixel> colors(numPts);
  constexpr float t_min = 0.0;
  constexpr float t_max = 1.0;
  const float t_step = (t_max - t_min) / static_cast<float>(numPts);
  float t = t_min;
  for (size_t i = 0; i < numPts; i++)
  {
    if (!useGrayScale)
    {
      colors[i] = ColorMap::colorMix(color0, color1, t);
    }
    else
    {
      colors[i] = Pixel{.channels{.r = static_cast<uint8_t>(t * channel_limits<uint32_t>::max()),
                                  .g = static_cast<uint8_t>(t * channel_limits<uint32_t>::max()),
                                  .b = static_cast<uint8_t>(t * channel_limits<uint32_t>::max()),
                                  .a = 0xff}};
    }
    colors[i] = gammaCorrect.getCorrection(brightness, colors[i]);
    t += t_step;
  }
  return colors;
}

float GoomDotsFx::GoomDotsImpl::getLargeSoundFactor(const SoundInfo& soundInfo) const
{
  float largefactor = soundInfo.getSpeed() / 150.0f + soundInfo.getVolume() / 1.5f;
  if (largefactor > 1.5f)
  {
    largefactor = 1.5f;
  }
  return largefactor;
}

void GoomDotsFx::GoomDotsImpl::dotFilter(PixelBuffer& prevBuff,
                                         PixelBuffer& currentBuff,
                                         const std::vector<Pixel>& colors,
                                         const float t1,
                                         const float t2,
                                         const float t3,
                                         const float t4,
                                         const float brightness,
                                         const uint32_t cycle,
                                         const uint32_t radius)
{
  const auto xOffset = static_cast<uint32_t>(t1 * std::cos(static_cast<float>(cycle) / t3));
  const auto yOffset = static_cast<uint32_t>(t2 * std::sin(static_cast<float>(cycle) / t4));
  const auto x0 = static_cast<int>(goomInfo->getScreenInfo().width / 2 + xOffset);
  const auto y0 = static_cast<int>(goomInfo->getScreenInfo().height / 2 + yOffset);

  const uint32_t diameter = 2 * radius;
  const auto screenWidthLessDiameter = static_cast<int>(goomInfo->getScreenInfo().width - diameter);
  const auto screenHeightLessDiameter =
      static_cast<int>(goomInfo->getScreenInfo().height - diameter);

  if ((x0 < static_cast<int>(diameter)) || (y0 < static_cast<int>(diameter)) ||
      (x0 >= screenWidthLessDiameter) || (y0 >= screenHeightLessDiameter))
  {
    return;
  }

  const auto xmid = x0 + static_cast<int>(radius);
  const auto ymid = y0 + static_cast<int>(radius);
  if (useSingleBufferOnly)
  {
    draw.filledCircle(currentBuff, xmid, ymid, static_cast<int>(radius), colors);
    draw.setPixelRGB(currentBuff, static_cast<uint32_t>(xmid), static_cast<uint32_t>(ymid),
                     middleColor);
  }
  else
  {
    std::vector<PixelBuffer*> buffs{&currentBuff, &prevBuff};
    std::vector<Pixel> prevBuffColors(colors.size());
    for (size_t i = 0; i < colors.size(); i++)
    {
      prevBuffColors[i] =
          gammaCorrect.getCorrection(brightness, getColorAverage(middleColor, colors[i]));
    }
    const std::vector<std::vector<Pixel>> colorSets{colors, prevBuffColors};
    draw.filledCircle(buffs, xmid, ymid, static_cast<int>(radius), colorSets);
    const std::vector<Pixel> centreColors{middleColor,
                                          gammaCorrect.getCorrection(brightness, middleColor)};
    draw.setPixelRGB(buffs, static_cast<uint32_t>(xmid), static_cast<uint32_t>(ymid), centreColors);
  }
}

} // namespace goom
