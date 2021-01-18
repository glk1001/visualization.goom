#include "tentacle_driver.h"

#include "goom_draw.h"
#include "goomutils/colormap.h"
#include "goomutils/colorutils.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
//#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "tentacles.h"
#include "v3d.h"

#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace GOOM
{

using namespace GOOM::UTILS;

inline auto ChangeCurrentColorMapEvent() -> bool
{
  return probabilityOfMInN(3, 5);
}

const size_t TentacleDriver::g_ChangeCurrentColorMapGroupEveryNUpdates = 400;
const size_t TentacleDriver::g_ChangeTentacleColorMapEveryNUpdates = 100;

TentacleDriver::TentacleDriver() noexcept = default;

TentacleDriver::TentacleDriver(const uint32_t screenW, const uint32_t screenH) noexcept
  : m_screenWidth{screenW}, m_screenHeight{screenH}, m_draw{m_screenWidth, m_screenHeight}
{
  const IterParamsGroup iter1 = {
      {100, 0.600, 1.0, {1.5, -10.0, +10.0, m_pi}, 100.0},
      {125, 0.700, 2.0, {1.0, -10.0, +10.0, 0.0}, 105.0},
  };
  const IterParamsGroup iter2 = {
      {125, 0.700, 0.5, {1.0, -10.0, +10.0, 0.0}, 100.0},
      {150, 0.800, 1.5, {1.5, -10.0, +10.0, m_pi}, 105.0},
  };
  const IterParamsGroup iter3 = {
      {150, 0.800, 1.5, {1.5, -10.0, +10.0, m_pi}, 100.0},
      {200, 0.900, 2.5, {1.0, -10.0, +10.0, 0.0}, 105.0},
  };
  /***
  const IterParamsGroup iter1 = {
      {100, 0.700, 1.0, {1.5, -10.0, +10.0, m_pi}, 100.0},
      {125, 0.800, 2.0, {1.0, -10.0, +10.0, 0.0}, 105.0},
  };
  const IterParamsGroup iter2 = {
      {125, 0.710, 0.5, {1.0, -10.0, +10.0, 0.0}, 100.0},
      {150, 0.810, 1.5, {1.5, -10.0, +10.0, m_pi}, 105.0},
  };
  const IterParamsGroup iter3 = {
      {150, 0.720, 1.5, {1.5, -10.0, +10.0, m_pi}, 100.0},
      {200, 0.820, 2.5, {1.0, -10.0, +10.0, 0.0}, 105.0},
  };
  ***/

  m_iterParamsGroups = {
      iter1,
      iter2,
      iter3,
  };

  logDebug("Constructed TentacleDriver.");
}

auto TentacleDriver::IterationParams::operator==(const IterationParams& p) const -> bool
{
  return numNodes == p.numNodes && prevYWeight == p.prevYWeight &&
         iterZeroYValWaveFreq == p.iterZeroYValWave && iterZeroYValWave == p.iterZeroYValWave &&
         length == p.length;
}

auto TentacleDriver::IterParamsGroup::operator==(const IterParamsGroup& p) const -> bool
{
  return first == p.first && last == p.last;
}

auto TentacleDriver::operator==(const TentacleDriver& t) const -> bool
{
  const bool result = m_colorMode == t.m_colorMode && m_iterParamsGroups == t.m_iterParamsGroups &&
                      m_screenWidth == t.m_screenWidth && m_screenHeight == t.m_screenHeight &&
                      m_draw == t.m_draw && m_buffSettings == t.m_buffSettings &&
                      m_updateNum == t.m_updateNum && m_tentacles == t.m_tentacles &&
                      m_numTentacles == t.m_numTentacles && m_tentacleParams == t.m_tentacleParams;

  if (result)
  {
    for (size_t i = 0; i < m_colorizers.size(); i++)
    {
      if (typeid(*m_colorizers[i]) != typeid(*t.m_colorizers[i]))
      {
        logInfo("TentacleDriver colorizers not of same type at index {}", i);
        return false;
      }
      const auto* c1 = dynamic_cast<const TentacleColorMapColorizer*>(m_colorizers[i].get());
      if (c1 == nullptr)
      {
        continue;
      }
      const auto* c2 = dynamic_cast<const TentacleColorMapColorizer*>(t.m_colorizers[i].get());
      if (c2 == nullptr)
      {
        logInfo("TentacleDriver colorizers not of same type at index {}", i);
        return false;
      }
      if (*c1 != *c2)
      {
        logInfo("TentacleDriver colorizers not the same at index {}", i);
        return false;
      }
    }
  }
  logInfo("colorizers == t.colorizers = true");

  logInfo("TentacleDriver result == {}", result);
  if (!result)
  {
    logInfo("TentacleDriver result == {}", result);
    logInfo("colorMode = {}, t.colorMode = {}", m_colorMode, t.m_colorMode);
    logInfo("iterParamsGroups == t.iterParamsGroups = {}",
            m_iterParamsGroups == t.m_iterParamsGroups);
    logInfo("screenWidth = {}, t.screenWidth = {}", m_screenWidth, t.m_screenWidth);
    logInfo("screenHeight = {}, t.screenHeight = {}", m_screenHeight, t.m_screenHeight);
    logInfo("draw == t.draw = {}", m_draw == t.m_draw);
    logInfo("buffSettings == t.buffSettings = {}", m_buffSettings == t.m_buffSettings);
    logInfo("updateNum = {}, t.updateNum = {}", m_updateNum, t.m_updateNum);
    logInfo("tentacles == t.tentacles = {}", m_tentacles == t.m_tentacles);
    logInfo("numTentacles = {}, t.numTentacles = {}", m_numTentacles, t.m_numTentacles);
    logInfo("tentacleParams == t.tentacleParams = {}", m_tentacleParams == t.m_tentacleParams);
  }

  return result;
}

auto TentacleDriver::GetBuffSettings() const -> const FXBuffSettings&
{
  return m_buffSettings;
}

void TentacleDriver::SetBuffSettings(const FXBuffSettings& settings)
{
  m_buffSettings = settings;
  m_draw.SetBuffIntensity(m_buffSettings.buffIntensity);
  m_draw.SetAllowOverexposed(m_buffSettings.allowOverexposed);
  m_tentacles.SetAllowOverexposed(m_buffSettings.allowOverexposed);
}

auto TentacleDriver::GetNumTentacles() const -> size_t
{
  return m_numTentacles;
}

auto TentacleDriver::GetColorMode() const -> TentacleDriver::ColorModes
{
  return m_colorMode;
}

void TentacleDriver::SetColorMode(ColorModes m)
{
  m_colorMode = m;
}

constexpr double TENT2D_XMIN = 0.0;
constexpr double TENT2D_YMIN = 0.065736;
constexpr double TENT2D_YMAX = 10000.0;

void TentacleDriver::Init(const ITentacleLayout& l)
{
  logDebug("Starting driver Init.");

  m_numTentacles = l.GetNumPoints();
  logDebug("numTentacles = {}.", m_numTentacles);

  m_tentacleParams.resize(m_numTentacles);

  constexpr V3d INITIAL_HEAD_POS = {0, 0, 0};

  const size_t numInParamGroup = m_numTentacles / m_iterParamsGroups.size();
  const float tStep = 1.0F / static_cast<float>(numInParamGroup - 1);
  logDebug("numInTentacleGroup = {}, tStep = {:.2f}.", numInParamGroup, tStep);

  const UTILS::ColorMapGroup initialColorMapGroup = m_colorMaps.GetRandomGroup();

  size_t paramsIndex = 0;
  float t = 0.0;
  for (size_t i = 0; i < m_numTentacles; i++)
  {
    const IterParamsGroup paramsGrp = m_iterParamsGroups.at(paramsIndex);
    if (i % numInParamGroup == 0)
    {
      if (paramsIndex < m_iterParamsGroups.size() - 1)
      {
        paramsIndex++;
      }
      t = 0.0;
    }
    const IterationParams params = paramsGrp.GetNext(t);
    t += tStep;
    m_tentacleParams[i] = params;

    std::shared_ptr<TentacleColorMapColorizer> colorizer{
        new TentacleColorMapColorizer{initialColorMapGroup, params.numNodes}};
    m_colorizers.push_back(std::move(colorizer));

    std::unique_ptr<Tentacle2D> tentacle2D{CreateNewTentacle2D(i, params)};
    logDebug("Created tentacle2D {}.", i);

    // To hide the annoying flapping tentacle head, make near the head very dark.
    const Pixel headColor = getIntColor(5, 5, 5);
    const Pixel headColorLow = headColor;
    Tentacle3D tentacle{std::move(tentacle2D),
                        m_colorizers[m_colorizers.size() - 1],
                        headColor,
                        headColorLow,
                        INITIAL_HEAD_POS,
                        Tentacle2D::MIN_NUM_NODES};

    m_tentacles.AddTentacle(std::move(tentacle));

    logDebug("Added tentacle {}.", i);
  }

  UpdateTentaclesLayout(l);

  m_updateNum = 0;
}

auto TentacleDriver::IterParamsGroup::GetNext(const float t) const
    -> TentacleDriver::IterationParams
{
  const float prevYWeight =
      getRandInRange(0.9F, 1.1F) *
      std::lerp(static_cast<float>(first.prevYWeight), static_cast<float>(last.prevYWeight), t);
  IterationParams params{};
  params.length =
      size_t(getRandInRange(1.0F, 1.1F) *
             std::lerp(static_cast<float>(first.length), static_cast<float>(last.length), t));
  params.numNodes =
      size_t(getRandInRange(0.9F, 1.1F) *
             std::lerp(static_cast<float>(first.numNodes), static_cast<float>(last.numNodes), t));
  params.prevYWeight = prevYWeight;
  params.iterZeroYValWave = first.iterZeroYValWave;
  params.iterZeroYValWaveFreq =
      getRandInRange(0.9F, 1.1F) * std::lerp(static_cast<float>(first.iterZeroYValWaveFreq),
                                             static_cast<float>(last.iterZeroYValWaveFreq), t);
  return params;
}

auto TentacleDriver::CreateNewTentacle2D(size_t id, const IterationParams& p)
    -> std::unique_ptr<Tentacle2D>
{
  logDebug("Creating new tentacle2D {}...", id);

  const size_t tentacleLen = getRandInRange(0.99F, 1.01F) * static_cast<float>(p.length);
  //const size_t tentacleLen = params.length;
  const double tent2d_xmax = TENT2D_XMIN + tentacleLen;

  std::unique_ptr<Tentacle2D> tentacle{new Tentacle2D{
      id, p.numNodes,
      //  size_t(static_cast<float>(params.numNodes) * getRandInRange(0.9f, 1.1f))
      TENT2D_XMIN, tent2d_xmax, TENT2D_YMIN, TENT2D_YMAX, p.prevYWeight, 1.0 - p.prevYWeight}};
  logDebug("Created new tentacle2D {}.", id);

  logDebug("tentacle {:3}:"
           " tentacleLen = {:4}, tent2d_xmin = {:7.2f}, tent2d_xmax = {:5.2f},"
           " prevYWeight = {:5.2f}, curYWeight = {:5.2f}, numNodes = {:5}",
           id, tentacleLen, tent2d_xmin, tent2d_xmax, tentacle->GetPrevYWeight(),
           tentacle->GetCurrentYWeight(), tentacle->GetNumNodes());

  tentacle->SetDoDamping(true);

  return tentacle;
}

void TentacleDriver::StartIterating()
{
  for (auto& t : m_tentacles)
  {
    t.Get2DTentacle().StartIterating();
  }
}

[[maybe_unused]] void TentacleDriver::StopIterating()
{
  for (auto& t : m_tentacles)
  {
    t.Get2DTentacle().FinishIterating();
  }
}

void TentacleDriver::UpdateTentaclesLayout(const ITentacleLayout& l)
{
  logDebug("Updating tentacles layout. numTentacles = {}.", m_numTentacles);
  assert(l.GetNumPoints() == m_numTentacles);

  std::vector<size_t> sortedLongestFirst(m_numTentacles);
  std::iota(sortedLongestFirst.begin(), sortedLongestFirst.end(), 0);
  const auto compareByLength = [this](const size_t id1, const size_t id2) -> bool {
    const double len1 = m_tentacles[id1].Get2DTentacle().GetLength();
    const double len2 = m_tentacles[id2].Get2DTentacle().GetLength();
    // Sort by longest first.
    return len1 > len2;
  };
  std::sort(sortedLongestFirst.begin(), sortedLongestFirst.end(), compareByLength);

  for (size_t i = 0; i < m_numTentacles; i++)
  {
    logDebug("{} {} tentacle[{}].len = {:.2}.", i, sortedLongestFirst.at(i),
             sortedLongestFirst.at(i),
             m_tentacles[sortedLongestFirst.at(i)].Get2DTentacle().GetLength());
  }

  for (size_t i = 0; i < m_numTentacles; i++)
  {
    m_tentacles[sortedLongestFirst.at(i)].SetHead(l.GetPoints().at(i));
  }

  // To help with perspective, any tentacles near vertical centre will be shortened.
  for (auto& tentacle : m_tentacles)
  {
    const V3d& head = tentacle.GetHead();
    if (std::fabs(head.x) < 10.0F)
    {
      Tentacle2D& tentacle2D = tentacle.Get2DTentacle();
      const double xmin = tentacle2D.GetXMin();
      const double xmax = tentacle2D.GetXMax();
      const double newXmax = xmin + 1.0 * (xmax - xmin);
      tentacle2D.SetXDimensions(xmin, newXmax);
      tentacle.SetNumHeadNodes(
          std::max(6 * Tentacle2D::MIN_NUM_NODES, tentacle.Get2DTentacle().GetNumNodes() / 2));
    }
  }
}

void TentacleDriver::MultiplyIterZeroYValWaveFreq(const float val)
{
  for (size_t i = 0; i < m_numTentacles; i++)
  {
    const float newFreq = val * m_tentacleParams[i].iterZeroYValWaveFreq;
    m_tentacleParams[i].iterZeroYValWave.setFrequency(newFreq);
  }
}

void TentacleDriver::SetReverseColorMix(const bool val)
{
  for (auto& t : m_tentacles)
  {
    t.SetReverseColorMix(val);
  }
}

void TentacleDriver::UpdateIterTimers()
{
  for (auto* t : m_iterTimers)
  {
    t->Next();
  }
}

auto TentacleDriver::GetNextColorMapGroups() const -> std::vector<UTILS::ColorMapGroup>
{
  const size_t numDifferentGroups =
      (m_colorMode == ColorModes::minimal || m_colorMode == ColorModes::oneGroupForAll ||
       probabilityOfMInN(99, 100))
          ? 1
          : getRandInRange(1U, std::min(size_t(5U), m_colorizers.size()));
  std::vector<UTILS::ColorMapGroup> groups(numDifferentGroups);
  for (size_t i = 0; i < numDifferentGroups; i++)
  {
    groups[i] = m_colorMaps.GetRandomGroup();
  }

  std::vector<UTILS::ColorMapGroup> nextColorMapGroups(m_colorizers.size());
  const size_t numPerGroup = nextColorMapGroups.size() / numDifferentGroups;
  size_t n = 0;
  for (size_t i = 0; i < nextColorMapGroups.size(); i++)
  {
    nextColorMapGroups[i] = groups[n];
    if ((i % numPerGroup == 0) && (n < numDifferentGroups - 1))
    {
      n++;
    }
  }

  if (probabilityOfMInN(1, 2))
  {
    std::random_shuffle(nextColorMapGroups.begin(), nextColorMapGroups.end());
  }

  return nextColorMapGroups;
}

void TentacleDriver::CheckForTimerEvents()
{
  //  logDebug("Update num = {}: checkForTimerEvents", updateNum);

  if (m_updateNum % g_ChangeCurrentColorMapGroupEveryNUpdates == 0)
  {
    const std::vector<UTILS::ColorMapGroup> nextGroups = GetNextColorMapGroups();
    for (size_t i = 0; i < m_colorizers.size(); i++)
    {
      m_colorizers[i]->SetColorMapGroup(nextGroups[i]);
    }
    if (m_colorMode != ColorModes::minimal)
    {
      for (auto& colorizer : m_colorizers)
      {
        if (ChangeCurrentColorMapEvent())
        {
          colorizer->ChangeColorMap();
        }
      }
    }
  }
}

void TentacleDriver::FreshStart()
{
  const UTILS::ColorMapGroup nextColorMapGroup = m_colorMaps.GetRandomGroup();
  for (auto& colorizer : m_colorizers)
  {
    colorizer->SetColorMapGroup(nextColorMapGroup);
    if (m_colorMode != ColorModes::minimal)
    {
      colorizer->ChangeColorMap();
    }
  }
}

void TentacleDriver::Update(float angle,
                            float distance,
                            float distance2,
                            const Pixel& color,
                            const Pixel& colorLow,
                            PixelBuffer& currentBuff,
                            PixelBuffer& nextBuff)
{
  m_updateNum++;
  logInfo("Doing Update {}.", m_updateNum);

  UpdateIterTimers();
  CheckForTimerEvents();

  constexpr float ITER_ZERO_LERP_FACTOR = 0.9;

  for (size_t i = 0; i < m_numTentacles; i++)
  {
    Tentacle3D& tentacle = m_tentacles[i];
    Tentacle2D& tentacle2D = tentacle.Get2DTentacle();

    const float iterZeroYVal = m_tentacleParams[i].iterZeroYValWave.getNext();
    tentacle2D.SetIterZeroLerpFactor(ITER_ZERO_LERP_FACTOR);
    tentacle2D.SetIterZeroYVal(iterZeroYVal);

    logDebug("Starting Iterate {} for tentacle {}.", tentacle2D.GetIterNum() + 1,
             tentacle2D.GetID());
    tentacle2D.Iterate();

    logDebug("Update num = {}, tentacle = {}, doing plot with angle = {}, "
             "distance = {}, distance2 = {}, color = {:x} and colorLow = {:x}.",
             m_updateNum, tentacle2D.GetID(), angle, distance, distance2, color.Rgba(),
             colorLow.Rgba());
    logDebug("tentacle head = ({:.2f}, {:.2f}, {:.2f}).", tentacle.GetHead().x,
             tentacle.GetHead().y, tentacle.GetHead().z);

    Plot3D(tentacle, color, colorLow, angle, distance, distance2, currentBuff, nextBuff);
  }
}

inline auto GetBrightnessCut(const Tentacle3D& tentacle, const float distance2) -> float
{
  if (std::abs(tentacle.GetHead().x) < 10)
  {
    if (distance2 < 8)
    {
      return 0.5;
    }
    return 0.2;
  }
  return 1.0;
}

constexpr int COORD_IGNORE_VAL = -666;

void TentacleDriver::Plot3D(const Tentacle3D& tentacle,
                            const Pixel& dominantColor,
                            const Pixel& dominantColorLow,
                            float angle,
                            float distance,
                            float distance2,
                            PixelBuffer& currentBuff,
                            PixelBuffer& nextBuff)
{
  const std::vector<V3d> vertices = tentacle.GetVertices();
  const size_t n = vertices.size();

  V3d cam = {0.0, 0.0, -3.0}; // TODO ????????????????????????????????
  cam.z += distance2;
  cam.y += 2.0F * std::sin(-(angle - m_half_pi) / 4.3F);
  logDebug("cam = ({:.2f}, {:.2f}, {:.2f}).", cam.x, cam.y, cam.z);

  float angleAboutY = angle;
  if (-10.0 < tentacle.GetHead().x && tentacle.GetHead().x < 0.0)
  {
    angleAboutY -= 0.05 * m_pi;
  }
  else if (0.0 <= tentacle.GetHead().x && tentacle.GetHead().x < 10.0)
  {
    angleAboutY += 0.05 * m_pi;
  }

  const float sina = std::sin(m_pi - angleAboutY);
  const float cosa = std::cos(m_pi - angleAboutY);
  logDebug("angle = {:.2f}, angleAboutY = {:.2f}, sina = {:.2}, cosa = {:.2},"
           " distance = {:.2f}, distance2 = {:.2f}.",
           angle, angleAboutY, sina, cosa, distance, distance2);

  std::vector<V3d> v3{vertices};
  for (size_t i = 0; i < n; i++)
  {
    logDebug("v3[{}]  = ({:.2f}, {:.2f}, {:.2f}).", i, v3[i].x, v3[i].y, v3[i].z);
    RotateV3DAboutYAxis(sina, cosa, v3[i], v3[i]);
    TranslateV3D(cam, v3[i]);
    logDebug("v3[{}]+ = ({:.2f}, {:.2f}, {:.2f}).", i, v3[i].x, v3[i].y, v3[i].z);
  }

  const std::vector<v2d> v2 = ProjectV3DOntoV2D(v3, distance);

  const float brightnessCut = GetBrightnessCut(tentacle, distance2);

  // Faraway tentacles get smaller and draw_line adds them together making them look
  // very white and over-exposed. If we reduce the brightness, then all the combined
  // tentacles look less bright and white and more colors show through.
  using GetMixedColorsFunc = std::function<std::tuple<Pixel, Pixel>(const size_t nodeNum)>;
  GetMixedColorsFunc getMixedColors = [&](const size_t nodeNum) {
    return tentacle.GetMixedColors(nodeNum, dominantColor, dominantColorLow, brightnessCut);
  };
  if (distance2 > 30.0)
  {
    const float randBrightness = getRandInRange(0.1F, 0.2F);
    float brightness = std::max(randBrightness, 30.0F / distance2) * brightnessCut;
    getMixedColors = [&, brightness](const size_t nodeNum) {
      return tentacle.GetMixedColors(nodeNum, dominantColor, dominantColorLow, brightness);
    };
  }

  for (size_t nodeNum = 0; nodeNum < v2.size() - 1; nodeNum++)
  {
    const auto ix0 = static_cast<int>(v2[nodeNum].x);
    const auto ix1 = static_cast<int>(v2[nodeNum + 1].x);
    const auto iy0 = static_cast<int>(v2[nodeNum].y);
    const auto iy1 = static_cast<int>(v2[nodeNum + 1].y);

    if (((ix0 == COORD_IGNORE_VAL) && (iy0 == COORD_IGNORE_VAL)) ||
        ((ix1 == COORD_IGNORE_VAL) && (iy1 == COORD_IGNORE_VAL)))
    {
      logDebug("Skipping draw ignore vals {}: ix0 = {}, iy0 = {}, ix1 = {}, iy1 = {}.", nodeNum,
               ix0, iy0, ix1, iy1);
    }
    else if ((ix0 == ix1) && (iy0 == iy1))
    {
      logDebug("Skipping draw equal points {}: ix0 = {}, iy0 = {}, ix1 = {}, iy1 = {}.", nodeNum,
               ix0, iy0, ix1, iy1);
    }
    else
    {
      logDebug("draw_line {}: ix0 = {}, iy0 = {}, ix1 = {}, iy1 = {}.", nodeNum, ix0, iy0, ix1,
               iy1);

      const auto [color, colorLow] = getMixedColors(nodeNum);
      /**
auto [color, colorLow] = GetMixedColors(nodeNum);
if (-10 < tentacle.getHead().x && tentacle.GetHead().x < 0)
{
  color = Pixel{0xFFFFFFFF};
  colorLow = color;
}
else if (0 <= tentacle.getHead().x && tentacle.GetHead().x < 10)
{
  color = Pixel{0xFF00FF00};
  colorLow = color;
}
**/
      const std::vector<Pixel> colors{color, colorLow};

      logDebug("draw_line {}: dominantColor = {:#x}, dominantColorLow = {:#x}.", nodeNum,
               dominantColor.Rgba(), dominantColorLow.Rgba());
      logDebug("draw_line {}: color = {:#x}, colorLow = {:#x}, brightnessCut = {:.2f}.", nodeNum,
               color.Rgba(), colorLow.Rgba(), brightnessCut);

      // TODO buff right way around ??????????????????????????????????????????????????????????????
      std::vector<PixelBuffer*> buffs{&currentBuff, &nextBuff};
      // TODO - Control brightness because of back buff??
      // One buff may be better????? Make lighten more aggressive over whole tentacle??
      // draw_line(frontBuff, ix0, iy0, ix1, iy1, color, 1280, 720);
      constexpr uint8_t THICKNESS = 1;
      m_draw.Line(buffs, ix0, iy0, ix1, iy1, colors, THICKNESS);
    }
  }
}

auto TentacleDriver::ProjectV3DOntoV2D(const std::vector<V3d>& v3, float distance) const
    -> std::vector<v2d>
{
  std::vector<v2d> v2(v3.size());

  const int Xp0 =
      v3[0].ignore || (v3[0].z <= 2) ? 1 : static_cast<int>(distance * v3[0].x / v3[0].z);
  const int Xpn = v3[v3.size() - 1].ignore || (v3[v3.size() - 1].z <= 2)
                      ? 1
                      : static_cast<int>(distance * v3[v3.size() - 1].x / v3[v3.size() - 1].z);
  const float xSpread = std::min(1.0F, std::abs(static_cast<float>(Xp0 - Xpn)) / 10.0F);

  for (size_t i = 0; i < v3.size(); ++i)
  {
    if (!v3[i].ignore && (v3[i].z > 2))
    {
      const int Xp = static_cast<int>(xSpread * distance * v3[i].x / v3[i].z);
      const int Yp = static_cast<int>(xSpread * distance * v3[i].y / v3[i].z);
      v2[i].x = Xp + static_cast<int>(m_screenWidth >> 1);
      v2[i].y = -Yp + static_cast<int>(m_screenHeight >> 1);
      logDebug("project_v3d_to_v2d i: {:3}: v3[].x = {:.2f}, v3[].y = {:.2f}, v2[].z = {:.2f}, Xp "
               "= {}, Yp = {}, v2[].x = {}, v2[].y = {}.",
               i, v3[i].x, v3[i].y, v3[i].z, Xp, Yp, v2[i].x, v2[i].y);
    }
    else
    {
      v2[i].x = v2[i].y = COORD_IGNORE_VAL;
      logDebug("project_v3d_to_v2d i: {:3}: v3[i].x = {:.2f}, v3[i].y = {:.2f}, v2[i].z = {:.2f}, "
               "v2[i].x = {}, v2[i].y = {}.",
               i, v3[i].x, v3[i].y, v3[i].z, v2[i].x, v2[i].y);
    }
  }

  return v2;
}

inline void TentacleDriver::RotateV3DAboutYAxis(float sina, float cosa, const V3d& vsrc, V3d& vdest)
{
  const float vi_x = vsrc.x;
  const float vi_z = vsrc.z;
  vdest.x = vi_x * cosa - vi_z * sina;
  vdest.z = vi_x * sina + vi_z * cosa;
  vdest.y = vsrc.y;
}

inline void TentacleDriver::TranslateV3D(const V3d& vadd, V3d& vinOut)
{
  vinOut.x += vadd.x;
  vinOut.y += vadd.y;
  vinOut.z += vadd.z;
}

TentacleColorMapColorizer::TentacleColorMapColorizer(const UTILS::ColorMapGroup cmg,
                                                     const size_t nNodes) noexcept
  : m_numNodes{nNodes},
    m_currentColorMapGroup{cmg},
    m_colorMap{&m_colorMaps.GetRandomColorMap(m_currentColorMapGroup)},
    m_prevColorMap{m_colorMap}
{
}

auto TentacleColorMapColorizer::operator==(const TentacleColorMapColorizer& t) const -> bool
{
  const bool result = m_numNodes == t.m_numNodes &&
                      m_currentColorMapGroup == t.m_currentColorMapGroup &&
                      m_colorMap == t.m_colorMap && m_prevColorMap == t.m_prevColorMap &&
                      m_tTransition == t.m_tTransition;
  if (!result)
  {
    logInfo("TentacleColorMapColorizer result == {}", result);
    logInfo("numNodes = {}, t.numNodes = {}", m_numNodes, t.m_numNodes);
    logInfo("currentColorMapGroup = {}, t.currentColorMapGroup = {}", m_currentColorMapGroup,
            t.m_currentColorMapGroup);
    logInfo("colorMap == t.colorMap = {}", m_colorMap == t.m_colorMap);
    logInfo("prevColorMap == t.prevColorMap = {}", m_prevColorMap == t.m_prevColorMap);
    logInfo("tTransition = {}, t.tTransition = {}", m_tTransition, t.m_tTransition);
  }

  return result;
}

auto TentacleColorMapColorizer::GetColorMapGroup() const -> UTILS::ColorMapGroup
{
  return m_currentColorMapGroup;
}

void TentacleColorMapColorizer::SetColorMapGroup(ColorMapGroup c)
{
  m_currentColorMapGroup = c;
}

void TentacleColorMapColorizer::ChangeColorMap()
{
  // Save the current color map to do smooth transitions to next color map.
  m_prevColorMap = m_colorMap;
  m_tTransition = 1.0;
  m_colorMap = &m_colorMaps.GetRandomColorMap(m_currentColorMapGroup);
}

auto TentacleColorMapColorizer::GetColor(size_t nodeNum) const -> Pixel
{
  const float t = static_cast<float>(nodeNum) / static_cast<float>(m_numNodes);
  Pixel nextColor = m_colorMap->GetColor(t);

  // Keep going with the smooth transition until tmix runs out.
  if (m_tTransition > 0.0)
  {
    nextColor = ColorMap::GetColorMix(nextColor, m_prevColorMap->GetColor(t), m_tTransition);
    m_tTransition -= TRANSITION_STEP;
  }

  return nextColor;
}

[[maybe_unused]] GridTentacleLayout::GridTentacleLayout(const float xmin,
                                                        const float xmax,
                                                        const size_t xNum,
                                                        const float ymin,
                                                        const float ymax,
                                                        const size_t yNum,
                                                        const float zConst)
  : m_points{}
{
  const float xStep = (xmax - xmin) / static_cast<float>(xNum - 1);
  const float yStep = (ymax - ymin) / static_cast<float>(yNum - 1);

  float y = ymin;
  for (size_t i = 0; i < yNum; i++)
  {
    float x = xmin;
    for (size_t j = 0; j < xNum; j++)
    {
      (void)m_points.emplace_back(V3d{x, y, zConst});
      x += xStep;
    }
    y += yStep;
  }
}

auto GridTentacleLayout::GetNumPoints() const -> size_t
{
  return m_points.size();
}

auto GridTentacleLayout::GetPoints() const -> const std::vector<V3d>&
{
  return m_points;
}

auto CirclesTentacleLayout::GetCircleSamples(const size_t numCircles,
                                             [[maybe_unused]] const size_t totalPoints)
    -> std::vector<size_t>
{
  std::vector<size_t> circleSamples(numCircles);

  return circleSamples;
}

CirclesTentacleLayout::CirclesTentacleLayout(const float radiusMin,
                                             const float radiusMax,
                                             const std::vector<size_t>& numCircleSamples,
                                             const float zConst)
  : m_points{}
{
  const size_t numCircles = numCircleSamples.size();
  if (numCircles < 2)
  {
    std::logic_error(std20::format("There must be >= 2 circle sample numbers not {}.", numCircles));
  }
  for (const auto numSample : numCircleSamples)
  {
    if (numSample % 2 != 0)
    {
      // Perspective looks bad with odd because of x=0 tentacle.
      std::logic_error(std20::format("Circle sample num must be even not {}.", numSample));
    }
  }

#ifndef NO_LOGGING
  // TODO - Should be lerps here?
  const auto logLastPoint = [&](size_t i, const float r, const float angle) {
    const size_t el = points.size() - 1;
    logDebug("  sample {:3}: angle = {:+6.2f}, cos(angle) = {:+6.2f}, r = {:+6.2f},"
             " pt[{:3}] = ({:+6.2f}, {:+6.2f}, {:+6.2f})",
             i, angle, cos(angle), r, el, points.at(el).x, points.at(el).y, points.at(el).z);
  };
#endif

  const auto getSamplePoints = [&](const float radius, const size_t numSample,
                                   const float angleStart, const float angleFinish) {
    const float angleStep = (angleFinish - angleStart) / static_cast<float>(numSample - 1);
    float angle = angleStart;
    for (size_t i = 0; i < numSample; i++)
    {
      const auto x = static_cast<float>(radius * std::cos(angle));
      const auto y = static_cast<float>(radius * std::sin(angle));
      const V3d point = {x, y, zConst};
      m_points.push_back(point);
#ifndef NO_LOGGING
      logLastPoint(i, radius, angle);
#endif
      angle += angleStep;
    };
  };

  const float angleLeftStart = +m_half_pi;
  const float angleLeftFinish = 1.5 * m_pi;
  const float angleRightStart = -m_half_pi;
  const float angleRightFinish = +m_half_pi;
  logDebug("Setup: angleLeftStart = {:.2f}, angleLeftFinish = {:.2f},"
           " angleRightStart = {:.2f}, angleRightFinish = {:.2f}",
           angleLeftStart, angleLeftFinish, angleRightStart, angleRightFinish);

  const float angleOffsetStart = 0.035 * m_pi;
  const float angleOffsetFinish = 0.035 * m_pi;
  const float offsetStep =
      (angleOffsetStart - angleOffsetFinish) / static_cast<float>(numCircles - 1);
  const float radiusStep = (radiusMax - radiusMin) / static_cast<float>(numCircles - 1);
  logDebug("Setup: numCircles = {}, radiusStep = {:.2f}, radiusMin = {:.2f}, radiusMax = {:.2f},"
           " offsetStep = {:.2f}, angleOffsetStart = {:.2f}, angleOffsetFinish = {:.2f}",
           numCircles, radiusStep, radiusMin, radiusMax, offsetStep, angleOffsetStart,
           angleOffsetFinish);

  float r = radiusMax;
  float angleOffset = angleOffsetStart;
  for (const auto numSample : numCircleSamples)
  {
    logDebug("Circle with {} samples: r = {:.2f}", numSample, r);

    getSamplePoints(r, numSample / 2, angleLeftStart + angleOffset, angleLeftFinish - angleOffset);
    getSamplePoints(r, numSample / 2, angleRightStart + angleOffset,
                    angleRightFinish - angleOffset);

    r -= radiusStep;
    angleOffset -= offsetStep;
  }
}

auto CirclesTentacleLayout::GetNumPoints() const -> size_t
{
  return m_points.size();
}

auto CirclesTentacleLayout::GetPoints() const -> const std::vector<V3d>&
{
  return m_points;
}

} // namespace GOOM
