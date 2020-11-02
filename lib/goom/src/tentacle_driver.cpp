#include "tentacle_driver.h"

#include "colorutils.h"
#include "goom_draw.h"
#include "goomutils/colormap.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
//#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "tentacles.h"
#include "v3d.h"

#include <algorithm>
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
  return probabilityOfMInN(1, 5);
}

const size_t TentacleDriver::changeCurrentColorMapGroupEveryNUpdates = 400;
const size_t TentacleDriver::changeTentacleColorMapEveryNUpdates = 100;

TentacleDriver::TentacleDriver() noexcept
{
}

TentacleDriver::TentacleDriver(const uint32_t screenW, const uint32_t screenH) noexcept
  : screenWidth{screenW}, screenHeight{screenH}, draw{screenWidth, screenHeight}
{
  const IterParamsGroup iter1 = {
      {100, 0.600, 1.0, {1.5, -10.0, +10.0, m_pi}, 70.0},
      {125, 0.700, 2.0, {1.0, -10.0, +10.0, 0.0}, 75.0},
  };
  const IterParamsGroup iter2 = {
      {125, 0.700, 0.5, {1.0, -10.0, +10.0, 0.0}, 70.0},
      {150, 0.800, 1.5, {1.5, -10.0, +10.0, m_pi}, 75.0},
  };
  const IterParamsGroup iter3 = {
      {150, 0.800, 1.5, {1.5, -10.0, +10.0, m_pi}, 70.0},
      {200, 0.900, 2.5, {1.0, -10.0, +10.0, 0.0}, 75.0},
  };

  iterParamsGroups = {
      iter1,
      iter2,
      iter3,
  };

  logDebug("Constructed TentacleDriver.");
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

    std::unique_ptr<TentacleColorMapColorizer> colorizer{
        new TentacleColorMapColorizer{initialColorMapGroup, params.numNodes}};
    colorizers.push_back(std::move(colorizer));

    std::unique_ptr<Tentacle2D> tentacle2D{createNewTentacle2D(i, params)};
    logDebug("Created tentacle2D {}.", i);

    // To hide the annoying flapping tentacle head, make near the head very dark.
    const Pixel headColor = getIntColor(5, 5, 5);
    const Pixel headColorLow = headColor;
    Tentacle3D tentacle{std::move(tentacle2D), *colorizers[colorizers.size() - 1], headColor,
                        headColorLow, initialHeadPos};

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

  //  const size_t tentacleLen =
  //      size_t(getRandInRange(0.99f, 1.01f) * static_cast<float>(params.length));
  const size_t tentacleLen = params.length;
  const double tent2d_xmax = tent2d_xmin + tentacleLen;

  std::unique_ptr<Tentacle2D> tentacle{
      new Tentacle2D{ID, std::move(createNewTweaker(
                             params, std::move(createNewDampingFunction(params, tentacleLen))))}};
  logDebug("Created new tentacle2D {}.", ID);

  tentacle->setXDimensions(tent2d_xmin, tent2d_xmax);
  tentacle->setYDimensions(tent2d_ymin, tent2d_ymax);

  tentacle->setPrevYWeight(params.prevYWeight);
  tentacle->setCurrentYWeight(1.0 - params.prevYWeight);
  //  tentacle->setNumNodes(size_t(static_cast<float>(params.numNodes) * getRandInRange(0.9f, 1.1f)));
  tentacle->setNumNodes(params.numNodes);
  logDebug("tentacle {:3}:"
           " tentacleLen = {:4}, tent2d_xmin = {:7.2f}, tent2d_xmax = {:5.2f},"
           " prevYWeight = {:5.2f}, curYWeight = {:5.2f}, numNodes = {:5}",
           ID, tentacleLen, tent2d_xmin, tent2d_xmax, tentacle->getPrevYWeight(),
           tentacle->getCurrentYWeight(), tentacle->getNumNodes());

  tentacle->setDoDamping(true);

  return tentacle;
}

std::unique_ptr<DampingFunction> TentacleDriver::createNewDampingFunction(
    const IterationParams& params, const size_t tentacleLen) const
{
  if (params.prevYWeight < 0.6)
  {
    return createNewLinearDampingFunction(params, tentacleLen);
  }
  return createNewExpDampingFunction(params, tentacleLen);
}

