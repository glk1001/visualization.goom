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

namespace goom
{

using namespace goom::utils;

inline bool changeCurrentColorMapEvent()
{
  return probabilityOfMInN(3, 5);
}

const size_t TentacleDriver::changeCurrentColorMapGroupEveryNUpdates = 400;
const size_t TentacleDriver::changeTentacleColorMapEveryNUpdates = 100;

TentacleDriver::TentacleDriver() noexcept = default;

TentacleDriver::TentacleDriver(const uint32_t screenW, const uint32_t screenH) noexcept
  : screenWidth{screenW}, screenHeight{screenH}, draw{screenWidth, screenHeight}
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

  iterParamsGroups = {
      iter1,
      iter2,
      iter3,
  };

  logDebug("Constructed TentacleDriver.");
}

bool TentacleDriver::IterationParams::operator==(const IterationParams& p) const
{
  return numNodes == p.numNodes && prevYWeight == p.prevYWeight &&
         iterZeroYValWaveFreq == p.iterZeroYValWave && iterZeroYValWave == p.iterZeroYValWave &&
         length == p.length;
}

bool TentacleDriver::IterParamsGroup::operator==(const IterParamsGroup& p) const
{
  return first == p.first && last == p.last;
}

bool TentacleDriver::operator==(const TentacleDriver& t) const
{
  const bool result = colorMode == t.colorMode && iterParamsGroups == t.iterParamsGroups &&
                      screenWidth == t.screenWidth && screenHeight == t.screenHeight &&
                      draw == t.draw && buffSettings == t.buffSettings &&
                      updateNum == t.updateNum && tentacles == t.tentacles &&
                      numTentacles == t.numTentacles && tentacleParams == t.tentacleParams;

  if (result)
  {
    for (size_t i = 0; i < colorizers.size(); i++)
    {
      if (typeid(*colorizers[i]) != typeid(*t.colorizers[i]))
      {
        logInfo("TentacleDriver colorizers not of same type at index {}", i);
        return false;
      }
      const auto* c1 = dynamic_cast<const TentacleColorMapColorizer*>(colorizers[i].get());
      if (c1 == nullptr)
      {
        continue;
      }
      const auto* c2 = dynamic_cast<const TentacleColorMapColorizer*>(t.colorizers[i].get());
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
    logInfo("colorMode = {}, t.colorMode = {}", colorMode, t.colorMode);
    logInfo("iterParamsGroups == t.iterParamsGroups = {}", iterParamsGroups == t.iterParamsGroups);
    logInfo("screenWidth = {}, t.screenWidth = {}", screenWidth, t.screenWidth);
    logInfo("screenHeight = {}, t.screenHeight = {}", screenHeight, t.screenHeight);
    logInfo("draw == t.draw = {}", draw == t.draw);
    logInfo("buffSettings == t.buffSettings = {}", buffSettings == t.buffSettings);
    logInfo("updateNum = {}, t.updateNum = {}", updateNum, t.updateNum);
    logInfo("tentacles == t.tentacles = {}", tentacles == t.tentacles);
    logInfo("numTentacles = {}, t.numTentacles = {}", numTentacles, t.numTentacles);
    logInfo("tentacleParams == t.tentacleParams = {}", tentacleParams == t.tentacleParams);
  }

  return result;
}

const FXBuffSettings& TentacleDriver::getBuffSettings() const
{
  return buffSettings;
}

void TentacleDriver::setBuffSettings(const FXBuffSettings& settings)
{
  buffSettings = settings;
  draw.setBuffIntensity(buffSettings.buffIntensity);
  draw.setAllowOverexposed(buffSettings.allowOverexposed);
  tentacles.setAllowOverexposed(buffSettings.allowOverexposed);
}

size_t TentacleDriver::getNumTentacles() const
{
  return numTentacles;
}

TentacleDriver::ColorModes TentacleDriver::getColorMode() const
{
  return colorMode;
}

void TentacleDriver::setColorMode(const ColorModes c)
{
  colorMode = c;
}

constexpr double tent2d_xmin = 0.0;
constexpr double tent2d_ymin = 0.065736;
constexpr double tent2d_ymax = 10000;

void TentacleDriver::init(const TentacleLayout& layout)
{
  logDebug("Starting driver init.");

  numTentacles = layout.getNumPoints();
  logDebug("numTentacles = {}.", numTentacles);

  tentacleParams.resize(numTentacles);

  constexpr V3d initialHeadPos = {0, 0, 0};

  const size_t numInParamGroup = numTentacles / iterParamsGroups.size();
  const float tStep = 1.0F / static_cast<float>(numInParamGroup - 1);
  logDebug("numInTentacleGroup = {}, tStep = {:.2f}.", numInParamGroup, tStep);

  const utils::ColorMapGroup initialColorMapGroup = colorMaps.getRandomGroup();

  size_t paramsIndex = 0;
  float t = 0;
  for (size_t i = 0; i < numTentacles; i++)
  {
    const IterParamsGroup paramsGrp = iterParamsGroups.at(paramsIndex);
    if (i % numInParamGroup == 0)
    {
      if (paramsIndex < iterParamsGroups.size() - 1)
      {
        paramsIndex++;
      }
      t = 0;
    }
    const IterationParams params = paramsGrp.getNext(t);
    t += tStep;
    tentacleParams[i] = params;

    std::shared_ptr<TentacleColorMapColorizer> colorizer{
        new TentacleColorMapColorizer{initialColorMapGroup, params.numNodes}};
    colorizers.push_back(std::move(colorizer));

    std::unique_ptr<Tentacle2D> tentacle2D{createNewTentacle2D(i, params)};
    logDebug("Created tentacle2D {}.", i);

    // To hide the annoying flapping tentacle head, make near the head very dark.
    const Pixel headColor = getIntColor(5, 5, 5);
    const Pixel headColorLow = headColor;
    Tentacle3D tentacle{std::move(tentacle2D),
                        colorizers[colorizers.size() - 1],
                        headColor,
                        headColorLow,
                        initialHeadPos,
                        Tentacle2D::minNumNodes};

    tentacles.addTentacle(std::move(tentacle));

    logDebug("Added tentacle {}.", i);
  }

  updateTentaclesLayout(layout);

  updateNum = 0;
}

TentacleDriver::IterationParams TentacleDriver::IterParamsGroup::getNext(const float t) const
{
  const float prevYWeight =
      getRandInRange(0.9f, 1.1f) *
      std::lerp(static_cast<float>(first.prevYWeight), static_cast<float>(last.prevYWeight), t);
  IterationParams params{};
  params.length =
      size_t(getRandInRange(1.0f, 1.1f) *
             std::lerp(static_cast<float>(first.length), static_cast<float>(last.length), t));
  params.numNodes =
      size_t(getRandInRange(0.9f, 1.1f) *
             std::lerp(static_cast<float>(first.numNodes), static_cast<float>(last.numNodes), t));
  params.prevYWeight = prevYWeight;
  params.iterZeroYValWave = first.iterZeroYValWave;
  params.iterZeroYValWaveFreq =
      getRandInRange(0.9f, 1.1f) * std::lerp(static_cast<float>(first.iterZeroYValWaveFreq),
                                             static_cast<float>(last.iterZeroYValWaveFreq), t);
  return params;
}

std::unique_ptr<Tentacle2D> TentacleDriver::createNewTentacle2D(const size_t ID,
                                                                const IterationParams& params)
{
  logDebug("Creating new tentacle2D {}...", ID);

  const size_t tentacleLen = getRandInRange(0.99f, 1.01f) * static_cast<float>(params.length);
  //const size_t tentacleLen = params.length;
  const double tent2d_xmax = tent2d_xmin + tentacleLen;

  std::unique_ptr<Tentacle2D> tentacle{
      new Tentacle2D{ID, params.numNodes,
                     //  size_t(static_cast<float>(params.numNodes) * getRandInRange(0.9f, 1.1f))
                     tent2d_xmin, tent2d_xmax, tent2d_ymin, tent2d_ymax, params.prevYWeight,
                     1.0 - params.prevYWeight}};
  logDebug("Created new tentacle2D {}.", ID);

  logDebug("tentacle {:3}:"
           " tentacleLen = {:4}, tent2d_xmin = {:7.2f}, tent2d_xmax = {:5.2f},"
           " prevYWeight = {:5.2f}, curYWeight = {:5.2f}, numNodes = {:5}",
           ID, tentacleLen, tent2d_xmin, tent2d_xmax, tentacle->getPrevYWeight(),
           tentacle->getCurrentYWeight(), tentacle->getNumNodes());

  tentacle->setDoDamping(true);

  return tentacle;
}

void TentacleDriver::startIterating()
{
  for (auto& t : tentacles)
  {
    t.get2DTentacle().startIterating();
  }
}

[[maybe_unused]] void TentacleDriver::stopIterating()
{
  for (auto& t : tentacles)
  {
    t.get2DTentacle().finishIterating();
  }
}

void TentacleDriver::updateTentaclesLayout(const TentacleLayout& layout)
{
  logDebug("Updating tentacles layout. numTentacles = {}.", numTentacles);
  assert(layout.getNumPoints() == numTentacles);

  std::vector<size_t> sortedLongestFirst(numTentacles);
  std::iota(sortedLongestFirst.begin(), sortedLongestFirst.end(), 0);
  const auto compareByLength = [this](const size_t id1, const size_t id2) -> bool {
    const double len1 = tentacles[id1].get2DTentacle().getLength();
    const double len2 = tentacles[id2].get2DTentacle().getLength();
    // Sort by longest first.
    return len1 > len2;
  };
  std::sort(sortedLongestFirst.begin(), sortedLongestFirst.end(), compareByLength);

  for (size_t i = 0; i < numTentacles; i++)
  {
    logDebug("{} {} tentacle[{}].len = {:.2}.", i, sortedLongestFirst.at(i),
             sortedLongestFirst.at(i),
             tentacles[sortedLongestFirst.at(i)].get2DTentacle().getLength());
  }

  for (size_t i = 0; i < numTentacles; i++)
  {
    tentacles[sortedLongestFirst.at(i)].setHead(layout.getPoints().at(i));
  }

  // To help with perspective, any tentacles near vertical centre will be shortened.
  for (auto& tentacle : tentacles)
  {
    const V3d& head = tentacle.getHead();
    if (std::fabs(head.x) < 10)
    {
      Tentacle2D& tentacle2D = tentacle.get2DTentacle();
      const double xmin = tentacle2D.getXMin();
      const double xmax = tentacle2D.getXMax();
      const double newXmax = xmin + 1.0 * (xmax - xmin);
      tentacle2D.setXDimensions(xmin, newXmax);
      tentacle.setNumHeadNodes(
          std::max(6 * Tentacle2D::minNumNodes, tentacle.get2DTentacle().getNumNodes() / 2));
    }
  }
}

void TentacleDriver::multiplyIterZeroYValWaveFreq(const float val)
{
  for (size_t i = 0; i < numTentacles; i++)
  {
    const float newFreq = val * tentacleParams[i].iterZeroYValWaveFreq;
    tentacleParams[i].iterZeroYValWave.setFrequency(newFreq);
  }
}

void TentacleDriver::setReverseColorMix(const bool val)
{
  for (auto& t : tentacles)
  {
    t.setReverseColorMix(val);
  }
}

void TentacleDriver::updateIterTimers()
{
  for (auto* t : iterTimers)
  {
    t->next();
  }
}

std::vector<utils::ColorMapGroup> TentacleDriver::getNextColorMapGroups() const
{
  const size_t numDifferentGroups =
      (colorMode == ColorModes::minimal || colorMode == ColorModes::oneGroupForAll ||
       probabilityOfMInN(99, 100))
          ? 1
          : getRandInRange(1U, std::min(size_t(5U), colorizers.size()));
  std::vector<utils::ColorMapGroup> groups(numDifferentGroups);
  for (size_t i = 0; i < numDifferentGroups; i++)
  {
    groups[i] = colorMaps.getRandomGroup();
  }

  std::vector<utils::ColorMapGroup> nextColorMapGroups(colorizers.size());
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

void TentacleDriver::checkForTimerEvents()
{
  //  logDebug("Update num = {}: checkForTimerEvents", updateNum);

  if (updateNum % changeCurrentColorMapGroupEveryNUpdates == 0)
  {
    const std::vector<utils::ColorMapGroup> nextGroups = getNextColorMapGroups();
    for (size_t i = 0; i < colorizers.size(); i++)
    {
      colorizers[i]->setColorMapGroup(nextGroups[i]);
    }
    if (colorMode != ColorModes::minimal)
    {
      for (auto& colorizer : colorizers)
      {
        if (changeCurrentColorMapEvent())
        {
          colorizer->changeColorMap();
        }
      }
    }
  }
}

void TentacleDriver::freshStart()
{
  const utils::ColorMapGroup nextColorMapGroup = colorMaps.getRandomGroup();
  for (auto& colorizer : colorizers)
  {
    colorizer->setColorMapGroup(nextColorMapGroup);
    if (colorMode != ColorModes::minimal)
    {
      colorizer->changeColorMap();
    }
  }
}

void TentacleDriver::update(const float angle,
                            const float distance,
                            const float distance2,
                            const Pixel& color,
                            const Pixel& colorLow,
                            PixelBuffer& currentBuff,
                            PixelBuffer& nextBuff)
{
  updateNum++;
  logInfo("Doing update {}.", updateNum);

  updateIterTimers();
  checkForTimerEvents();

  constexpr float iterZeroLerpFactor = 0.9;

  for (size_t i = 0; i < numTentacles; i++)
  {
    Tentacle3D& tentacle = tentacles[i];
    Tentacle2D& tentacle2D = tentacle.get2DTentacle();

    const float iterZeroYVal = tentacleParams[i].iterZeroYValWave.getNext();
    tentacle2D.setIterZeroLerpFactor(iterZeroLerpFactor);
    tentacle2D.setIterZeroYVal(iterZeroYVal);

    logDebug("Starting iterate {} for tentacle {}.", tentacle2D.getIterNum() + 1,
             tentacle2D.getID());
    tentacle2D.iterate();

    logDebug("Update num = {}, tentacle = {}, doing plot with angle = {}, "
             "distance = {}, distance2 = {}, color = {:x} and colorLow = {:x}.",
             updateNum, tentacle2D.getID(), angle, distance, distance2, color.rgba(),
             colorLow.rgba());
    logDebug("tentacle head = ({:.2f}, {:.2f}, {:.2f}).", tentacle.getHead().x,
             tentacle.getHead().y, tentacle.getHead().z);

    plot3D(tentacle, color, colorLow, angle, distance, distance2, currentBuff, nextBuff);
  }
}

inline float getBrightnessCut(const Tentacle3D& tentacle, const float distance2)
{
  if (std::abs(tentacle.getHead().x) < 10)
  {
    if (distance2 < 8)
    {
      return 0.5;
    }
    return 0.2;
  }
  return 1.0;
}

constexpr int coordIgnoreVal = -666;

void TentacleDriver::plot3D(const Tentacle3D& tentacle,
                            const Pixel& dominantColor,
                            const Pixel& dominantColorLow,
                            const float angle,
                            const float distance,
                            const float distance2,
                            PixelBuffer& currentBuff,
                            PixelBuffer& nextBuff)
{
  const std::vector<V3d> vertices = tentacle.getVertices();
  const size_t n = vertices.size();

  V3d cam = {0, 0, -3}; // TODO ????????????????????????????????
  cam.z += distance2;
  cam.y += 2.0F * std::sin(-(angle - m_half_pi) / 4.3F);
  logDebug("cam = ({:.2f}, {:.2f}, {:.2f}).", cam.x, cam.y, cam.z);

  float angleAboutY = angle;
  if (-10 < tentacle.getHead().x && tentacle.getHead().x < 0)
  {
    angleAboutY -= 0.05 * m_pi;
  }
  else if (0 <= tentacle.getHead().x && tentacle.getHead().x < 10)
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
    rotateV3dAboutYAxis(sina, cosa, v3[i], v3[i]);
    translateV3d(cam, v3[i]);
    logDebug("v3[{}]+ = ({:.2f}, {:.2f}, {:.2f}).", i, v3[i].x, v3[i].y, v3[i].z);
  }

  const std::vector<v2d> v2 = projectV3dOntoV2d(v3, distance);

  const float brightnessCut = getBrightnessCut(tentacle, distance2);

  // Faraway tentacles get smaller and draw_line adds them together making them look
  // very white and over-exposed. If we reduce the brightness, then all the combined
  // tentacles look less bright and white and more colors show through.
  using GetMixedColorsFunc = std::function<std::tuple<Pixel, Pixel>(const size_t nodeNum)>;
  GetMixedColorsFunc getMixedColors = [&](const size_t nodeNum) {
    return tentacle.getMixedColors(nodeNum, dominantColor, dominantColorLow, brightnessCut);
  };
  if (distance2 > 30)
  {
    const float randBrightness = getRandInRange(0.1f, 0.2f);
    float brightness = std::max(randBrightness, 30.0F / distance2) * brightnessCut;
    getMixedColors = [&, brightness](const size_t nodeNum) {
      return tentacle.getMixedColors(nodeNum, dominantColor, dominantColorLow, brightness);
    };
  }

  for (size_t nodeNum = 0; nodeNum < v2.size() - 1; nodeNum++)
  {
    const auto ix0 = static_cast<int>(v2[nodeNum].x);
    const auto ix1 = static_cast<int>(v2[nodeNum + 1].x);
    const auto iy0 = static_cast<int>(v2[nodeNum].y);
    const auto iy1 = static_cast<int>(v2[nodeNum + 1].y);

    if (((ix0 == coordIgnoreVal) && (iy0 == coordIgnoreVal)) ||
        ((ix1 == coordIgnoreVal) && (iy1 == coordIgnoreVal)))
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
auto [color, colorLow] = getMixedColors(nodeNum);
if (-10 < tentacle.getHead().x && tentacle.getHead().x < 0)
{
  color = Pixel{0xFFFFFFFF};
  colorLow = color;
}
else if (0 <= tentacle.getHead().x && tentacle.getHead().x < 10)
{
  color = Pixel{0xFF00FF00};
  colorLow = color;
}
**/
      const std::vector<Pixel> colors{color, colorLow};

      logDebug("draw_line {}: dominantColor = {:#x}, dominantColorLow = {:#x}.", nodeNum,
               dominantColor.rgba(), dominantColorLow.rgba());
      logDebug("draw_line {}: color = {:#x}, colorLow = {:#x}, brightnessCut = {:.2f}.", nodeNum,
               color.rgba(), colorLow.rgba(), brightnessCut);

      // TODO buff right way around ??????????????????????????????????????????????????????????????
      std::vector<PixelBuffer*> buffs{&currentBuff, &nextBuff};
      // TODO - Control brightness because of back buff??
      // One buff may be better????? Make lighten more aggressive over whole tentacle??
      // draw_line(frontBuff, ix0, iy0, ix1, iy1, color, 1280, 720);
      constexpr uint8_t thickness = 1;
      draw.line(buffs, ix0, iy0, ix1, iy1, colors, thickness);
    }
  }
}

