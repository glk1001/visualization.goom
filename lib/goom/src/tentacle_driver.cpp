#include "tentacle_driver.h"

#include "goom.h"
#include "drawmethods.h"
#include "tentacles_new.h"
#include "v3d.h"

#include <fmt/format.h>
#include <vivid/vivid.h>

#include <cstdint>
#include <cmath>
#include <functional>
#include <memory>
#include <stack>
#include <stdexcept>
#include <utility>
#include <vector>


TentacleColorGroup::TentacleColorGroup(const ColorGroup& colorGrp, const size_t nNodes)
  : colorGroup{ &colorGrp }
  , colorStack()
  , numNodes{ nNodes }
{
}

void TentacleColorGroup::pushColor(const ColorGroup& col)
{
  colorStack.push(colorGroup);
  colorGroup = &col;
}

void TentacleColorGroup::TentacleColorGroup::popColor()
{
  colorGroup = colorStack.top();
  colorStack.pop();
}

uint32_t TentacleColorGroup::getColor(size_t nodeNum) const
{
  const size_t colorNum = colorGroup->numColors()*nodeNum/numNodes;
  return colorGroup->getColor(colorNum);
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

TentacleDriver::TentacleDriver()
  : iterParamsGroups{}
  , colorGroups{}
  , colorizers{}
  , constPrevYWeightFunc()
  , constCurrentYWeightFunc()
  , weightsHandler{ constPrevYWeightFunc, constCurrentYWeightFunc }
  , weightsReset{ nullptr }
  , weightsAdjust{ nullptr }
  , dampingFunc{ nullptr }
  , tentacles{}
  , tweaker{ nullptr }
  , glitchTimer{ glitchIterLength }
  , glitchColorGroup{ vivid::ColorMap::Preset::Magma }
  , iterTimers{ &glitchTimer }
{
  const IterParamsGroup iter1 = {
    { 200, 0.300, 0.700, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 200, 0.350, 0.650, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 200, 0.400, 0.600, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 200, 0.450, 0.550, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 200, 0.500, 0.500, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 200, 0.550, 0.450, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 200, 0.600, 0.400, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 200, 0.650, 0.350, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 200, 0.700, 0.300, 1.0, { 1.0, -10.0, +10.0, M_PI }, 60.0 },
    { 200, 0.750, 0.250, 1.5, { 1.5, -10.0, +10.0,  0.0 }, 40.0 },
    { 200, 0.800, 0.200, 1.0, { 1.0, -10.0, +10.0, M_PI }, 60.0 },
    { 200, 0.850, 0.150, 1.5, { 1.5, -10.0, +10.0,  0.0 }, 40.0 },
    { 200, 0.900, 0.100, 1.0, { 1.0, -10.0, +10.0, M_PI }, 60.0 },
  };
  const IterParamsGroup iter2 = {
    { 100, 0.300, 0.700, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 100, 0.350, 0.650, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 100, 0.400, 0.600, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 100, 0.450, 0.550, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 100, 0.500, 0.500, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 100, 0.550, 0.450, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 100, 0.600, 0.400, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 100, 0.650, 0.350, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 100, 0.700, 0.300, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 100, 0.750, 0.250, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 100, 0.800, 0.200, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 100, 0.850, 0.150, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 100, 0.900, 0.100, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
  };
  const IterParamsGroup iter3 = {
    { 150, 0.300, 0.700, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 150, 0.350, 0.650, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 150, 0.400, 0.600, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 150, 0.450, 0.550, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 150, 0.500, 0.500, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 150, 0.550, 0.450, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 150, 0.600, 0.400, 1.0, { 1.0, -10.0, +10.0,  0.0 }, 60.0 },
    { 150, 0.650, 0.350, 1.5, { 1.5, -10.0, +10.0, M_PI }, 40.0 },
    { 150, 0.700, 0.300, 1.0, { 1.0, -10.0, +10.0, M_PI }, 60.0 },
    { 150, 0.750, 0.250, 1.5, { 1.5, -10.0, +10.0,  0.0 }, 40.0 },
    { 150, 0.800, 0.200, 1.0, { 1.0, -10.0, +10.0, M_PI }, 60.0 },
    { 150, 0.850, 0.150, 1.5, { 1.5, -10.0, +10.0,  0.0 }, 40.0 },
    { 150, 0.900, 0.100, 1.0, { 1.0, -10.0, +10.0, M_PI }, 60.0 },
  };

  iterParamsGroups = {
    iter1,
    iter2,
    iter3,
  };
}

void TentacleDriver::init()
{
  constexpr double tent2d_xmin = -30.0;
  constexpr double tent2d_xmax = +30.0;
  constexpr double tent2d_ymin = 0.065736;
  constexpr double tent2d_ymax = 10000;

  using namespace std::placeholders;

  init_color_groups(colorGroups);

  TentacleTweaker::WeightFunctionsResetter weightsReset =
      std::bind(&SimpleWeightHandler::weightsReset, &weightsHandler, _1, _2, _3, _4);
  TentacleTweaker::WeightFunctionsAdjuster weightsAdjust =
      std::bind(&SimpleWeightHandler::weightsAdjust, &weightsHandler, _1, _2, _3, _4, _5);

//  RandSequenceFunction randPrevYWeightFunc;
//  RandSequenceFunction randCurrentYWeightFunc;
//  RandWeightHandler weightsHandler { randPrevYWeightFunc, randCurrentYWeightFunc, 0.9, 1.1 };
//  TentacleTweaker::WeightFunctionsResetter weightsReset =
//      std::bind(&RandWeightHandler::weightsReset, &weightsHandler, _1, _2, _3, _4);
//  TentacleTweaker::WeightFunctionsAdjuster weightsAdjust =
//      std::bind(&RandWeightHandler::weightsAdjust, &weightsHandler, _1, _2, _3, _4);

  dampingFunc.reset(new ExpDampingFunction{ 1.0, -10.0, 1.5, tent2d_xmax, 20 });

  tweaker.reset(new TentacleTweaker{
    dampingFunc.get(),
    std::bind(&TentacleDriver::resetYVec, this, _1, _2, _3, _4),
    &(weightsHandler.getPrevYWeightFunc()),
    &(weightsHandler.getCurrentYWeightFunc()),
    weightsReset,
    weightsAdjust
  });

  GridTentacleLayout layout{ -100, 100, xRowLen, -100, 100, mult, 0 };
  assert(layout.getNumPoints() == numTentacles);

  for (size_t i=0; i < numTentacles; i++) {
    std::unique_ptr<Tentacle2D> tentacle{ new Tentacle2D{ i, tweaker.get() } };

    const IterParamsGroup paramsGrp = iterParamsGroups[getRandInRange(0, iterParamsGroups.size())];
    const IterationParams param = paramsGrp[getRandInRange(0, paramsGrp.size())];

    const double xmin = tent2d_xmax - param.length;
    const double xmax = tent2d_xmax;
    tentacle->setXDimensions(xmin, xmax);
    tentacle->setYDimensions(tent2d_ymin, tent2d_ymax);
    tentacle->setYScale(0.5);

    tentacle->setPrevYWeight(param.prevYWeight);
    tentacle->setCurrentYWeight(param.currentYWeight);
    tentacle->setNumNodes(param.numNodes);

    tentacleParams[i] = param;

    tentacle->setDoDamping(true);
    tentacle->setDoPrevYWeightAdjust(false);
    tentacle->setDoCurrentYWeightAdjust(false);
//    tentacle->turnOffAllAdjusts();

    std::unique_ptr<TentacleColorGroup> colorizer{
      new TentacleColorGroup(colorGroups[i % colorGroups.size()], param.numNodes) };
    colorizers.push_back(std::move(colorizer));

    const V3d pos = layout.getNextPosition();
    tentacles.addTentacle(std::move(tentacle), *colorizers[colorizers.size()-1], pos);
  }

  updateNum = 0;
}

void TentacleDriver::init_color_groups(std::vector<ColorGroup>& colorGroups)
{
  static const std::vector<vivid::ColorMap::Preset> cmaps = {
    vivid::ColorMap::Preset::BlueYellow,
    vivid::ColorMap::Preset::CoolWarm,
    vivid::ColorMap::Preset::Hsl,
    vivid::ColorMap::Preset::HslPastel,
    vivid::ColorMap::Preset::Inferno,
    vivid::ColorMap::Preset::Magma,
    vivid::ColorMap::Preset::Plasma,
    vivid::ColorMap::Preset::Rainbow,
    vivid::ColorMap::Preset::Turbo,
    vivid::ColorMap::Preset::Viridis,
    vivid::ColorMap::Preset::Vivid
  };
  for (const auto& cm : cmaps) {
    colorGroups.push_back(ColorGroup(cm));
  }
}

const ColorGroup& TentacleDriver::getRandomColorGroup() const
{
  return colorGroups[getRandInRange(0, colorGroups.size())];
}

uint32_t TentacleDriver::getRandomColor(const ColorGroup& cg) const
{
  return cg.getColor(getRandInRange(0, cg.numColors()));
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

void TentacleDriver::updateIterTimers()
{
  for (auto t : iterTimers) {
    t->next();
  }
}

void TentacleDriver::resetYVec(
    const size_t ID, const size_t iterNum, const std::vector<double>& xvec, std::vector<double>& yvec)
{
  if (iterNum == 1) {
    for (size_t i=0; i < xvec.size(); i++) {
      yvec[i] = (*dampingFunc)(xvec[i]);
    }
  }
  if (glitchTimer.getCurrentCount() > 0) {
//    logInfo(fmt::format("iter = {} and tentacle {} and resetGlitchTimer.getCurrentCount() = {}.",
//        iterNum, ID, glitchTimer.getCurrentCount()));
    if (glitchTimer.atStart()) {
      constexpr float lower = -1.5;
      constexpr float upper = +1.5;
      for (double& y : yvec) {
        y += getRandInRange(lower, upper);
      }
//      logInfo(fmt::format("Pushing color for iter = {} and tentacle {}.", iterNum, ID));
      colorizers[ID]->pushColor(glitchColorGroup);
    } else if (glitchTimer.getCurrentCount() == 1) {
//      logInfo(fmt::format("Popping color for iter = {} and tentacle {}.", iterNum, ID));
      colorizers[ID]->popColor();
    }
  }
}

void TentacleDriver::checkForTimerEvents()
{
//  logInfo(fmt::format("Update num = {}: checkForTimerEvents", updateNum));
  if (updateNum % doGlitchEveryNUpdates == 0) {
//    logInfo(fmt::format("Update num = {}: starting glitchTimer.", updateNum));
    glitchTimer.start();
  }
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

void TentacleDriver::update(const float angle,
    const float distance, const float distance2,
    const uint32_t color, const uint32_t colorLow, Pixel* frontBuff, Pixel* backBuff)
{
  updateNum++;
//  logInfo(fmt::format("Doing update {}.", updateNum));

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
      const size_t numNodes = tentacle.get2DTentacle().getNumNodes();
      const uint32_t tentacleColor = tentacle.getColor(i);
      const uint32_t color = getColorMix(i, numNodes, tentacleColor, dominantColor);
      const uint32_t colorLow = getColorMix(i, numNodes, tentacleColor, dominantColorLow);
      const std::vector<_PIXEL> colors = { { .val = color }, { .val = colorLow } };

      Pixel* buffs[2] = { frontBuff, backBuff };
      // TODO - Control brightness because of back buff??
      // One buff may be better????? Make lighten more agressive over whole tentacle??
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

inline uint32_t TentacleDriver::getColorMix(const size_t nodeNum, const size_t numNodes,
                                            const uint32_t segmentColor, const uint32_t dominantColor)
{
  const auto tFac = [&](const size_t nodeNum) -> float {
    return float(nodeNum+1)/float(numNodes);
  };

  const float t = tFac(nodeNum);
  const uint32_t col = colorMix(dominantColor, segmentColor, t);

  return col;
}

inline uint32_t TentacleDriver::colorMix(const uint32_t col1, const uint32_t col2, const float t)
{
  const vivid::rgb_t c1 = vivid::rgb::fromRgb32(col1);
  const vivid::rgb_t c2 = vivid::rgb::fromRgb32(col2);
  return vivid::lerpHsl(c1, c2, t).rgb32();
}
