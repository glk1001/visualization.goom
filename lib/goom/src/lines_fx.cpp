#include "lines_fx.h"

#include "goom_config.h"
#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goomutils/colormaps.h"
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

namespace GOOM
{

using namespace GOOM::UTILS;

class LinesFx::LinesImpl
{
public:
  LinesImpl() noexcept;
  ~LinesImpl() noexcept = default;
  // construit un effet de line (une ligne horitontale pour commencer)
  LinesImpl(std::shared_ptr<const PluginInfo> goomInfo,
            LineType srceLineType,
            float srceParam,
            const Pixel& srceColor,
            LineType destLineType,
            float destParam,
            const Pixel& destColor);
  LinesImpl(const LinesImpl&) noexcept = delete;
  LinesImpl(LinesImpl&&) noexcept = delete;
  auto operator=(const LinesImpl&) -> LinesImpl& = delete;
  auto operator=(LinesImpl&&) -> LinesImpl& = delete;

  auto GetRandomLineColor() -> Pixel;

  [[nodiscard]] auto GetPower() const -> float;
  void SetPower(float val);

  void SwitchLines(LineType newLineType, float newParam, float newAmplitude, const Pixel& newColor);

  void DrawLines(const std::vector<int16_t>& soundData,
                 PixelBuffer& prevBuff,
                 PixelBuffer& currentBuff);

  auto operator==(const LinesImpl& l) const -> bool;

private:
  std::shared_ptr<const PluginInfo> m_goomInfo{};
  GoomDraw m_draw{};
  UTILS::ColorMaps m_colorMaps{};

  struct LinePoint
  {
    float x;
    float y;
    float angle;
  };
  std::vector<LinePoint> m_points1{};
  std::vector<LinePoint> m_points2{};
  void GenerateLine(LineType lineType, float lineParam, std::vector<LinePoint>& line);

  float m_power = 0;
  float m_powinc = 0;

  LineType m_destLineType = LineType::circle;
  float m_param = 0;
  float m_amplitudeF = 1;
  float m_amplitude = 1;

  // pour l'instant je stocke la couleur a terme, on stockera le mode couleur et l'on animera
  Pixel m_color1{};
  Pixel m_color2{};

  void GoomLinesMove();

  friend class cereal::access;
  template<class Archive>
  void save(Archive& ar) const;
  template<class Archive>
  void load(Archive& ar);
};

LinesFx::LinesFx() noexcept : m_fxImpl{new LinesImpl{}}
{
}

LinesFx::LinesFx(const std::shared_ptr<const PluginInfo>& info,
                 const LineType srceLineType,
                 const float srceParam,
                 const Pixel& srceColor,
                 const LineType destLineType,
                 const float destParam,
                 const Pixel& destColor) noexcept
  : m_fxImpl{
        new LinesImpl{info, srceLineType, srceParam, srceColor, destLineType, destParam, destColor}}
{
}

LinesFx::~LinesFx() noexcept = default;

auto LinesFx::operator==(const LinesFx& l) const -> bool
{
  return m_fxImpl->operator==(*l.m_fxImpl);
}

auto LinesFx::GetRandomLineColor() -> Pixel
{
  return m_fxImpl->GetRandomLineColor();
}

auto LinesFx::GetPower() const -> float
{
  return m_fxImpl->GetPower();
}

void LinesFx::SetPower(const float val)
{
  m_fxImpl->SetPower(val);
}

void LinesFx::SwitchLines(LineType newLineType,
                          const float newParam,
                          const float newAmplitude,
                          const Pixel& newColor)
{
  m_fxImpl->SwitchLines(newLineType, newParam, newAmplitude, newColor);
}

void LinesFx::DrawLines(const std::vector<int16_t>& soundData,
                        PixelBuffer& prevBuff,
                        PixelBuffer& currentBuff)
{
  m_fxImpl->DrawLines(soundData, prevBuff, currentBuff);
}

template<class Archive>
void LinesFx::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_enabled), CEREAL_NVP(m_fxImpl));
}

// Need to explicitly instantiate template functions for serialization.
template void LinesFx::serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);
template void LinesFx::serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template void LinesFx::LinesImpl::save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
template void LinesFx::LinesImpl::load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template<class Archive>
void LinesFx::LinesImpl::save(Archive& ar) const
{
  ar(CEREAL_NVP(m_goomInfo), CEREAL_NVP(m_draw), CEREAL_NVP(m_power), CEREAL_NVP(m_powinc),
     CEREAL_NVP(m_destLineType), CEREAL_NVP(m_param), CEREAL_NVP(m_amplitudeF),
     CEREAL_NVP(m_amplitude), CEREAL_NVP(m_color1), CEREAL_NVP(m_color2));
}

