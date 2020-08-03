#include "tentacle_driver.h"

#include "colormap.h"
#include "drawmethods.h"
#include "goom.h"
#include "tentacles_new.h"
#include "v3d.h"

#include <fmt/format.h>

#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <stack>
#include <stdexcept>
#include <utility>
#include <vector>


TentacleColorMapColorizer::TentacleColorMapColorizer(const ColorMap& cm, const size_t nNodes)
  : origColorMap{ &cm }
  , colorMap{ &cm }
  , colorStack()
  , numNodes{ nNodes }
{
}

void TentacleColorMapColorizer::resetColorMap(const ColorMap& cm)
{
  origColorMap = &cm;
  colorMap = &cm;
  colorStack = std::stack<const ColorMap*>{ };
}

void TentacleColorMapColorizer::pushColorMap(const ColorMap& cm)
{
  colorStack.push(colorMap);
  colorMap = &cm;
}

void TentacleColorMapColorizer::TentacleColorMapColorizer::popColorMap()
{
  if (colorStack.empty()) {
    return;
  }
  colorMap = colorStack.top();
  colorStack.pop();
}

uint32_t TentacleColorMapColorizer::getColor(size_t nodeNum) const
{
  const size_t colorNum = colorMap->numColors()*nodeNum/numNodes;
  return colorMap->getColor(colorNum);
}

SimpleWeightHandler::SimpleWeightHandler(
    ConstantSequenceFunction& prevYWeightFun,
    ConstantSequenceFunction& currentYWeightFun)
  : prevYWeightFunc(&prevYWeightFun)
  , currentYWeightFunc(&currentYWeightFun)
{
}

void SimpleWeightHandler::weightsReset(
    [[ maybe_unused ]]const size_t ID, [[ maybe_unused ]]const size_t nmNodes,
    const float basePrevYWgt, const float baseCurrentYWgt)
{
  basePrevYWeight = basePrevYWgt;
  baseCurrentYWeight = baseCurrentYWgt;
  prevYWeightFunc->setConstVal(basePrevYWeight);
  currentYWeightFunc->setConstVal(baseCurrentYWeight);
}

void SimpleWeightHandler::weightsAdjust(
    [[ maybe_unused ]]const size_t ID,
    [[ maybe_unused ]]const size_t iterNum, [[ maybe_unused ]]const size_t nodeNum,
    [[ maybe_unused ]]const float prevY, [[ maybe_unused ]]const float currentY)
{
  prevYWeightFunc->setConstVal(basePrevYWeight * getRandInRange(0.8f, 1.2f));
  currentYWeightFunc->setConstVal(1.0 - prevYWeightFunc->getConstVal());
}

RandWeightHandler::RandWeightHandler(
    RandSequenceFunction& prevYWeightFun,
    RandSequenceFunction& currentYWeightFun,
    const float _r0, const float _r1)
  : prevYWeightFunc(&prevYWeightFun)
  , currentYWeightFunc(&currentYWeightFun)
  , r0(_r0)
  , r1(_r1)
{
}

void RandWeightHandler::weightsReset(
    [[ maybe_unused ]]const size_t ID,
    [[ maybe_unused ]]const size_t numItrs, [[ maybe_unused ]]const size_t nmNodes,
    const float basePrevYWgt, const float baseCurrentYWgt)
{
  prevYWeightFunc->setX0(r0*basePrevYWgt);
  prevYWeightFunc->setX1(r1*basePrevYWgt);
  currentYWeightFunc->setX0(r0*baseCurrentYWgt);
  currentYWeightFunc->setX1(r1*baseCurrentYWgt);
}

void RandWeightHandler::weightsAdjust(
    [[ maybe_unused ]]const size_t ID,
    [[ maybe_unused ]]const size_t iterNum, [[ maybe_unused ]]const size_t nodeNum,
    [[ maybe_unused ]]const float prevY, [[ maybe_unused ]]const float currentY)
{
}

static constexpr int coordIgnoreVal = -666;

GridTentacleLayout::GridTentacleLayout(
    const float x0, const float x1, const size_t xn,
    const float y0, const float y1, const size_t yn,
    const float z)
  : xmin{ x0 }
  , xmax{ x1 }
  , xNum{ xn }
  , ymin{ y0 }
  , ymax{ y1 }
  , yNum{ yn }
  , zConst{ z }
  , xStep{ (x1 - x0)/float(xn - 1) }
  , yStep{ (y1 - y0)/float(yn - 1) }
  , x{ x0 }
  , y{ y0 }
{
}

