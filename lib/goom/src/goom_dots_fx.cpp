#include "goom_dots_fx.h"

#include "colorutils.h"
#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"
#include "goomutils/logging_control.h"
#undef NO_LOGGING
#include "goomutils/logging.h"

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace goom
{

using namespace goom::utils;

inline bool changeDotColorsEvent()
{
  return probabilityOfMInN(1, 3);
}

GoomDots::GoomDots(PluginInfo* info)
  : goomInfo(info),
    screenWidth{goomInfo->screen.width},
    screenHeight{goomInfo->screen.height},
    pointWidth{(screenWidth * 2) / 5},
    pointHeight{(screenHeight * 2) / 5},
    pointWidthDiv2{static_cast<float>(pointWidth / 2.0F)},
    pointHeightDiv2{static_cast<float>(pointHeight / 2.0F)},
    pointWidthDiv3{static_cast<float>(pointWidth / 3.0F)},
    pointHeightDiv3{static_cast<float>(pointHeight / 3.0F)},
    draw{screenWidth, screenHeight},
    buffSettings{defaultFXBuffSettings},
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

void GoomDots::setBuffSettings(const FXBuffSettings& settings)
{
  buffSettings = settings;
  draw.setBuffIntensity(buffSettings.buffIntensity);
  draw.setAllowOverexposed(buffSettings.allowOverexposed);
}

void GoomDots::start()
{
}

void GoomDots::finish()
{
}

std::string GoomDots::getFxName() const
{
  return "goom dots";
}

void GoomDots::saveState(std::ostream&)
{
}

void GoomDots::loadState(std::istream&)
{
}

void GoomDots::changeColors()
{
  colorMap1 = &colorMaps.getRandomColorMap();
  colorMap2 = &colorMaps.getRandomColorMap();
  colorMap3 = &colorMaps.getRandomColorMap();
  colorMap4 = &colorMaps.getRandomColorMap();
  colorMap5 = &colorMaps.getRandomColorMap();
  middleColor = ColorMap::getRandomColor(colorMaps.getRandomColorMap(ColorMapGroup::misc), 0.1, 1);

  useSingleBufferOnly = probabilityOfMInN(1, 2);
}

std::vector<Pixel> GoomDots::getColors(const uint32_t color0,
                                       const uint32_t color1,
                                       const size_t numPts)
{
  std::vector<Pixel> colors(numPts);
  constexpr float t_min = 0.0;
  constexpr float t_max = 1.0;
  const float t_step = (t_max - t_min) / static_cast<float>(numPts);
  float t = t_min;
  for (size_t i = 0; i < numPts; i++)
  {
    colors[i] = Pixel{.val = ColorMap::colorMix(color0, color1, t)};
    t += t_step;
  }
  return colors;
}

void GoomDots::apply(Pixel* prevBuff, Pixel* currentBuff)
{
  uint32_t radius = 3;
  if ((goomInfo->sound->getTimeSinceLastGoom() == 0) || changeDotColorsEvent())
  {
    changeColors();
    radius = 5;
  }

  const float largeFactor = getLargeSoundFactor(*(goomInfo->sound));
  const uint32_t speedvarMult80Plus15 = goomInfo->sound->getSpeed() * 80 + 15;
  const uint32_t speedvarMult50Plus1 = goomInfo->sound->getSpeed() * 50 + 1;
  logDebug("speedvarMult80Plus15 = {}", speedvarMult80Plus15);
  logDebug("speedvarMult50Plus1 = {}", speedvarMult50Plus1);

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

  constexpr float t_min = 0.5;
  constexpr float t_max = 1.0;
  const float t_step = (t_max - t_min) / static_cast<float>(speedvarMult80Plus15);

  const size_t numColors = radius;

  logDebug("goomInfo->update.loopvar = {}", goomInfo->update.loopvar);
  float t = t_min;
  for (uint32_t i = 1; i * 15 <= speedvarMult80Plus15; i++)
  {
    goomInfo->update.loopvar += speedvarMult50Plus1;
    logDebug("goomInfo->update.loopvar = {}", goomInfo->update.loopvar);

    const uint32_t loopvar_div_i = goomInfo->update.loopvar / i;
    const float i_mult_10 = 10.0f * i;

    const std::vector<Pixel> colors1 = getColors(middleColor, colorMap1->getColor(t), numColors);
    const float color1_t3 = i * 152.0f;
    const float color1_t4 = 128.0f;
    const uint32_t color1_cycle = goomInfo->update.loopvar + i * 2032;

    const std::vector<Pixel> colors2 = getColors(middleColor, colorMap2->getColor(t), numColors);
    const float color2_t1 = pointWidthDiv2MultLarge / i + i_mult_10;
    const float color2_t2 = pointHeightDiv2MultLarge / i + i_mult_10;
    const float color2_t3 = 96.0f;
    const float color2_t4 = i * 80.0f;
    const uint32_t color2_cycle = loopvar_div_i;

    const std::vector<Pixel> colors3 = getColors(middleColor, colorMap3->getColor(t), numColors);
    const float color3_t1 = pointWidthDiv3MultLarge / i + i_mult_10;
    const float color3_t2 = pointHeightDiv3MultLarge / i + i_mult_10;
    const float color3_t3 = i + 122.0f;
    const float color3_t4 = 134.0f;
    const uint32_t color3_cycle = loopvar_div_i;

    const std::vector<Pixel> colors4 = getColors(middleColor, colorMap4->getColor(t), numColors);
    const float color4_t3 = 58.0f;
    const float color4_t4 = i * 66.0f;
    const uint32_t color4_cycle = loopvar_div_i;

    const std::vector<Pixel> colors5 = getColors(middleColor, colorMap5->getColor(t), numColors);
    const float color5_t1 = (pointWidthMultLarge + i_mult_10) / i;
    const float color5_t2 = (pointHeightMultLarge + i_mult_10) / i;
    const float color5_t3 = 66.0f;
    const float color5_t4 = 74.0f;
    const uint32_t color5_cycle = goomInfo->update.loopvar + i * 500;

    dotFilter(prevBuff, currentBuff, colors1, color1_t1, color1_t2, color1_t3, color1_t4,
              color1_cycle, radius);
    dotFilter(prevBuff, currentBuff, colors2, color2_t1, color2_t2, color2_t3, color2_t4,
              color2_cycle, radius);
    dotFilter(prevBuff, currentBuff, colors3, color3_t1, color3_t2, color3_t3, color3_t4,
              color3_cycle, radius);
    dotFilter(prevBuff, currentBuff, colors4, color4_t1, color4_t2, color4_t3, color4_t4,
              color4_cycle, radius);
    dotFilter(prevBuff, currentBuff, colors5, color5_t1, color5_t2, color5_t3, color5_t4,
              color5_cycle, radius);

    t += t_step;
  }
}

float GoomDots::getLargeSoundFactor(const SoundInfo& soundInfo) const
{
  float largefactor = soundInfo.getSpeed() / 150.0f + soundInfo.getVolume() / 1.5f;
  if (largefactor > 1.5f)
  {
    largefactor = 1.5f;
  }
  return largefactor;
}

void GoomDots::dotFilter(Pixel* prevBuff,
                         Pixel* currentBuff,
                         const std::vector<Pixel>& colors,
                         const float t1,
                         const float t2,
                         const float t3,
                         const float t4,
                         const uint32_t cycle,
                         const uint32_t radius)
{
  const uint32_t xOffset = static_cast<uint32_t>(t1 * cos(static_cast<float>(cycle) / t3));
  const uint32_t yOffset = static_cast<uint32_t>(t2 * sin(static_cast<float>(cycle) / t4));
  const int x0 = static_cast<int>(screenWidth / 2 + xOffset);
  const int y0 = static_cast<int>(screenHeight / 2 + yOffset);

  const uint32_t diameter = 2 * radius;
  const int screenWidthLessDiameter = static_cast<int>(screenWidth - diameter);
  const int screenHeightLessDiameter = static_cast<int>(screenHeight - diameter);

  if ((x0 < static_cast<int>(diameter)) || (y0 < static_cast<int>(diameter)) ||
      (x0 >= screenWidthLessDiameter) || (y0 >= screenHeightLessDiameter))
  {
    return;
  }

  const int xmid = x0 + static_cast<int>(radius);
  const int ymid = y0 + static_cast<int>(radius);
  if (useSingleBufferOnly)
  {
    draw.filledCircle(currentBuff, xmid, ymid, static_cast<int>(radius), colors);
    draw.setPixelRGB(currentBuff, static_cast<uint32_t>(xmid), static_cast<uint32_t>(ymid),
                     Pixel{.val = middleColor});
  }
  else
  {
    std::vector<Pixel*> buffs{currentBuff, prevBuff};
    std::vector<Pixel> prevBuffColors(colors.size());
    for (size_t i = 0; i < colors.size(); i++)
    {
      prevBuffColors[i] = Pixel{.val = ColorMap::colorMix(middleColor, colors[i].val, 0.5)};
    }
    const std::vector<std::vector<Pixel>> colorSets{colors, prevBuffColors};
    draw.filledCircle(buffs, xmid, ymid, static_cast<int>(radius), colorSets);
    const std::vector<Pixel> centreColors{
        Pixel{.val = middleColor},
        Pixel{.val = getBrighterColor(0.5, middleColor, buffSettings.allowOverexposed)}};
    draw.setPixelRGB(buffs, static_cast<uint32_t>(xmid), static_cast<uint32_t>(ymid), centreColors);
  }
}

} // namespace goom
