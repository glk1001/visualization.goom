#include "tentacle_driver.h"

#include "drawmethods.h"
#include "goom.h"
#include "tentacles_new.h"
#include "v3d.h"

#include "goomutils/colormap.h"
#include "goomutils/logging.h"

#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <stack>
#include <stdexcept>
#include <utility>
#include <vector>


TentacleDriver::TentacleDriver(const ColorMaps& cm, const int screenW, const int screenH)
  : iterParamsGroups{}
  , screenWidth{ screenW }
  , screenHeight{ screenH }
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

constexpr double tent2d_xmax = +30.0;
constexpr double tent2d_ymin = 0.065736;
constexpr double tent2d_ymax = 10000;

void TentacleDriver::init()
{
  constexpr V3d initialHeadPos = { 0, 0, 0 };
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
    Tentacle3D tentacle{ std::move(tentacle2D),
      *colorizers[colorizers.size()-1], headColor, headColorLow, initialHeadPos };

    tentacle.setSpecialColorNodes(*specialColorMap, specialColorNodes);

    tentacles.addTentacle(std::move(tentacle));
  }

  const CirclesTentacleLayout layout{  10, 80, { 34, 24, 16, 6, 4 }, 0 };
//  const GridTentacleLayout layout{ -100, 100, xRowLen, -100, 100, numXRows, 0 };
  updateLayout(layout);

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

