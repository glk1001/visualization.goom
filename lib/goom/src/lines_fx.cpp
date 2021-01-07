#include "lines_fx.h"

#include "goom_config.h"
#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goomutils/colormap.h"
#include "goomutils/colorutils.h"
#include "goomutils/goomrand.h"
#include "goomutils/mathutils.h"

#undef NDEBUG
#include <cassert>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace goom
{

using namespace goom::utils;

class LinesFx::LinesImpl
{
public:
  LinesImpl() noexcept;
  // construit un effet de line (une ligne horitontale pour commencer)
  LinesImpl(std::shared_ptr<const PluginInfo> goomInfo,
            LineType srceLineType,
            float srceParam,
            const Pixel& srceColor,
            LineType destLineType,
            float destParam,
            const Pixel& destColor) noexcept;
  LinesImpl(const LinesImpl&) = delete;
  LinesImpl& operator=(const LinesImpl&) = delete;

  Pixel getRandomLineColor();

  [[nodiscard]] float getPower() const;
  void setPower(float val);

  void switchLines(LineType newLineType, float newParam, float newAmplitude, const Pixel& newColor);

  void drawLines(const std::vector<int16_t>& soundData,
                 PixelBuffer& prevBuff,
                 PixelBuffer& currentBuff);

  bool operator==(const LinesImpl&) const;

private:
  std::shared_ptr<const PluginInfo> goomInfo{};
  GoomDraw draw{};
  utils::ColorMaps colorMaps{};

  struct LinePoint
  {
    float x;
    float y;
    float angle;
  };
  std::vector<LinePoint> points1{};
  std::vector<LinePoint> points2{};
  void generateLine(LineType, float lineParam, std::vector<LinePoint>&);

  float power = 0;
  float powinc = 0;

  LineType destLineType = LineType::circle;
  float param = 0;
  float amplitudeF = 1;
  float amplitude = 1;

  // pour l'instant je stocke la couleur a terme, on stockera le mode couleur et l'on animera
  Pixel color{};
  Pixel color2{};

  void goomLinesMove();

  friend class cereal::access;
  template<class Archive>
  void save(Archive&) const;
  template<class Archive>
  void load(Archive&);
};

LinesFx::LinesFx() noexcept : fxImpl{new LinesImpl{}}
{
}

LinesFx::LinesFx(const std::shared_ptr<const PluginInfo>& info,
                 const LineType srceLineType,
                 const float srceParam,
                 const Pixel& srceColor,
                 const LineType destLineType,
                 const float destParam,
                 const Pixel& destColor) noexcept
  : fxImpl{
        new LinesImpl{info, srceLineType, srceParam, srceColor, destLineType, destParam, destColor}}
{
}

LinesFx::~LinesFx() noexcept = default;

bool LinesFx::operator==(const LinesFx& l) const
{
  return fxImpl->operator==(*l.fxImpl);
}

Pixel LinesFx::getRandomLineColor()
{
  return fxImpl->getRandomLineColor();
}

float LinesFx::getPower() const
{
  return fxImpl->getPower();
}

void LinesFx::setPower(const float val)
{
  fxImpl->setPower(val);
}

void LinesFx::switchLines(LineType newLineType,
                          const float newParam,
                          const float newAmplitude,
                          const Pixel& newColor)
{
  fxImpl->switchLines(newLineType, newParam, newAmplitude, newColor);
}

void LinesFx::drawLines(const std::vector<int16_t>& soundData,
                        PixelBuffer& prevBuff,
                        PixelBuffer& currentBuff)
{
  fxImpl->drawLines(soundData, prevBuff, currentBuff);
}

template<class Archive>
void LinesFx::serialize(Archive& ar)
{
  ar(CEREAL_NVP(enabled), CEREAL_NVP(fxImpl));
}

// Need to explicitly instantiate template functions for serialization.
template void LinesFx::serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);
template void LinesFx::serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template void LinesFx::LinesImpl::save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
template void LinesFx::LinesImpl::load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template<class Archive>
void LinesFx::LinesImpl::save(Archive& ar) const
{
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(draw), CEREAL_NVP(power), CEREAL_NVP(powinc),
     CEREAL_NVP(destLineType), CEREAL_NVP(param), CEREAL_NVP(amplitudeF), CEREAL_NVP(amplitude),
     CEREAL_NVP(color), CEREAL_NVP(color2));
}