size_t GridTentacleLayout::getNumPoints() const
{
  return xNum*yNum;
}

void GridTentacleLayout::reset()
{
  x = xmin;
  y = ymin;
}

bool GridTentacleLayout::finished() const
{
  return y < ymax;
}

V3d GridTentacleLayout::getNextPosition()
{
  if (y > ymax) {
    throw std::runtime_error(fmt::format("y out of bounds: {} > {}.", y, ymax));
  }
  const V3d pos = { x, y, zConst };
  x += xStep;
  if (x > xmax) {
    x = xmin;
    y += yStep;
  }
  return pos;
}

TentacleDriver::TentacleDriver(const ColorMaps& cm)
  : iterParamsGroups{}
  , colorMaps{ &cm }
  , currentColorMapGroup{ &ColorMaps::getRandomGroup() }
  , colorizers{}
  , constPrevYWeightFunc()
  , constCurrentYWeightFunc()
  , weightsHandler{ constPrevYWeightFunc, constCurrentYWeightFunc }
  , weightsReset{ nullptr }
  , weightsAdjust{ nullptr }
  , tentacles{}
  , glitchTimer{ glitchIterLength }
  , glitchColorGroup{ vivid::ColorMap::Preset::Magma }
  , iterTimers{ &glitchTimer }
{
  const IterParamsGroup iter1 = {
    { 100, 0.500, 1.0, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 125, 0.600, 2.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
  };
  const IterParamsGroup iter2 = {
    { 125, 0.600, 0.5, { 1.0, -10.0, +10.0,  0.0 }, 40.0 },
    { 150, 0.700, 1.5, { 1.5, -10.0, +10.0, M_PI }, 60.0 },
  };
  const IterParamsGroup iter3 = {
    { 150, 0.700, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 200, 0.900, 2.5, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
  };

  iterParamsGroups = {
    iter1,
    iter2,
    iter3,
  };
}

constexpr double tent2d_xmin = -30.0;
constexpr double tent2d_xmax = +30.0;
constexpr double tent2d_ymin = 0.065736;
constexpr double tent2d_ymax = 10000;

void TentacleDriver::init()
{
  GridTentacleLayout layout{ -100, 100, xRowLen, -100, 100, numXRows, 0 };
  assert(layout.getNumPoints() == numTentacles);
  const ColorMap* specialColorMap = &colorMaps->getRandomColorMap(ColorMaps::getQualitativeMaps());
  const std::vector<size_t> specialColorNodes = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  const ColorMap* headColorMap = &colorMaps->getColorMap("red_black_sky");

  const size_t numInTentacleGroup = numTentacles/iterParamsGroups.size();
  const float tStep = 1.0/float(numInTentacleGroup - 1);
  size_t paramsIndex = 0;
  float t = 0;
  for (size_t i=0; i < numTentacles; i++) {
    const IterParamsGroup paramsGrp = iterParamsGroups.at(paramsIndex);
    if (i % numInTentacleGroup == 0) {
      if (paramsIndex < iterParamsGroups.size()-1) {
        paramsIndex++;
      }
      t = 0;
    }
    const IterationParams params = paramsGrp.getNext(t);
    t += tStep;

    tentacleParams[i] = params;

    std::unique_ptr<TentacleColorMapColorizer> colorizer{
      new TentacleColorMapColorizer{ colorMaps->getRandomColorMap(*currentColorMapGroup), params.numNodes } };
    colorizers.push_back(std::move(colorizer));

    std::unique_ptr<Tentacle2D> tentacle2D{ createNewTentacle2D(i, params) };

    const uint32_t headColor = ColorMap::getRandomColor(*headColorMap);
    const uint32_t headColorLow = ColorMap::getLighterColor(headColor, 50);
    const V3d headPos = layout.getNextPosition();

    Tentacle3D tentacle{ std::move(tentacle2D), *colorizers[colorizers.size()-1], headColor, headColorLow, headPos };

    tentacle.setSpecialColorNodes(*specialColorMap, specialColorNodes);

    tentacles.addTentacle(std::move(tentacle));
  }

  updateNum = 0;
}

TentacleDriver::IterationParams TentacleDriver::IterParamsGroup::getNext(const float t) const
{
  const float prevYWeight = getRandInRange(0.9f, 1.1f)*std::lerp(float(first.prevYWeight), float(last.prevYWeight), t);
  IterationParams params{};
  params.length = size_t(getRandInRange(0.9f, 1.1f)*std::lerp(float(first.length), float(last.length), t));
  params.numNodes = size_t(getRandInRange(0.9f, 1.1f)*std::lerp(float(first.numNodes), float(last.numNodes), t));
  params.prevYWeight = prevYWeight;
  params.iterZeroYValWave = first.iterZeroYValWave;
  params.iterZeroYValWaveFreq = getRandInRange(0.9f, 1.1f)*std::lerp(
                                     float(first.iterZeroYValWaveFreq), float(last.iterZeroYValWaveFreq), t);
  return params;
}

std::unique_ptr<Tentacle2D> TentacleDriver::createNewTentacle2D(const size_t ID, const IterationParams& params)
{
  const size_t tentacleLen = size_t(getRandInRange(0.9f, 1.1f)*float(params.length));
  const double xmin = tent2d_xmax - tentacleLen;

  std::unique_ptr<Tentacle2D> tentacle{ new Tentacle2D{ ID, std::move(createNewTweaker(tentacleLen)) } };

  tentacle->setXDimensions(xmin, tent2d_xmax);
  tentacle->setYDimensions(tent2d_ymin, tent2d_ymax);
  tentacle->setYScale(0.5);

  tentacle->setPrevYWeight(params.prevYWeight);
  tentacle->setCurrentYWeight(1.0 - params.prevYWeight);
  tentacle->setNumNodes(size_t(float(params.numNodes)*getRandInRange(0.9f, 1.1f)));

  tentacle->setDoDamping(true);
  tentacle->setPostProcessing(false);
  tentacle->setDoPrevYWeightAdjust(false);
  tentacle->setDoCurrentYWeightAdjust(false);
//    tentacle->turnOffAllAdjusts();

  return tentacle;
}

std::unique_ptr<TentacleTweaker> TentacleDriver::createNewTweaker(const size_t tentacleLen)
{
  using namespace std::placeholders;

  const double xmin = tent2d_xmax - tentacleLen;
  const double xRiseStart = xmin + 0.25*tentacleLen;
  constexpr double dampStart = 5;
  constexpr double dampMax = 30;

  std::unique_ptr<ExpDampingFunction> dampingFunc{
    new ExpDampingFunction{ 1.0, xRiseStart, dampStart, tent2d_xmax, dampMax }
  };

  //  TentacleTweaker::WeightFunctionsResetter weightsReset =
  //      std::bind(&RandWeightHandler::weightsReset, &weightsHandler, _1, _2, _3, _4);
  //  TentacleTweaker::WeightFunctionsAdjuster weightsAdjust =
  //      std::bind(&RandWeightHandler::weightsAdjust, &weightsHandler, _1, _2, _3, _4);

  TentacleTweaker::WeightFunctionsResetter weightsReset =
      std::bind(&SimpleWeightHandler::weightsReset, &weightsHandler, _1, _2, _3, _4);
  TentacleTweaker::WeightFunctionsAdjuster weightsAdjust =
      std::bind(&SimpleWeightHandler::weightsAdjust, &weightsHandler, _1, _2, _3, _4, _5);

  return std::unique_ptr<TentacleTweaker>{ new TentacleTweaker {
    std::move(dampingFunc),
    std::bind(&TentacleDriver::beforeIter, this, _1, _2, _3, _4),
    &(weightsHandler.getPrevYWeightFunc()),
    &(weightsHandler.getCurrentYWeightFunc()),
    weightsReset,
    weightsAdjust
  }};
}

void TentacleDriver::startIterating()
{
  for (auto& t : tentacles) {
    t.get2DTentacle().startIterating();
  }
}

void TentacleDriver::stopIterating()
{
  for (auto& t : tentacles) {
    t.get2DTentacle().finishIterating();
  }
}

void TentacleDriver::multiplyIterZeroYValWaveFreq(const float val)
{
  for (size_t i=0; i < numTentacles; i++) {
    const float newFreq = val*tentacleParams[i].iterZeroYValWaveFreq;
    tentacleParams[i].iterZeroYValWave.setFrequency(newFreq);
  }
}

void TentacleDriver::setGlitchValues(const float lower, const float upper)
{
  glitchLower = lower;
  glitchUpper = upper;
}

void TentacleDriver::setReverseColorMix(const bool val)
{
  for (auto& t : tentacles) {
    t.setReverseColorMix(val);
  }
}

void TentacleDriver::updateIterTimers()
{
  for (auto t : iterTimers) {
    t->next();
  }
}

void TentacleDriver::checkForTimerEvents()
{
//  logInfo(fmt::format("Update num = {}: checkForTimerEvents", updateNum));

  /**
  if (updateNum % doGlitchEveryNUpdates == 0) {
//    logInfo(fmt::format("Update num = {}: starting glitchTimer.", updateNum));
    glitchTimer.start();
  }
  **/
  /**
  if (updateNum % doDominantColorEveryNUpdates == 0) {
    if (updateNum % (2*doDominantColorEveryNUpdates) == 0) {
      logInfo(fmt::format("Update num = {}: get new dominantColor", updateNum));
      dominantColorGroup = &getRandomColorGroup();
      logInfo(fmt::format("Update num = {}: got new dominantColorGroup", updateNum));
      dominantColor = getRandomColor(*dominantColorGroup);
      logInfo(fmt::format("Update num = {}: new dominantColor = {}", updateNum, dominantColor));
    }
    dominantColor = getEvolvedColor(dominantColor);
  }
  **/
}

void TentacleDriver::beforeIter(
    const size_t ID, const size_t iterNum, const std::vector<double>& xvec, std::vector<double>& yvec)
{
  if (iterNum == 1) {
    for (size_t i=0; i < xvec.size(); i++) {
//      yvec[i] = (*dampingFunc)(xvec[i]);
      yvec[i] = getRandInRange(-10.0f, +10.0f);
    }
  }

  if ((std::fabs(glitchUpper - glitchLower) > 0.001) && (glitchTimer.getCurrentCount() > 0)) {
    for (double& y : yvec) {
      y += getRandInRange(glitchLower, glitchUpper);
    }
    /**
//    logInfo(fmt::format("iter = {} and tentacle {} and resetGlitchTimer.getCurrentCount() = {}.",
//        iterNum, ID, glitchTimer.getCurrentCount()));
    if (glitchTimer.atStart()) {
      for (double& y : yvec) {
        y += getRandInRange(glitchLower, glitchUpper);
      }
//      logInfo(fmt::format("Pushing color for iter = {} and tentacle {}.", iterNum, ID));
      colorizers[ID]->pushColorMap(glitchColorGroup);
    } else if (glitchTimer.getCurrentCount() == 1) {
//      logInfo(fmt::format("Popping color for iter = {} and tentacle {}.", iterNum, ID));
      colorizers[ID]->popColorMap();
    }
    **/
  }

  if (iterNum % changeTentacleColorMapEveryNUpdates == 0) {
    colorizers[ID]->resetColorMap(colorMaps->getRandomColorMap(*currentColorMapGroup));
  }
}

void TentacleDriver::update(const float angle,
    const float distance, const float distance2,
    const uint32_t color, const uint32_t colorLow, Pixel* frontBuff, Pixel* backBuff)
{
  updateNum++;
//  logInfo(fmt::format("Doing update {}.", updateNum));

  if (updateNum % changeCurrentColorMapGroupEveryNUpdates == 0) {
    currentColorMapGroup = &ColorMaps::getRandomGroup();
  }

  updateIterTimers();
  checkForTimerEvents();

  const float iterZeroLerpFactor = 0.9;

  for (size_t i=0; i < numTentacles; i++) {
    Tentacle3D& tentacle = tentacles[i];
    Tentacle2D& tentacle2D = tentacle.get2DTentacle();

    const float iterZeroYVal = tentacleParams[i].iterZeroYValWave.getNext();
    tentacle2D.setIterZeroLerpFactor(iterZeroLerpFactor);
    tentacle2D.setIterZeroYVal(iterZeroYVal);

//    logInfo(fmt::format("Starting iterate {} for tentacle {}.", tentacle2D.getIterNum()+1, tentacle2D.getID()));
    tentacle2D.iterate();

//    logInfo(fmt::format("Update num = {}, tentacle = {}, doing plot with angle = {}, "
//        "distance = {}, distance2 = {}, color = {} and colorLow = {}.",
//        updateNum, tentacle2D.getID(), angle, distance, distance2, color, colorLow));
    plot3D(tentacle, color, colorLow, angle, distance, distance2, frontBuff, backBuff);
  }
}

void TentacleDriver::plot3D(const Tentacle3D& tentacle,
                            const uint32_t dominantColor, const uint32_t dominantColorLow,
                            const float angle, const float distance, const float distance2,
                            Pixel* frontBuff, Pixel* backBuff)
{
  const std::vector<V3d> vertices = tentacle.getVertices();
  const size_t n = vertices.size();

  V3d cam = { 0, 0, 3 }; // TODO ????????????????????????????????
  cam.z += distance2;
  cam.y += 2.0 * sin(-(angle - 0.5*M_PI) / 4.3f);

  const float sina = sin(M_PI - angle);
  const float cosa = cos(M_PI - angle);
  std::vector<V3d> v3{ vertices };
  for (size_t i = 0; i < n; i++) {
    y_rotate_v3d(v3[i], v3[i], sina, cosa);
    translate_v3d(cam, v3[i]);
  }

  std::vector<v2d> v2(n);
  project_v3d_to_v2d(v3, v2, distance);

  for (size_t i=0; i < v2.size()-1; i++) {
    const int ix0 = int(v2[i].x);
    const int ix1 = int(v2[i+1].x);
    const int iy0 = int(v2[i].y);
    const int iy1 = int(v2[i+1].y);

    if ((ix0 == ix1) && (iy0 == iy1)) {
//      logInfo(fmt::format("Skipping draw {}: ix0 = {}, iy0 = {}, ix1 = {}, iy1 = {}, color = {}.",
//          i, ix0, iy0, ix1, iy1, color));
    } else {
//      logInfo(fmt::format("draw_line {}: ix0 = {}, iy0 = {}, ix1 = {}, iy1 = {}, color = {}.",
//          i, ix0, iy0, ix1, iy1, color));
      const auto [color, colorLow] = tentacle.getMixedColors(i, dominantColor, dominantColorLow);
      const std::vector<_PIXEL> colors = { { .val = color }, { .val = colorLow } };

      Pixel* buffs[2] = { frontBuff, backBuff };
      // TODO - Control brightness because of back buff??
      // One buff may be better????? Make lighten more aggressive over whole tentacle??
//      draw_line(frontBuff, ix0, iy0, ix1, iy1, color, 1280, 720);
      draw_line(std::size(buffs), buffs, colors, ix0, iy0, ix1, iy1, 1280, 720);
    }
  }
}

inline void TentacleDriver::y_rotate_v3d(const V3d& vi, V3d& vf, const float sina, const float cosa)                                                           \
{
  const float vi_x = vi.x;
  const float vi_z = vi.z;
  vf.x = vi_x * cosa - vi_z * sina;
  vf.z = vi_x * sina + vi_z * cosa;
  vf.y = vi.y;
}

inline void TentacleDriver::translate_v3d(const V3d& vsrc, V3d& vdest)
{
  vdest.x += vsrc.x;
  vdest.y += vsrc.y;
  vdest.z += vsrc.z;
}

void TentacleDriver::project_v3d_to_v2d(const std::vector<V3d>& v3, std::vector<v2d>& v2, const float distance)
{
  constexpr int width = 1280;
  constexpr int height = 720;

  for (size_t i = 0; i < v3.size(); ++i) {
//    logInfo(fmt::format("project_v3d_to_v2d {}: v3[i].x = {}, v3[i].y = {}, v2[i].z = {}.",
//        i, v3[i].x, v3[i].y, v3[i].z));
    if (!v3[i].ignore && (v3[i].z > 2)) {
      const int Xp = (int)(distance * v3[i].x / v3[i].z);
      const int Yp = (int)(distance * v3[i].y / v3[i].z);
      v2[i].x = Xp + (width >> 1);
      v2[i].y = -Yp + (height >> 1);
//      logInfo(fmt::format("project_v3d_to_v2d {}: Xp = {}, Yp = {}, v2[i].x = {}, v2[i].y = {}.",
//          i, Xp, Yp, v2[i].x, v2[i].y));
    } else {
      v2[i].x = v2[i].y = coordIgnoreVal;
//      logInfo(fmt::format("project_v3d_to_v2d {}: v2[i].x = {}, v2[i].y = {}.", i, v2[i].x, v2[i].y));
    }
  }
}