std::unique_ptr<DampingFunction> TentacleDriver::createNewExpDampingFunction(
    const IterationParams&, const size_t tentacleLen) const
{
  const double tent2d_xmax = tent2d_xmin + tentacleLen;

  const double xRiseStart = tent2d_xmin + 0.25 * tent2d_xmax;
  constexpr double dampStart = 5;
  constexpr double dampMax = 30;

  return std::unique_ptr<DampingFunction>{
      new ExpDampingFunction{0.1, xRiseStart, dampStart, tent2d_xmax, dampMax}};
}

std::unique_ptr<DampingFunction> TentacleDriver::createNewLinearDampingFunction(
    const IterationParams&, const size_t tentacleLen) const
{
  const double tent2d_xmax = tent2d_xmin + tentacleLen;

  constexpr float yScale = 30;

  std::vector<std::tuple<double, double, std::unique_ptr<DampingFunction>>> pieces{};
  pieces.emplace_back(
      std::make_tuple(tent2d_xmin, 0.1 * tent2d_xmax,
                      std::unique_ptr<DampingFunction>{new FlatDampingFunction{0.1}}));
  pieces.emplace_back(std::make_tuple(0.1 * tent2d_xmax, 10 * tent2d_xmax,
                                      std::unique_ptr<DampingFunction>{new LinearDampingFunction{
                                          0.1 * tent2d_xmax, 0.1, tent2d_xmax, yScale}}));

  return std::unique_ptr<DampingFunction>{new PiecewiseDampingFunction{pieces}};
}

std::unique_ptr<TentacleTweaker> TentacleDriver::createNewTweaker(
    const IterationParams&, std::unique_ptr<DampingFunction> dampingFunc)
{
  using namespace std::placeholders;

  return std::unique_ptr<TentacleTweaker>{new TentacleTweaker{std::move(dampingFunc)}};
}

void TentacleDriver::startIterating()
{
  for (auto& t : tentacles)
  {
    t.get2DTentacle().startIterating();
  }
}

void TentacleDriver::stopIterating()
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
      const double newXmax = xmin + 0.6 * (xmax - xmin);
      tentacle2D.setXDimensions(xmin, newXmax);
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
      for (size_t i = 0; i < colorizers.size(); i++)
      {
        if (changeCurrentColorMapEvent())
        {
          colorizers[i]->changeColorMap();
        }
      }
    }
  }
}

void TentacleDriver::freshStart()
{
  const utils::ColorMapGroup nextColorMapGroup = colorMaps.getRandomGroup();
  for (auto& c : colorizers)
  {
    c->setColorMapGroup(nextColorMapGroup);
    if (colorMode != ColorModes::minimal)
    {
      c->changeColorMap();
    }
  }
}

void TentacleDriver::update(const float angle,
                            const float distance,
                            const float distance2,
                            const Pixel& color,
                            const Pixel& colorLow,
                            Pixel* prevBuff,
                            Pixel* currentBuff)
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
             "distance = {}, distance2 = {}, color = {} and colorLow = {}, doDraw = {}.",
             updateNum, tentacle2D.getID(), angle, distance, distance2, color, colorLow, doDraw);

    plot3D(tentacle, color, colorLow, angle, distance, distance2, prevBuff, currentBuff);
  }
}

constexpr int coordIgnoreVal = -666;