std::vector<v2d> TentacleDriver::projectV3dOntoV2d(const std::vector<V3d>& v3,
                                                   const float distance) const
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
      v2[i].x = Xp + static_cast<int>(screenWidth >> 1);
      v2[i].y = -Yp + static_cast<int>(screenHeight >> 1);
      logDebug("project_v3d_to_v2d i: {:3}: v3[].x = {:.2f}, v3[].y = {:.2f}, v2[].z = {:.2f}, Xp "
               "= {}, Yp = {}, v2[].x = {}, v2[].y = {}.",
               i, v3[i].x, v3[i].y, v3[i].z, Xp, Yp, v2[i].x, v2[i].y);
    }
    else
    {
      v2[i].x = v2[i].y = coordIgnoreVal;
      logDebug("project_v3d_to_v2d i: {:3}: v3[i].x = {:.2f}, v3[i].y = {:.2f}, v2[i].z = {:.2f}, "
               "v2[i].x = {}, v2[i].y = {}.",
               i, v3[i].x, v3[i].y, v3[i].z, v2[i].x, v2[i].y);
    }
  }

  return v2;
}

inline void TentacleDriver::rotateV3dAboutYAxis(const float sina,
                                                const float cosa,
                                                const V3d& vsrc,
                                                V3d& vdest)
{
  const float vi_x = vsrc.x;
  const float vi_z = vsrc.z;
  vdest.x = vi_x * cosa - vi_z * sina;
  vdest.z = vi_x * sina + vi_z * cosa;
  vdest.y = vsrc.y;
}

