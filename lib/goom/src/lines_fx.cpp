#include "lines_fx.h"

#include "goom_config.h"
#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goomutils/colormaps.h"
#include "goomutils/colorutils.h"
#include "goomutils/goomrand.h"
#include "goomutils/image_bitmaps.h"
#include "goomutils/mathutils.h"
#include "goomutils/random_colormaps.h"
#include "goomutils/small_image_bitmaps.h"

#undef NDEBUG
#include <cassert>
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

  [[nodiscard]] auto GetResourcesDirectory() const -> const std::string&;
  void SetResourcesDirectory(const std::string& dirName);

  void SetSmallImageBitmaps(const SmallImageBitmaps& smallBitmaps);

  void Start();

  auto GetRandomLineColor() -> Pixel;

  [[nodiscard]] auto GetPower() const -> float;
  void SetPower(float val);

  [[nodiscard]] auto CanResetDestLine() const -> bool;

  void ResetDestLine(LineType newLineType,
                     float newParam,
                     float newAmplitude,
                     const Pixel& newColor);

  void DrawLines(const std::vector<int16_t>& soundData,
                 PixelBuffer& prevBuff,
                 PixelBuffer& currentBuff);

  void Finish();

private:
  const std::shared_ptr<const PluginInfo> m_goomInfo;
  GoomDraw m_draw;
  RandomColorMaps m_colorMaps{};
  std::string m_resourcesDirectory{};

  struct LinePoint
  {
    float x;
    float y;
    float angle;
  };
  std::vector<LinePoint> m_srcePoints{};
  std::vector<LinePoint> m_srcePointsCopy{};
  std::vector<LinePoint> m_destPoints{};
  static constexpr float LINE_LERP_FINISHED_VAL = 1.1F;
  static constexpr float LINE_LERP_INC = 1.0F / static_cast<float>(MIN_LINE_DURATION - 1);
  static constexpr float DEFAULT_LINE_LERP_FACTOR = LINE_LERP_INC;
  float m_lineLerpFactor = DEFAULT_LINE_LERP_FACTOR;
  void GenerateLinePoints(LineType lineType, float lineParam, std::vector<LinePoint>& line);

  float m_power = 0;
  float m_powinc = 0;

  LineType m_destLineType = LineType::circle;
  float m_param = 0;
  float m_newAmplitude = 1;
  float m_amplitude = 1;
  static constexpr size_t MIN_DOT_SIZE = 3;
  static constexpr size_t MAX_DOT_SIZE = 7;
  static_assert(MAX_DOT_SIZE <= SmallImageBitmaps::MAX_IMAGE_SIZE, "Max dot size mismatch.");
  size_t m_currentDotSize = MIN_DOT_SIZE;
  static auto GetNextDotSize() -> size_t;
  const SmallImageBitmaps* m_smallBitmaps{};
  auto GetImageBitmap(size_t size) -> const ImageBitmap&;

  // pour l'instant je stocke la couleur a terme, on stockera le mode couleur et l'on animera
  Pixel m_color1{};
  Pixel m_color2{};

  void MoveSrceLineCloserToDest();
};

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

auto LinesFx::GetResourcesDirectory() const -> const std::string&
{
  return m_fxImpl->GetResourcesDirectory();
}

void LinesFx::SetResourcesDirectory(const std::string& dirName)
{
  m_fxImpl->SetResourcesDirectory(dirName);
}

void LinesFx::SetSmallImageBitmaps(const SmallImageBitmaps& smallBitmaps)
{
  m_fxImpl->SetSmallImageBitmaps(smallBitmaps);
}

void LinesFx::Start()
{
  m_fxImpl->Start();
}