void TentacleDriver::updateLayout(const TentacleLayout& layout)
{
  assert(layout.getNumPoints() == numTentacles);

  for (size_t i=0; i < numTentacles; i++) {
    tentacles[i].setHead(layout.getPosition(i));
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
//  logInfo(stdnew::format("Update num = {}: checkForTimerEvents", updateNum));

  /**
  if (updateNum % doGlitchEveryNUpdates == 0) {
//    logInfo(stdnew::format("Update num = {}: starting glitchTimer.", updateNum));
    glitchTimer.start();
  }
  **/
  /**
  if (updateNum % doDominantColorEveryNUpdates == 0) {
    if (updateNum % (2*doDominantColorEveryNUpdates) == 0) {
      logInfo(stdnew::format("Update num = {}: get new dominantColor", updateNum));
      dominantColorGroup = &getRandomColorGroup();
      logInfo(stdnew::format("Update num = {}: got new dominantColorGroup", updateNum));
      dominantColor = getRandomColor(*dominantColorGroup);
      logInfo(stdnew::format("Update num = {}: new dominantColor = {}", updateNum, dominantColor));
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
//    logInfo(stdnew::format("iter = {} and tentacle {} and resetGlitchTimer.getCurrentCount() = {}.",
//        iterNum, ID, glitchTimer.getCurrentCount()));
    if (glitchTimer.atStart()) {
      for (double& y : yvec) {
        y += getRandInRange(glitchLower, glitchUpper);
      }
//      logInfo(stdnew::format("Pushing color for iter = {} and tentacle {}.", iterNum, ID));
      colorizers[ID]->pushColorMap(glitchColorGroup);
    } else if (glitchTimer.getCurrentCount() == 1) {
//      logInfo(stdnew::format("Popping color for iter = {} and tentacle {}.", iterNum, ID));
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
//  logInfo(stdnew::format("Doing update {}.", updateNum));

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

//    logInfo(stdnew::format("Starting iterate {} for tentacle {}.", tentacle2D.getIterNum()+1, tentacle2D.getID()));
    tentacle2D.iterate();

//    logInfo(stdnew::format("Update num = {}, tentacle = {}, doing plot with angle = {}, "
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
//      logInfo(stdnew::format("Skipping draw {}: ix0 = {}, iy0 = {}, ix1 = {}, iy1 = {}, color = {}.",
//          i, ix0, iy0, ix1, iy1, color));
    } else {
//      logInfo(stdnew::format("draw_line {}: ix0 = {}, iy0 = {}, ix1 = {}, iy1 = {}, color = {}.",
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
  constexpr int coordIgnoreVal = -666;

  for (size_t i = 0; i < v3.size(); ++i) {
//    logInfo(stdnew::format("project_v3d_to_v2d {}: v3[i].x = {}, v3[i].y = {}, v2[i].z = {}.",
//        i, v3[i].x, v3[i].y, v3[i].z));
    if (!v3[i].ignore && (v3[i].z > 2)) {
      const int Xp = (int)(distance * v3[i].x / v3[i].z);
      const int Yp = (int)(distance * v3[i].y / v3[i].z);
      v2[i].x = Xp + (screenWidth >> 1);
      v2[i].y = -Yp + (screenHeight >> 1);
//      logInfo(stdnew::format("project_v3d_to_v2d {}: Xp = {}, Yp = {}, v2[i].x = {}, v2[i].y = {}.",
//          i, Xp, Yp, v2[i].x, v2[i].y));
    } else {
      v2[i].x = v2[i].y = coordIgnoreVal;
//      logInfo(stdnew::format("project_v3d_to_v2d {}: v2[i].x = {}, v2[i].y = {}.", i, v2[i].x, v2[i].y));
    }
  }
}

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
{
}

size_t GridTentacleLayout::getNumPoints() const
{
  return xNum*yNum;
}

V3d GridTentacleLayout::getPosition(size_t elementNum) const
{
  const size_t row = elementNum/xNum;
  if (row > yNum) {
    throw std::runtime_error(stdnew::format("row out of bounds: {} > {}, {} elementNum = {}.", row, yNum, elementNum));
  }
  const size_t col = elementNum % xNum;

  const float x = xmin + float(col)*xStep;
  const float y = ymin + float(row)*yStep;
  const V3d pos = { x, y, zConst };

  return pos;
}

std::vector<size_t> CirclesTentacleLayout::getCircleSamples(const size_t numCircles, const size_t totalPoints)
{
  std::vector<size_t> circleSamples(numCircles);

  return circleSamples;
}

CirclesTentacleLayout::CirclesTentacleLayout(
    const float radiusMin, const float radiusMax, const std::vector<size_t>& numCircleSamples, const float zConst)
  : points{}
{
  const size_t numCircles = numCircleSamples.size();
  if (numCircles < 2) {
    std::runtime_error(stdnew::format("There must be >= 2 circle sample numbers not {}.", numCircles));
  }
  for (const auto numSample : numCircleSamples) {
    if (numSample % 2 != 0) {
      std::runtime_error(stdnew::format("Circle sample num must be even not {}.", numSample));
    }
  }

  const auto logLastPoint = [&](size_t i, const float r, const float angle)
  {
    const size_t el = points.size() - 1;
    logInfo("  sample {:3}: angle = {:+6.2f}, cos(angle) = {:+6.2f}, r = {:+6.2f},"\
            " pt[{:3}] = ({:+6.2f}, {:+6.2f}, {:+6.2f})",
            i, angle, cos(angle), r, el, points.at(el).x, points.at(el).y, points.at(el).z);
  };
  const auto getSamplePoints = [&](
      const float radius, const size_t numSample,
      const float angleStart, const float angleFinish)
  {
    const float angleStep = (angleFinish - angleStart)/float(numSample - 1);
    float angle = angleStart;
    for (size_t i=0; i < numSample; i++) {
      const float x = float(radius*cos(angle));
      const float y = float(radius*sin(angle));
      const V3d point = { x, y, zConst };
      points.push_back(point);
      logLastPoint(i, radius, angle);
      angle += angleStep;
    };
  };

  const float angleLeftStart = +0.5*M_PI + 0.4;
  const float angleLeftFinish = +1.5*M_PI - 0.4;
  const float angleRightStart = -0.5*M_PI + 0.4;
  const float angleRightFinish = +0.5*M_PI - 0.4;

  const float radiusStep = (radiusMax - radiusMin)/float(numCircles - 1);
  float r = radiusMax;
  for (const auto numSample : numCircleSamples) {
    logInfo("Circle with {} samples:", numSample);

    getSamplePoints(r, numSample/2, angleLeftStart, angleLeftFinish);
    getSamplePoints(r, numSample/2, angleRightStart, angleRightFinish);

    r -= radiusStep;
  }
}

size_t CirclesTentacleLayout::getNumPoints() const
{
  return points.size();
}

V3d CirclesTentacleLayout::getPosition(size_t elementNum) const
{
  return points.at(elementNum);
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