template<class Archive>
void LinesFx::LinesImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(m_goomInfo), CEREAL_NVP(m_draw), CEREAL_NVP(m_power), CEREAL_NVP(m_powinc),
     CEREAL_NVP(m_destLineType), CEREAL_NVP(m_param), CEREAL_NVP(m_amplitudeF),
     CEREAL_NVP(m_amplitude), CEREAL_NVP(m_color1), CEREAL_NVP(m_color2));
}

auto LinesFx::LinesImpl::operator==(const LinesImpl& l) const -> bool
{
  if (m_goomInfo == nullptr && l.m_goomInfo != nullptr)
  {
    return false;
  }
  if (m_goomInfo != nullptr && l.m_goomInfo == nullptr)
  {
    return false;
  }

  return ((m_goomInfo == nullptr && l.m_goomInfo == nullptr) || (*m_goomInfo == *l.m_goomInfo)) &&
         m_draw == l.m_draw && m_power == l.m_power && m_powinc == l.m_powinc &&
         m_destLineType == l.m_destLineType && m_param == l.m_param &&
         m_amplitudeF == l.m_amplitudeF && m_amplitude == l.m_amplitude && m_color1 == l.m_color1 &&
         m_color2 == l.m_color2;
}

LinesFx::LinesImpl::LinesImpl() noexcept = default;

LinesFx::LinesImpl::LinesImpl(std::shared_ptr<const PluginInfo> info,
                              const LineType srceLineType,
                              const float srceParam,
                              const Pixel& srceColor,
                              const LineType destLineTyp,
                              const float destParam,
                              const Pixel& destColor)
  : m_goomInfo{std::move(info)},
    m_draw{m_goomInfo->GetScreenInfo().width, m_goomInfo->GetScreenInfo().height},
    m_points1(AUDIO_SAMPLE_LEN),
    m_points2(AUDIO_SAMPLE_LEN),
    m_destLineType{destLineTyp},
    m_param{destParam},
    m_color1{srceColor},
    m_color2{destColor}
{
  GenerateLine(srceLineType, srceParam, m_points1);
  GenerateLine(m_destLineType, destParam, m_points2);

  SwitchLines(m_destLineType, destParam, 1.0, destColor);
}