void LinesFx::Finish()
{
  m_fxImpl->Finish();
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

auto LinesFx::CanResetDestLine() const -> bool
{
  return m_fxImpl->CanResetDestLine();
}

void LinesFx::ResetDestLine(const LineType newLineType,
                            const float newParam,
                            const float newAmplitude,
                            const Pixel& newColor)
{
  m_fxImpl->ResetDestLine(newLineType, newParam, newAmplitude, newColor);
}

void LinesFx::DrawLines(const std::vector<int16_t>& soundData,
                        PixelBuffer& prevBuff,
                        PixelBuffer& currentBuff)
{
  m_fxImpl->DrawLines(soundData, prevBuff, currentBuff);
}

LinesFx::LinesImpl::LinesImpl(std::shared_ptr<const PluginInfo> info,
                              const LineType srceLineType,
                              const float srceParam,
                              const Pixel& srceColor,
                              const LineType destLineType,
                              const float destParam,
                              const Pixel& destColor)
  : m_goomInfo{std::move(info)},
    m_draw{m_goomInfo->GetScreenInfo().width, m_goomInfo->GetScreenInfo().height},
    m_srcePoints(AUDIO_SAMPLE_LEN),
    m_srcePointsCopy(AUDIO_SAMPLE_LEN),
    m_destPoints(AUDIO_SAMPLE_LEN),
    m_destLineType{destLineType},
    m_param{destParam},
    m_color1{srceColor},
    m_color2{destColor}
{
  GenerateLinePoints(srceLineType, srceParam, m_srcePoints);
  m_srcePointsCopy = m_srcePoints;
  ResetDestLine(m_destLineType, destParam, 1.0, destColor);
}

void LinesFx::LinesImpl::Start()
{
}

void LinesFx::LinesImpl::Finish()
{
}

void LinesFx::LinesImpl::GenerateLinePoints(const LineType lineType,
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

void LinesFx::LinesImpl::MoveSrceLineCloserToDest()
{
  const float t = std::min(1.0F, m_lineLerpFactor);
  for (uint32_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
  {
    m_srcePoints[i].x = stdnew::lerp(m_srcePointsCopy[i].x, m_destPoints[i].x, t);
    m_srcePoints[i].y = stdnew::lerp(m_srcePointsCopy[i].y, m_destPoints[i].y, t);
    m_srcePoints[i].angle = stdnew::lerp(m_srcePointsCopy[i].angle, m_destPoints[i].angle, t);
  }
  m_lineLerpFactor += LINE_LERP_INC;

  m_color1 = IColorMap::GetColorMix(m_color1, m_color2, 1.0F / 64.0F);

  m_power += m_powinc;
  if (m_power < 1.1F)
  {
    m_power = 1.1F;
    m_powinc = GetRandInRange(0.03F, 0.10F);
  }
  if (m_power > 17.5F)
  {
    m_power = 17.5F;
    m_powinc = -GetRandInRange(0.03F, 0.10F);
  }

  m_amplitude = stdnew::lerp(m_amplitude, m_newAmplitude, 0.01F);
}

auto LinesFx::LinesImpl::CanResetDestLine() const -> bool
{
  return m_lineLerpFactor > LINE_LERP_FINISHED_VAL;
}

void LinesFx::LinesImpl::ResetDestLine(LineType newLineType,
                                       float newParam,
                                       float newAmplitude,
                                       const Pixel& newColor)
{
  GenerateLinePoints(newLineType, m_param, m_destPoints);
  m_destLineType = newLineType;
  m_param = newParam;
  m_newAmplitude = newAmplitude;
  m_color2 = newColor;
  m_lineLerpFactor = DEFAULT_LINE_LERP_FACTOR;
  m_srcePointsCopy = m_srcePoints;
}

inline auto LinesFx::LinesImpl::GetResourcesDirectory() const -> const std::string&
{
  return m_resourcesDirectory;
}

inline void LinesFx::LinesImpl::SetResourcesDirectory(const std::string& dirName)
{
  m_resourcesDirectory = dirName;
}

inline void LinesFx::LinesImpl::SetSmallImageBitmaps(const SmallImageBitmaps& smallBitmaps)
{
  m_smallBitmaps = &smallBitmaps;
}

inline auto LinesFx::LinesImpl::GetImageBitmap(const size_t size) -> const ImageBitmap&
{
  return m_smallBitmaps->GetImageBitmap(
      SmallImageBitmaps::ImageNames::CIRCLE,
      stdnew::clamp(size % 2 != 0 ? size : size + 1, MIN_DOT_SIZE, MAX_DOT_SIZE));
}

inline auto LinesFx::LinesImpl::GetPower() const -> float
{
  return m_power;
}

inline void LinesFx::LinesImpl::SetPower(const float val)
{
  m_power = val;
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
  if (ProbabilityOfMInN(1, 10))
  {
    return GetColor(static_cast<int>(GetNRand(6)));
  }
  return RandomColorMaps::GetRandomColor(m_colorMaps.GetRandomColorMap(), 0.0F, 1.0F);
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
  const std::vector<PixelBuffer*> buffs{&currentBuff, &prevBuff};
  const LinePoint& pt0 = m_srcePoints[0];
  const Pixel lineColor = GetLightenedColor(m_color1, m_power);

  const auto audioRange = static_cast<float>(m_goomInfo->GetSoundInfo().GetAllTimesMaxVolume() -
                                             m_goomInfo->GetSoundInfo().GetAllTimesMinVolume());
  assert(audioRange >= 0.0);

  if (audioRange < 0.0001)
  {
    // No range - flatline audio
    const std::vector<Pixel> colors = {lineColor, lineColor};
    m_draw.Line(buffs, static_cast<int>(pt0.x), static_cast<int>(pt0.y),
                static_cast<int>(pt0.x) + AUDIO_SAMPLE_LEN, static_cast<int>(pt0.y), colors, 1);
    MoveSrceLineCloserToDest();
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

  const auto getNextPoint = [&](const LinePoint& pt, const float dataVal) {
    assert(m_goomInfo->GetSoundInfo().GetAllTimesMinVolume() <= dataVal);
    const float cosAngle = std::cos(pt.angle);
    const float sinAngle = std::sin(pt.angle);
    const float normalizedDataVal = getNormalizedData(dataVal);
    assert(normalizedDataVal >= 0.0);
    const auto x = static_cast<int>(pt.x + m_amplitude * cosAngle * normalizedDataVal);
    const auto y = static_cast<int>(pt.y + m_amplitude * sinAngle * normalizedDataVal);
    const float maxBrightness =
        GetRandInRange(1.0F, 3.0F) * normalizedDataVal / static_cast<float>(MAX_NORMALIZED_PEAK);
    const float t = std::min(1.0F, maxBrightness);
    static GammaCorrection s_gammaCorrect{4.2, 0.1};
    const Pixel modColor =
        s_gammaCorrect.GetCorrection(t, IColorMap::GetColorMix(lineColor, randColor, t));
    return std::make_tuple(x, y, modColor);
  };

  const std::vector<float> data = GetDataPoints(soundData);
#if __cplusplus <= 201402L
  const auto nextPoint = getNextPoint(pt0, data[0]);
  auto x1 = std::get<0>(nextPoint);
  auto y1 = std::get<1>(nextPoint);
#else
  auto [x1, y1, modColor] = getNextPoint(pt0, data[0]);
#endif
  constexpr uint8_t THICKNESS = 1;

  for (size_t i = 1; i < AUDIO_SAMPLE_LEN; i++)
  {
    const LinePoint& pt = m_srcePoints[i];
#if __cplusplus <= 201402L
    const auto nextPoint2 = getNextPoint(pt, data[i]);
    const auto x2 = std::get<0>(nextPoint2);
    const auto y2 = std::get<1>(nextPoint2);
    const auto modColor = std::get<2>(nextPoint2);
#else
    const auto [x2, y2, modColor] = getNextPoint(pt, data[i]);
#endif

    const std::vector<Pixel> colors = {modColor, lineColor};
    m_draw.Line(buffs, x1, y1, x2, y2, colors, THICKNESS);

    if (m_currentDotSize > 1)
    {
      const auto getModColor = [&]([[maybe_unused]] const size_t x, [[maybe_unused]] const size_t y,
                                   const Pixel& b) -> Pixel {
        return GetColorMultiply(b, colors[0], true);
      };
      const auto getLineColor = [&]([[maybe_unused]] const size_t x,
                                    [[maybe_unused]] const size_t y, const Pixel& b) -> Pixel {
        return GetColorMultiply(b, colors[1], true);
      };
      const std::vector<GoomDraw::GetBitmapColorFunc> getColors{getModColor, getLineColor};
      const ImageBitmap& bitmap = GetImageBitmap(m_currentDotSize);
      const std::vector<const PixelBuffer*> bitmaps{&bitmap, &bitmap};
      m_draw.Bitmap(buffs, x2, y2, bitmaps, getColors);
    }

    x1 = x2;
    y1 = y2;
  }

  MoveSrceLineCloserToDest();

  m_currentDotSize = GetNextDotSize();
}

inline auto LinesFx::LinesImpl::GetNextDotSize() -> size_t
{
  static const Weights<size_t> s_dotSizes{{
      {1, 100},
      {3, 10},
      {5, 5},
      {7, 1},
  }};

  return s_dotSizes.GetRandomWeighted();
}

} // namespace GOOM