template<class Archive>
void LinesFx::LinesImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(goomInfo), CEREAL_NVP(draw), CEREAL_NVP(power), CEREAL_NVP(powinc),
     CEREAL_NVP(destLineType), CEREAL_NVP(param), CEREAL_NVP(amplitudeF), CEREAL_NVP(amplitude),
     CEREAL_NVP(color), CEREAL_NVP(color2));
}

bool LinesFx::LinesImpl::operator==(const LinesImpl& l) const
{
  if (goomInfo == nullptr && l.goomInfo != nullptr)
  {
    return false;
  }
  if (goomInfo != nullptr && l.goomInfo == nullptr)
  {
    return false;
  }

  return ((goomInfo == nullptr && l.goomInfo == nullptr) || (*goomInfo == *l.goomInfo)) &&
         draw == l.draw && power == l.power && powinc == l.powinc &&
         destLineType == l.destLineType && param == l.param && amplitudeF == l.amplitudeF &&
         amplitude == l.amplitude && color == l.color && color2 == l.color2;
}

LinesFx::LinesImpl::LinesImpl() noexcept = default;

LinesFx::LinesImpl::LinesImpl(std::shared_ptr<const PluginInfo> info,
                              const LineType srceLineType,
                              const float srceParam,
                              const Pixel& srceColor,
                              const LineType destLineTyp,
                              const float destParam,
                              const Pixel& destColor) noexcept
  : goomInfo{std::move(info)},
    draw{goomInfo->getScreenInfo().width, goomInfo->getScreenInfo().height},
    points1(AUDIO_SAMPLE_LEN),
    points2(AUDIO_SAMPLE_LEN),
    destLineType{destLineTyp},
    param{destParam},
    color{srceColor},
    color2{destColor}
{
  generateLine(srceLineType, srceParam, points1);
  generateLine(destLineType, destParam, points2);

  switchLines(destLineType, destParam, 1.0, destColor);
}

void LinesFx::LinesImpl::generateLine(const LineType lineType,
                                      const float lineParam,
                                      std::vector<LinePoint>& line)
{
  switch (lineType)
  {
    case LineType::hline:
    {
      const float xStep = static_cast<float>(goomInfo->getScreenInfo().width - 1) /
                          static_cast<float>(AUDIO_SAMPLE_LEN - 1);
      float x = 0;
      for (auto& l : line)
      {
        l.angle = m_half_pi;
        l.x = x;
        l.y = lineParam;

        x += xStep;
      }
      return;
    }
    case LineType::vline:
    {
      const float yStep = static_cast<float>(goomInfo->getScreenInfo().height - 1) /
                          static_cast<float>(AUDIO_SAMPLE_LEN - 1);
      float y = 0;
      for (auto& l : line)
      {
        l.angle = 0;
        l.x = lineParam;
        l.y = y;

        y += yStep;
      }
      return;
    }
    case LineType::circle:
    {
      const float cx = 0.5F * static_cast<float>(goomInfo->getScreenInfo().width);
      const float cy = 0.5F * static_cast<float>(goomInfo->getScreenInfo().height);
      // Make sure the circle joins at each end - use AUDIO_SAMPLE_LEN - 1
      const float angleStep = m_two_pi / static_cast<float>(AUDIO_SAMPLE_LEN - 1);
      float angle = 0;
      for (auto& l : line)
      {
        l.angle = angle;
        l.x = cx + lineParam * std::cos(angle);
        l.y = cy + lineParam * std::sin(angle);

        angle += angleStep;
      }
      return;
    }
    default:
      throw std::logic_error("Unknown LineType enum.");
  }
}

inline float LinesFx::LinesImpl::getPower() const
{
  return power;
}

inline void LinesFx::LinesImpl::setPower(const float val)
{
  power = val;
}