inline void TentacleDriver::translateV3d(const V3d& vadd, V3d& vinOut)
{
  vinOut.x += vadd.x;
  vinOut.y += vadd.y;
  vinOut.z += vadd.z;
}

TentacleColorMapColorizer::TentacleColorMapColorizer(const utils::ColorMapGroup cmg,
                                                     const size_t nNodes) noexcept
  : numNodes{nNodes},
    currentColorMapGroup{cmg},
    colorMap{&colorMaps.getRandomColorMap(currentColorMapGroup)},
    prevColorMap{colorMap}
{
}

bool TentacleColorMapColorizer::operator==(const TentacleColorMapColorizer& t) const
{
  const bool result = numNodes == t.numNodes && currentColorMapGroup == t.currentColorMapGroup &&
                      colorMap == t.colorMap && prevColorMap == t.prevColorMap &&
                      tTransition == t.tTransition;
  if (!result)
  {
    logInfo("TentacleColorMapColorizer result == {}", result);
    logInfo("numNodes = {}, t.numNodes = {}", numNodes, t.numNodes);
    logInfo("currentColorMapGroup = {}, t.currentColorMapGroup = {}", currentColorMapGroup,
            t.currentColorMapGroup);
    logInfo("colorMap == t.colorMap = {}", colorMap == t.colorMap);
    logInfo("prevColorMap == t.prevColorMap = {}", prevColorMap == t.prevColorMap);
    logInfo("tTransition = {}, t.tTransition = {}", tTransition, t.tTransition);
  }

  return result;
}

