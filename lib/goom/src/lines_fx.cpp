#include "lines_fx.h"

#include "colorutils.h"
#include "goom_config.h"
#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goomutils/colormap.h"
#include "goomutils/goomrand.h"
#include "goomutils/mathutils.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace goom
{

using namespace goom::utils;

GMLine::GMLine(PluginInfo* info,
               const uint16_t rx,
               const uint16_t ry,
               const LineType IDsrc,
               const float paramS,
               const Pixel& srcColor,
               const LineType dest,
               const float paramD,
               const Pixel& destColor)
  : goomInfo{info},
    draw{goomInfo->screen.width, goomInfo->screen.height},
    screenX{rx},
    screenY{ry},
    IDdest{dest},
    param{paramD},
    color{srcColor},
    color2{destColor}
{
  generateLine(IDsrc, paramS, rx, ry, points);
  generateLine(IDdest, paramD, rx, ry, points2);

  switchGoomLines(IDdest, paramD, 1.0, destColor);
}

void GMLine::generateLine(
    const LineType id, const float param, const uint32_t rx, const uint32_t ry, GMUnitPointer* l)
{
  switch (id)
  {
    case LineType::hline:
      for (uint32_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
      {
        l[i].x = (static_cast<float>(i) * rx) / static_cast<float>(AUDIO_SAMPLE_LEN);
        l[i].y = param;
        l[i].angle = m_half_pi;
      }
      return;
    case LineType::vline:
      for (uint32_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
      {
        l[i].y = (static_cast<float>(i) * ry) / static_cast<float>(AUDIO_SAMPLE_LEN);
        l[i].x = param;
        l[i].angle = 0.0f;
      }
      return;
    case LineType::circle:
      for (uint32_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
      {
        l[i].angle = m_two_pi * static_cast<float>(i) / static_cast<float>(AUDIO_SAMPLE_LEN);
        const float cosa = param * cos(l[i].angle);
        const float sina = param * sin(l[i].angle);
        l[i].x = static_cast<float>(rx) / 2.0f + cosa;
        l[i].y = static_cast<float>(ry) / 2.0f + sina;
      }
      return;
    default:
      throw std::logic_error("Unknown LineType enum.");
  }
}

void GMLine::setResolution(const uint32_t rx, const uint32_t ry)
{
  screenX = rx;
  screenY = ry;
  generateLine(IDdest, param, rx, ry, points2);
}

void GMLine::goomLinesMove()
{
  for (uint32_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
  {
    points[i].x = (points2[i].x + 39.0f * points[i].x) / 40.0f;
    points[i].y = (points2[i].y + 39.0f * points[i].y) / 40.0f;
    points[i].angle = (points2[i].angle + 39.0f * points[i].angle) / 40.0f;
  }

  unsigned char* c1 = reinterpret_cast<unsigned char*>(&color);
  unsigned char* c2 = reinterpret_cast<unsigned char*>(&color2);
  for (int i = 0; i < 4; i++)
  {
    int cc1 = *c1;
    int cc2 = *c2;
    *c1 = static_cast<unsigned char>((cc1 * 63 + cc2) >> 6);
    ++c1;
    ++c2;
  }

  power += powinc;
  if (power < 1.1f)
  {
    power = 1.1f;
    powinc = static_cast<float>(getNRand(20) + 10) / 300.0f;
  }
  if (power > 17.5f)
  {
    power = 17.5f;
    powinc = -static_cast<float>(getNRand(20) + 10) / 300.0f;
  }

  amplitude = (99.0f * amplitude + amplitudeF) / 100.0f;
}

void GMLine::switchGoomLines(const LineType dest,
                             const float prm,
                             const float amp,
                             const Pixel& color)
{
  generateLine(dest, param, screenX, screenY, points2);
  IDdest = dest;
  param = prm;
  amplitudeF = amp;
  color2 = color;
}

// les modes couleur possible (si tu mets un autre c'est noir)
#define GML_BLEUBLANC 0
#define GML_RED 1
#define GML_ORANGE_V 2
#define GML_ORANGE_J 3
#define GML_VERT 4
#define GML_BLEU 5
#define GML_BLACK 6

inline Pixel getcouleur(const int mode)
{
  switch (mode)
  {
    case GML_RED:
      return getIntColor(230, 120, 18);
    case GML_ORANGE_J:
      return getIntColor(120, 252, 18);
    case GML_ORANGE_V:
      return getIntColor(160, 236, 40);
    case GML_BLEUBLANC:
      return getIntColor(40, 220, 140);
    case GML_VERT:
      return getIntColor(200, 80, 18);
    case GML_BLEU:
      return getIntColor(250, 30, 80);
    case GML_BLACK:
      return getIntColor(16, 16, 16);
    default:
      throw std::logic_error("Unknown line color.");
  }
}

Pixel getBlackLineColor()
{
  return getcouleur(GML_BLACK);
}

Pixel getGreenLineColor()
{
  return getcouleur(GML_VERT);
}

Pixel getRedLineColor()
{
  return getcouleur(GML_RED);
}

Pixel GMLine::getRandomLineColor()
{
  if (getNRand(10) == 0)
  {
    return getcouleur(static_cast<int>(getNRand(6)));
  }
  return ColorMap::getRandomColor(colorMaps.getRandomColorMap());
}

std::vector<float> simpleMovingAverage(const int16_t x[AUDIO_SAMPLE_LEN], const uint32_t winLen)
{
  int32_t temp = 0;
  for (size_t i = 0; i < winLen - 1; i++)
  {
    temp += x[i];
  }

  std::vector<float> result{};
  result.reserve(AUDIO_SAMPLE_LEN - winLen + 1);
  for (size_t i = 0; i < AUDIO_SAMPLE_LEN - winLen + 1; i++)
  {
    temp += x[winLen - 1 + i];
    result.push_back(static_cast<float>(temp) / winLen);
    temp -= x[i];
  }

  return result;
}

inline std::vector<float> getDataPoints(const int16_t x[AUDIO_SAMPLE_LEN])
{
  //  return std::vector<float>{x, x + AUDIO_SAMPLE_LEN};
  if (probabilityOfMInN(9999, 10000))
  {
    return std::vector<float>{x, x + AUDIO_SAMPLE_LEN};
  }

  return simpleMovingAverage(x, 5);
}

void GMLine::drawGoomLines(const int16_t audioData[AUDIO_SAMPLE_LEN],
                           Pixel* prevBuff,
                           Pixel* currentBuff)
{
  std::vector<Pixel*> buffs{currentBuff, prevBuff};
  const GMUnitPointer* pt = &(points[0]);
  const Pixel lineColor = getLightenedColor(color, power);

  const float audioRange = static_cast<float>(goomInfo->getSoundInfo().getAllTimesMaxVolume() -
                                              goomInfo->getSoundInfo().getAllTimesMinVolume());
  if (audioRange < 0.0001)
  {
    // No range - flatline audio
    const std::vector<Pixel> colors = {lineColor, lineColor};
    draw.line(buffs, pt->x, pt->y, pt->x + AUDIO_SAMPLE_LEN, pt->y, colors, 1);
    goomLinesMove();
    return;
  }

  // This factor gives height to the audio samples lines. This value seems pleasing.
  constexpr float maxNormalizedPeak = 120000;
  const auto getNormalizedData = [&](const float data) {
    return maxNormalizedPeak *
           (data - static_cast<float>(goomInfo->getSoundInfo().getAllTimesMinVolume())) /
           audioRange;
  };

  const Pixel randColor = getRandomLineColor();

  const auto getNextPoint = [&](const GMUnitPointer* pt, const float dataVal) {
    const float cosa = cos(pt->angle) / 1000.0f;
    const float sina = sin(pt->angle) / 1000.0f;
    const float fdata = getNormalizedData(dataVal);
    const int x = static_cast<int>(pt->x + cosa * amplitude * fdata);
    const int y = static_cast<int>(pt->y + sina * amplitude * fdata);
    const float t = std::max(0.05f, fdata / static_cast<float>(maxNormalizedPeak));
    const Pixel modColor = ColorMap::colorMix(lineColor, randColor, t);
    return std::make_tuple(x, y, modColor);
  };

  const std::vector<float> data = getDataPoints(audioData);
  auto [x1, y1, modColor] = getNextPoint(pt, data[0]);
  constexpr uint8_t thickness = 1;

  for (size_t i = 1; i < AUDIO_SAMPLE_LEN; i++)
  {
    const GMUnitPointer* const pt = &(points[i]);
    const auto [x2, y2, modColor] = getNextPoint(pt, data[i]);

    const std::vector<Pixel> colors = {modColor, lineColor};
    draw.line(buffs, x1, y1, x2, y2, colors, thickness);

    x1 = x2;
    y1 = y2;
  }

  goomLinesMove();
}

} // namespace goom