void LinesFx::LinesImpl::goomLinesMove()
{
  for (uint32_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
  {
    points1[i].x = (points2[i].x + 39.0F * points1[i].x) / 40.0F;
    points1[i].y = (points2[i].y + 39.0F * points1[i].y) / 40.0F;
    points1[i].angle = (points2[i].angle + 39.0F * points1[i].angle) / 40.0F;
  }

  auto* c1 = reinterpret_cast<unsigned char*>(&color);
  auto* c2 = reinterpret_cast<unsigned char*>(&color2);
  for (int i = 0; i < 4; i++)
  {
    int cc1 = *c1;
    int cc2 = *c2;
    *c1 = static_cast<unsigned char>((cc1 * 63 + cc2) >> 6);
    ++c1;
    ++c2;
  }

  power += powinc;
  if (power < 1.1F)
  {
    power = 1.1F;
    powinc = static_cast<float>(getNRand(20) + 10) / 300.0F;
  }
  if (power > 17.5F)
  {
    power = 17.5F;
    powinc = -static_cast<float>(getNRand(20) + 10) / 300.0F;
  }

  amplitude = (99.0F * amplitude + amplitudeF) / 100.0F;
}

void LinesFx::LinesImpl::switchLines(LineType newLineType,
                                     float newParam,
                                     float newAmplitude,
                                     const Pixel& newColor)
{
  generateLine(newLineType, param, points2);
  destLineType = newLineType;
  param = newParam;
  amplitudeF = newAmplitude;
  color2 = newColor;
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

Pixel LinesFx::LinesImpl::getRandomLineColor()
{
  if (getNRand(10) == 0)
  {
    return getcouleur(static_cast<int>(getNRand(6)));
  }
  return ColorMap::getRandomColor(colorMaps.getRandomColorMap());
}

std::vector<float> simpleMovingAverage(const std::vector<int16_t>& x, const uint32_t winLen)
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

inline std::vector<float> getDataPoints(const std::vector<int16_t>& x)
{
  return std::vector<float>{x.data(), x.data() + AUDIO_SAMPLE_LEN};
  if (probabilityOfMInN(9999, 10000))
  {
    return std::vector<float>{x.data(), x.data() + AUDIO_SAMPLE_LEN};
  }

  return simpleMovingAverage(x, 3);
}

void LinesFx::LinesImpl::drawLines(const std::vector<int16_t>& soundData,
                                   PixelBuffer& prevBuff,
                                   PixelBuffer& currentBuff)
{
  std::vector<PixelBuffer*> buffs{&currentBuff, &prevBuff};
  const LinePoint* pt0 = &(points1[0]);
  const Pixel lineColor = getLightenedColor(color, power);

  const auto audioRange = static_cast<float>(goomInfo->getSoundInfo().getAllTimesMaxVolume() -
                                             goomInfo->getSoundInfo().getAllTimesMinVolume());
  assert(audioRange >= 0.0);

  if (audioRange < 0.0001)
  {
    // No range - flatline audio
    const std::vector<Pixel> colors = {lineColor, lineColor};
    draw.line(buffs, pt0->x, pt0->y, pt0->x + AUDIO_SAMPLE_LEN, pt0->y, colors, 1);
    goomLinesMove();
    return;
  }

  // This factor gives height to the audio samples lines. This value seems pleasing.
  constexpr float maxNormalizedPeak = 120;
  const auto getNormalizedData = [&](const float data) {
    return maxNormalizedPeak *
           (data - static_cast<float>(goomInfo->getSoundInfo().getAllTimesMinVolume())) /
           audioRange;
  };

  const Pixel randColor = getRandomLineColor();

  const auto getNextPoint = [&](const LinePoint* pt, const float dataVal) {
    assert(goomInfo->getSoundInfo().getAllTimesMinVolume() <= dataVal);
    const float cosAngle = std::cos(pt->angle);
    const float sinAngle = std::sin(pt->angle);
    const float normalizedDataVal = getNormalizedData(dataVal);
    assert(normalizedDataVal >= 0.0);
    const auto x = static_cast<int>(pt->x + amplitude * cosAngle * normalizedDataVal);
    const auto y = static_cast<int>(pt->y + amplitude * sinAngle * normalizedDataVal);
    const float maxBrightness =
        getRandInRange(1.0F, 3.0F) * normalizedDataVal / static_cast<float>(maxNormalizedPeak);
    const float t = std::min(1.0F, maxBrightness);
    static GammaCorrection gammaCorrect{4.2, 0.1};
    const Pixel modColor =
        gammaCorrect.getCorrection(t, ColorMap::colorMix(lineColor, randColor, t));
    return std::make_tuple(x, y, modColor);
  };

  const std::vector<float> data = getDataPoints(soundData);
  auto [x1, y1, modColor] = getNextPoint(pt0, data[0]);
  constexpr uint8_t thickness = 1;

  for (size_t i = 1; i < AUDIO_SAMPLE_LEN; i++)
  {
    const LinePoint* const pt = &(points1[i]);
    const auto [x2, y2, modColor] = getNextPoint(pt, data[i]);

    const std::vector<Pixel> colors = {modColor, lineColor};
    draw.line(buffs, x1, y1, x2, y2, colors, thickness);

    x1 = x2;
    y1 = y2;
  }

  goomLinesMove();
}

} // namespace goom