utils::ColorMapGroup TentacleColorMapColorizer::getColorMapGroup() const
{
  return currentColorMapGroup;
}

void TentacleColorMapColorizer::setColorMapGroup(const utils::ColorMapGroup val)
{
  currentColorMapGroup = val;
}

void TentacleColorMapColorizer::changeColorMap()
{
  // Save the current color map to do smooth transitions to next color map.
  prevColorMap = colorMap;
  tTransition = 1.0;
  colorMap = &colorMaps.getRandomColorMap(currentColorMapGroup);
}

Pixel TentacleColorMapColorizer::getColor(const size_t nodeNum) const
{
  const float t = static_cast<float>(nodeNum) / static_cast<float>(numNodes);
  Pixel nextColor = colorMap->getColor(t);

  // Keep going with the smooth transition until tmix runs out.
  if (tTransition > 0.0)
  {
    nextColor = ColorMap::colorMix(nextColor, prevColorMap->getColor(t), tTransition);
    tTransition -= transitionStep;
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
  : points{}
{
  const float xStep = (xmax - xmin) / static_cast<float>(xNum - 1);
  const float yStep = (ymax - ymin) / static_cast<float>(yNum - 1);

  float y = ymin;
  for (size_t i = 0; i < yNum; i++)
  {
    float x = xmin;
    for (size_t j = 0; j < xNum; j++)
    {
      points.emplace_back(V3d{x, y, zConst});
      x += xStep;
    }
    y += yStep;
  }
}

size_t GridTentacleLayout::getNumPoints() const
{
  return points.size();
}

const std::vector<V3d>& GridTentacleLayout::getPoints() const
{
  return points;
}

std::vector<size_t> CirclesTentacleLayout::getCircleSamples(
    const size_t numCircles, [[maybe_unused]] const size_t totalPoints)
{
  std::vector<size_t> circleSamples(numCircles);

  return circleSamples;
}

CirclesTentacleLayout::CirclesTentacleLayout(const float radiusMin,
                                             const float radiusMax,
                                             const std::vector<size_t>& numCircleSamples,
                                             const float zConst)
  : points{}
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
      points.push_back(point);
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

size_t CirclesTentacleLayout::getNumPoints() const
{
  return points.size();
}

const std::vector<V3d>& CirclesTentacleLayout::getPoints() const
{
  return points;
}

} // namespace goom