void LinesFx::LinesImpl::GenerateLine(const LineType lineType,
                                      const float lineParam,
                                      std::vector<LinePoint>& line)
{
  switch (lineType)
  {
    case LineType::hline:
    {
      const float xStep = static_cast<float>(m_goomInfo->GetScreenInfo().width - 1) /
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
      const float yStep = static_cast<float>(m_goomInfo->GetScreenInfo().height - 1) /
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
      const float cx = 0.5F * static_cast<float>(m_goomInfo->GetScreenInfo().width);
      const float cy = 0.5F * static_cast<float>(m_goomInfo->GetScreenInfo().height);
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

inline auto LinesFx::LinesImpl::GetPower() const -> float
{
  return m_power;
}

inline void LinesFx::LinesImpl::SetPower(const float val)
{
  m_power = val;
}

void LinesFx::LinesImpl::GoomLinesMove()
{
  for (uint32_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
  {
    m_points1[i].x = (m_points2[i].x + 39.0F * m_points1[i].x) / 40.0F;
    m_points1[i].y = (m_points2[i].y + 39.0F * m_points1[i].y) / 40.0F;
    m_points1[i].angle = (m_points2[i].angle + 39.0F * m_points1[i].angle) / 40.0F;
  }

  auto* c1 = reinterpret_cast<unsigned char*>(&m_color1);
  auto* c2 = reinterpret_cast<unsigned char*>(&m_color2);
  for (int i = 0; i < 4; i++)
  {
    int cc1 = *c1;
    int cc2 = *c2;
    *c1 = static_cast<unsigned char>((cc1 * 63 + cc2) >> 6);
    ++c1;
    ++c2;
  }

  m_power += m_powinc;
  if (m_power < 1.1F)
  {
    m_power = 1.1F;
    m_powinc = static_cast<float>(GetNRand(20) + 10) / 300.0F;
  }
  if (m_power > 17.5F)
  {
    m_power = 17.5F;
    m_powinc = -static_cast<float>(GetNRand(20) + 10) / 300.0F;
  }

  m_amplitude = (99.0F * m_amplitude + m_amplitudeF) / 100.0F;
}

void LinesFx::LinesImpl::SwitchLines(LineType newLineType,
                                     float newParam,
                                     float newAmplitude,
                                     const Pixel& newColor)
{
  GenerateLine(newLineType, m_param, m_points2);
  m_destLineType = newLineType;
  m_param = newParam;
  m_amplitudeF = newAmplitude;
  m_color2 = newColor;
}

// les modes couleur possible (si tu mets un autre c'est noir)
#define GML_BLEUBLANC 0
#define GML_RED 1
#define GML_ORANGE_V 2
#define GML_ORANGE_J 3
#define GML_VERT 4
#define GML_BLEU 5
#define GML_BLACK 6

inline auto GetColor(const int mode) -> Pixel
{
  switch (mode)
  {
    case GML_RED:
      return GetIntColor(230, 120, 18);
    case GML_ORANGE_J:
      return GetIntColor(120, 252, 18);
    case GML_ORANGE_V:
      return GetIntColor(160, 236, 40);
    case GML_BLEUBLANC:
      return GetIntColor(40, 220, 140);
    case GML_VERT:
      return GetIntColor(200, 80, 18);
    case GML_BLEU:
      return GetIntColor(250, 30, 80);
    case GML_BLACK:
      return GetIntColor(16, 16, 16);
    default:
      throw std::logic_error("Unknown line color.");
  }
}

auto GetBlackLineColor() -> Pixel
{
  return GetColor(GML_BLACK);
}

auto GetGreenLineColor() -> Pixel
{
  return GetColor(GML_VERT);
}

auto GetRedLineColor() -> Pixel
{
  return GetColor(GML_RED);
}

auto LinesFx::LinesImpl::GetRandomLineColor() -> Pixel
{
  if (GetNRand(10) == 0)
  {
    return GetColor(static_cast<int>(GetNRand(6)));
  }
  return m_colorMaps.GetRandomColorMap().GetRandomColor(0.0F, 1.0F);
}

auto SimpleMovingAverage(const std::vector<int16_t>& x, const uint32_t winLen) -> std::vector<float>
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

inline auto GetDataPoints(const std::vector<int16_t>& x) -> std::vector<float>
{
  return std::vector<float>{x.data(), x.data() + AUDIO_SAMPLE_LEN};
  if (ProbabilityOfMInN(9999, 10000))
  {
    return std::vector<float>{x.data(), x.data() + AUDIO_SAMPLE_LEN};
  }

  return SimpleMovingAverage(x, 3);
}

void LinesFx::LinesImpl::DrawLines(const std::vector<int16_t>& soundData,
                                   PixelBuffer& prevBuff,
                                   PixelBuffer& currentBuff)
{
  std::vector<PixelBuffer*> buffs{&currentBuff, &prevBuff};
  const LinePoint* pt0 = &(m_points1[0]);
  const Pixel lineColor = GetLightenedColor(m_color1, m_power);

  const auto audioRange = static_cast<float>(m_goomInfo->GetSoundInfo().GetAllTimesMaxVolume() -
                                             m_goomInfo->GetSoundInfo().GetAllTimesMinVolume());
  assert(audioRange >= 0.0);

  if (audioRange < 0.0001)
  {
    // No range - flatline audio
    const std::vector<Pixel> colors = {lineColor, lineColor};
    m_draw.Line(buffs, pt0->x, pt0->y, pt0->x + AUDIO_SAMPLE_LEN, pt0->y, colors, 1);
    GoomLinesMove();
    return;
  }

  // This factor gives height to the audio samples lines. This value seems pleasing.
  constexpr float MAX_NORMALIZED_PEAK = 120;
  const auto getNormalizedData = [&](const float data) {
    return MAX_NORMALIZED_PEAK *
           (data - static_cast<float>(m_goomInfo->GetSoundInfo().GetAllTimesMinVolume())) /
           audioRange;
  };

  const Pixel randColor = GetRandomLineColor();

  const auto getNextPoint = [&](const LinePoint* pt, const float dataVal) {
    assert(m_goomInfo->GetSoundInfo().GetAllTimesMinVolume() <= dataVal);
    const float cosAngle = std::cos(pt->angle);
    const float sinAngle = std::sin(pt->angle);
    const float normalizedDataVal = getNormalizedData(dataVal);
    assert(normalizedDataVal >= 0.0);
    const auto x = static_cast<int>(pt->x + m_amplitude * cosAngle * normalizedDataVal);
    const auto y = static_cast<int>(pt->y + m_amplitude * sinAngle * normalizedDataVal);
    const float maxBrightness =
        GetRandInRange(1.0F, 3.0F) * normalizedDataVal / static_cast<float>(MAX_NORMALIZED_PEAK);
    const float t = std::min(1.0F, maxBrightness);
    static GammaCorrection s_gammaCorrect{4.2, 0.1};
    const Pixel modColor =
        s_gammaCorrect.GetCorrection(t, IColorMap::GetColorMix(lineColor, randColor, t));
    return std::make_tuple(x, y, modColor);
  };

  const std::vector<float> data = GetDataPoints(soundData);
  auto [x1, y1, modColor] = getNextPoint(pt0, data[0]);
  constexpr uint8_t THICKNESS = 1;

  for (size_t i = 1; i < AUDIO_SAMPLE_LEN; i++)
  {
    const LinePoint* const pt = &(m_points1[i]);
    const auto [x2, y2, modColor] = getNextPoint(pt, data[i]);

    const std::vector<Pixel> colors = {modColor, lineColor};
    m_draw.Line(buffs, x1, y1, x2, y2, colors, THICKNESS);

    x1 = x2;
    y1 = y2;
  }

  GoomLinesMove();
}

} // namespace GOOM