void TentacleDriver::plot3D(const Tentacle3D& tentacle,
                            const Pixel& dominantColor,
                            const Pixel& dominantColorLow,
                            const float angle,
                            const float distance,
                            const float distance2,
                            Pixel* prevBuff,
                            Pixel* currentBuff)
{
  const std::vector<V3d> vertices = tentacle.getVertices();
  const size_t n = vertices.size();

  V3d cam = {0, 0, 3}; // TODO ????????????????????????????????
  cam.z += distance2;
  cam.y += 2.0 * sin(-(angle - m_half_pi) / 4.3f);
  logDebug("cam = ({:.2f}, {:.2f}, {:.2f}).", cam.x, cam.y, cam.z);

  const float sina = sin(m_pi - angle);
  const float cosa = cos(m_pi - angle);
  logDebug("angle = {:.2f}, sina = {:.2}, cosa = {:.2},"
           " distance = {:.2f}, distance2 = {:.2f}.",
           angle, sina, cosa, distance, distance2);

  std::vector<V3d> v3{vertices};
  for (size_t i = 0; i < n; i++)
  {
    logDebug("v3[{}]  = ({:.2f}, {:.2f}, {:.2f}).", i, v3[i].x, v3[i].y, v3[i].z);
    rotateV3dAboutYAxis(sina, cosa, v3[i], v3[i]);
    translateV3d(cam, v3[i]);
    logDebug("v3[{}]+ = ({:.2f}, {:.2f}, {:.2f}).", i, v3[i].x, v3[i].y, v3[i].z);
  }

  const std::vector<v2d> v2 = projectV3dOntoV2d(v3, distance);

  // Faraway tentacles get smaller and draw_line adds them together making them look
  // very white and over-exposed. If we reduce the brightness, then all the combined
  // tentacles look less bright and white and more colors show through.
  using GetMixedColorsFunc = std::function<std::tuple<Pixel, Pixel>(const size_t nodeNum)>;
  GetMixedColorsFunc getMixedColors = [&](const size_t nodeNum) {
    return tentacle.getMixedColors(nodeNum, dominantColor, dominantColorLow);
  };
  if (distance2 > 50)
  {
    const float randBrightness = getRandInRange(0.1f, 0.5f);
    const float brightness = std::max(randBrightness, 50.0f / distance2);
    getMixedColors = [&, brightness](const size_t nodeNum) {
      return tentacle.getMixedColors(nodeNum, dominantColor, dominantColorLow, brightness);
    };
  }

  for (size_t nodeNum = 0; nodeNum < v2.size() - 1; nodeNum++)
  {
    const int ix0 = static_cast<int>(v2[nodeNum].x);
    const int ix1 = static_cast<int>(v2[nodeNum + 1].x);
    const int iy0 = static_cast<int>(v2[nodeNum].y);
    const int iy1 = static_cast<int>(v2[nodeNum + 1].y);

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
      const std::vector<Pixel> colors{color, colorLow};

      logInfo("draw_line {}: dominantColor = {:#x}, dominantColorLow = {:#x}.", nodeNum,
              dominantColor, dominantColorLow);
      logInfo("draw_line {}: color = {:#x}, colorLow = {:#x}.", nodeNum, color, colorLow);

      // TODO buff right way around ??????????????????????????????????????????????????????????????
      std::vector<Pixel*> buffs{currentBuff, prevBuff};
      // TODO - Control brightness because of back buff??
      // One buff may be better????? Make lighten more aggressive over whole tentacle??
      // draw_line(frontBuff, ix0, iy0, ix1, iy1, color, 1280, 720);
      constexpr uint8_t thickness = 1;
      draw.line(buffs, ix0, iy0, ix1, iy1, colors, thickness);
    }
  }
}

std::vector<v2d> TentacleDriver::projectV3dOntoV2d(const std::vector<V3d>& v3, const float distance)
{
  std::vector<v2d> v2(v3.size());

  for (size_t i = 0; i < v3.size(); ++i)
  {
    logDebug("project_v3d_to_v2d {}: v3[i].x = {:.2f}, v3[i].y = {:.2f}, v2[i].z = {:.2f}.", i,
             v3[i].x, v3[i].y, v3[i].z);
    if (!v3[i].ignore && (v3[i].z > 2))
    {
      const int Xp = static_cast<int>(distance * v3[i].x / v3[i].z);
      const int Yp = static_cast<int>(distance * v3[i].y / v3[i].z);
      v2[i].x = Xp + static_cast<int>(screenWidth >> 1);
      v2[i].y = -Yp + static_cast<int>(screenHeight >> 1);
      logDebug("project_v3d_to_v2d {}: Xp = {}, Yp = {}, v2[i].x = {}, v2[i].y = {}.", i, Xp, Yp,
               v2[i].x, v2[i].y);
    }
    else
    {
      v2[i].x = v2[i].y = coordIgnoreVal;
      logDebug("project_v3d_to_v2d {}: v2[i].x = {}, v2[i].y = {}.", i, v2[i].x, v2[i].y);
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
                                                     const size_t nNodes)
  : currentColorMapGroup{cmg},
    colorMap{&colorMaps.getRandomColorMap(currentColorMapGroup)},
    prevColorMap{colorMap},
    tTransition{0},
    numNodes{nNodes}
{
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

GridTentacleLayout::GridTentacleLayout(const float xmin,
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
      const float x = static_cast<float>(radius * cos(angle));
      const float y = static_cast<float>(radius * sin(angle));
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

  const float angleOffsetStart = 0.10;
  const float angleOffsetFinish = 0.30;
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
