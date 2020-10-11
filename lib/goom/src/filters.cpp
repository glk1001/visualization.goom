// --- CHUI EN TRAIN DE SUPPRIMER LES EXTERN RESOLX ET C_RESOLY ---

/* filter.c version 0.7
 * contient les filtres applicable a un buffer
 * creation : 01/10/2000
 *  -ajout de sinFilter()
 *  -ajout de zoomFilter()
 *  -copie de zoomFilter() en zoomFilterRGB(), gerant les 3 couleurs
 *  -optimisation de sinFilter (utilisant une table de sin)
 *	-asm
 *	-optimisation de la procedure de generation du buffer de transformation
 *		la vitesse est maintenant comprise dans [0..128] au lieu de [0..100]
 */

#include "filters.h"

#include "goom_config.h"
#include "goom_fx.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_testing.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
#include "goomutils/mathutils.h"
// #undef NO_LOGGING
#include "goomutils/logging.h"
#include "v3d.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace goom
{

using namespace goom::utils;

class FilterStats
{
public:
  FilterStats() noexcept = default;

  void reset();
  void log(const StatsLogValueFunc) const;
  void doZoomVector();
  void doZoomVectorCrystalBallMode();
  void doZoomVectorAmuletteMode();
  void doZoomVectorWaveMode();
  void doZoomVectorScrunchMode();
  void doZoomVectorSpeedwayMode();
  void doZoomVectorDefaultMode();
  void doZoomVectorNoisify();
  void doZoomVectorNoiseFactor();
  void doZoomVectorHypercosEffect();
  void doZoomVectorHPlaneEffect();
  void doZoomVectorVPlaneEffect();
  void doMakeZoomBufferStripe();
  void doGetMixedColor();
  void doGetMixedColorZero();
  void doGetBlockyMixedColor();
  void doCZoom();
  void doGenerateWaterFXHorizontalBuffer();
  void doZoomFilterFastRGB();
  void doZoomFilterFastRGBChangeConfig();
  void doZoomFilterFastRGBInterlaceStartEqualMinus1_1();
  void doZoomFilterFastRGBInterlaceStartEqualMinus1_2();
  void doZoomFilterFastRGBSwitchIncrNotZero();
  void doZoomFilterFastRGBSwitchIncrNotEqual1();

  void setLastGeneralSpeed(const float val);
  void setLastPrevX(const uint32_t val);
  void setLastPrevY(const uint32_t val);
  void setLastInterlaceStart(const int val);
  void setLastBuffratio(const int val);

private:
  uint64_t numZoomVectors = 0;
  uint64_t numZoomVectorCrystalBallMode = 0;
  uint64_t numZoomVectorAmuletteMode = 0;
  uint64_t numZoomVectorWaveMode = 0;
  uint64_t numZoomVectorScrunchMode = 0;
  uint64_t numZoomVectorSpeedwayMode = 0;
  uint64_t numZoomVectorDefaultMode = 0;
  uint64_t numZoomVectorNoisify = 0;
  uint64_t numChangeZoomVectorNoiseFactor = 0;
  uint64_t numZoomVectorHypercosEffect = 0;
  uint64_t numZoomVectorHPlaneEffect = 0;
  uint64_t numZoomVectorVPlaneEffect = 0;
  uint64_t numMakeZoomBufferStripe = 0;
  uint64_t numGetMixedColor = 0;
  uint64_t numGetMixedColorZero = 0;
  uint64_t numGetBlockyMixedColor = 0;
  uint64_t numCZoom = 0;
  uint64_t numGenerateWaterFXHorizontalBuffer = 0;
  uint64_t numZoomFilterFastRGB = 0;
  uint64_t numZoomFilterFastRGBChangeConfig = 0;
  uint64_t numZoomFilterFastRGBInterlaceStartEqualMinus1_1 = 0;
  uint64_t numZoomFilterFastRGBInterlaceStartEqualMinus1_2 = 0;
  uint64_t numZoomFilterFastRGBSwitchIncrNotZero = 0;
  uint64_t numZoomFilterFastRGBSwitchIncrNotEqual1 = 0;

  float lastGeneralSpeed;
  uint32_t lastPrevX = 0;
  uint32_t lastPrevY = 0;
  int lastInterlaceStart = 0;
  int lastBuffratio = 0;
};

void FilterStats::log(const StatsLogValueFunc logVal) const
{
  const constexpr char* module = "Filter";

  logVal(module, "lastGeneralSpeed", lastGeneralSpeed);
  logVal(module, "lastPrevX", lastPrevX);
  logVal(module, "lastPrevY", lastPrevY);
  logVal(module, "lastInterlaceStart", lastInterlaceStart);
  logVal(module, "lastBuffratio", lastBuffratio);

  logVal(module, "numZoomVectors", numZoomVectors);
  logVal(module, "numZoomVectorCrystalBallMode", numZoomVectorCrystalBallMode);
  logVal(module, "numZoomVectorAmuletteMode", numZoomVectorAmuletteMode);
  logVal(module, "numZoomVectorWaveMode", numZoomVectorWaveMode);
  logVal(module, "numZoomVectorScrunchMode", numZoomVectorScrunchMode);
  logVal(module, "numZoomVectorSpeedwayMode", numZoomVectorSpeedwayMode);
  logVal(module, "numZoomVectorDefaultMode", numZoomVectorDefaultMode);
  logVal(module, "numZoomVectorNoisify", numZoomVectorNoisify);
  logVal(module, "numChangeZoomVectorNoiseFactor", numChangeZoomVectorNoiseFactor);
  logVal(module, "numZoomVectorHypercosEffect", numZoomVectorHypercosEffect);
  logVal(module, "numZoomVectorHPlaneEffect", numZoomVectorHPlaneEffect);
  logVal(module, "numZoomVectorVPlaneEffect", numZoomVectorVPlaneEffect);
  logVal(module, "numMakeZoomBufferStripe", numMakeZoomBufferStripe);
  logVal(module, "numGetMixedColor", numGetMixedColor);
  logVal(module, "numGetMixedColorZero", numGetMixedColorZero);
  logVal(module, "numGetBlockyMixedColor", numGetBlockyMixedColor);
  logVal(module, "numCZoom", numCZoom);
  logVal(module, "numGenerateWaterFXHorizontalBuffer", numGenerateWaterFXHorizontalBuffer);
  logVal(module, "numZoomFilterFastRGBChangeConfig", numZoomFilterFastRGBChangeConfig);
  logVal(module, "numZoomFilterFastRGBInterlaceStartEqualMinus1_1",
         numZoomFilterFastRGBInterlaceStartEqualMinus1_1);
  logVal(module, "numZoomFilterFastRGBInterlaceStartEqualMinus1_2",
         numZoomFilterFastRGBInterlaceStartEqualMinus1_2);
  logVal(module, "numZoomFilterFastRGBSwitchIncrNotZero", numZoomFilterFastRGBSwitchIncrNotZero);
  logVal(module, "numZoomFilterFastRGBSwitchIncrNotEqual1",
         numZoomFilterFastRGBSwitchIncrNotEqual1);
}

void FilterStats::reset()
{
  numZoomVectors = 0;
  numZoomVectorCrystalBallMode = 0;
  numZoomVectorAmuletteMode = 0;
  numZoomVectorWaveMode = 0;
  numZoomVectorScrunchMode = 0;
  numZoomVectorSpeedwayMode = 0;
  numZoomVectorDefaultMode = 0;
  numZoomVectorNoisify = 0;
  numChangeZoomVectorNoiseFactor = 0;
  numZoomVectorHypercosEffect = 0;
  numZoomVectorHPlaneEffect = 0;
  numZoomVectorVPlaneEffect = 0;
  numMakeZoomBufferStripe = 0;
  numGetMixedColor = 0;
  numGetMixedColorZero = 0;
  numGetBlockyMixedColor = 0;
  numCZoom = 0;
  numGenerateWaterFXHorizontalBuffer = 0;
  numZoomFilterFastRGB = 0;
  numZoomFilterFastRGBChangeConfig = 0;
  numZoomFilterFastRGBInterlaceStartEqualMinus1_1 = 0;
  numZoomFilterFastRGBInterlaceStartEqualMinus1_2 = 0;
  numZoomFilterFastRGBSwitchIncrNotZero = 0;
  numZoomFilterFastRGBSwitchIncrNotEqual1 = 0;
}

inline void FilterStats::doZoomVector()
{
  numZoomVectors++;
}

inline void FilterStats::doZoomVectorCrystalBallMode()
{
  numZoomVectorCrystalBallMode++;
}

inline void FilterStats::doZoomVectorAmuletteMode()
{
  numZoomVectorAmuletteMode++;
}

inline void FilterStats::doZoomVectorWaveMode()
{
  numZoomVectorWaveMode++;
}

inline void FilterStats::doZoomVectorScrunchMode()
{
  numZoomVectorScrunchMode++;
}

inline void FilterStats::doZoomVectorSpeedwayMode()
{
  numZoomVectorSpeedwayMode++;
}

inline void FilterStats::doZoomVectorDefaultMode()
{
  numZoomVectorDefaultMode++;
}

inline void FilterStats::doZoomVectorNoisify()
{
  numZoomVectorNoisify++;
}

inline void FilterStats::doZoomVectorNoiseFactor()
{
  numChangeZoomVectorNoiseFactor++;
}

inline void FilterStats::doZoomVectorHypercosEffect()
{
  numZoomVectorHypercosEffect++;
}

inline void FilterStats::doZoomVectorHPlaneEffect()
{
  numZoomVectorHPlaneEffect++;
}

inline void FilterStats::doZoomVectorVPlaneEffect()
{
  numZoomVectorVPlaneEffect++;
}

inline void FilterStats::doMakeZoomBufferStripe()
{
  numMakeZoomBufferStripe++;
}

inline void FilterStats::doGetMixedColor()
{
  numGetMixedColor++;
}

inline void FilterStats::doGetMixedColorZero()
{
  numGetMixedColorZero++;
}

inline void FilterStats::doGetBlockyMixedColor()
{
  numGetBlockyMixedColor++;
}

inline void FilterStats::doCZoom()
{
  numCZoom++;
}

inline void FilterStats::doGenerateWaterFXHorizontalBuffer()
{
  numGenerateWaterFXHorizontalBuffer++;
}

inline void FilterStats::doZoomFilterFastRGB()
{
  numZoomFilterFastRGB++;
}

inline void FilterStats::doZoomFilterFastRGBChangeConfig()
{
  numZoomFilterFastRGBChangeConfig++;
}

inline void FilterStats::doZoomFilterFastRGBInterlaceStartEqualMinus1_1()
{
  numZoomFilterFastRGBInterlaceStartEqualMinus1_1++;
}

inline void FilterStats::doZoomFilterFastRGBInterlaceStartEqualMinus1_2()
{
  numZoomFilterFastRGBInterlaceStartEqualMinus1_2++;
}

inline void FilterStats::doZoomFilterFastRGBSwitchIncrNotEqual1()
{
  numZoomFilterFastRGBSwitchIncrNotEqual1++;
}

inline void FilterStats::doZoomFilterFastRGBSwitchIncrNotZero()
{
  numZoomFilterFastRGBSwitchIncrNotZero++;
}

void FilterStats::setLastGeneralSpeed(const float val)
{
  lastGeneralSpeed = val;
}

void FilterStats::setLastPrevX(const uint32_t val)
{
  lastPrevX = val;
}

void FilterStats::setLastPrevY(const uint32_t val)
{
  lastPrevY = val;
}

void FilterStats::setLastInterlaceStart(const int val)
{
  lastInterlaceStart = val;
}

void FilterStats::setLastBuffratio(const int val)
{
  lastBuffratio = val;
}

static FilterStats stats;

// For noise amplitude, take the reciprocal of these.
constexpr float noiseMin = 70;
constexpr float noiseMax = 120;

constexpr size_t BUFFPOINTNB = 16;
constexpr float BUFFPOINTNBF = static_cast<float>(BUFFPOINTNB);
constexpr int BUFFPOINTMASK = 0xffff;

constexpr uint32_t sqrtperte = 16;
// faire : a % sqrtperte <=> a & pertemask
constexpr uint32_t perteMask = 0xf;
// faire : a / sqrtperte <=> a >> PERTEDEC
constexpr uint32_t perteDec = 4;
using CoeffArray = union
{
  struct
  {
    uint8_t c1;
    uint8_t c2;
    uint8_t c3;
    uint8_t c4;
  } vals;
  uint32_t intVal = 0;
};

static void generatePrecalCoef(uint32_t precalCoef[BUFFPOINTNB][BUFFPOINTNB]);

// TODO repeated fields in here
struct FilterDataWrapper
{
  ZoomFilterData filterData;
  float generalSpeed;

  PluginParam enabled_bp;
  PluginParameters params;

  uint32_t* coeffs;
  uint32_t* freecoeffs;

  int* brutS;
  int* freebrutS; // source
  int* brutD;
  int* freebrutD; // dest
  int* brutT;
  int* freebrutT; // temp (en cours de generation)

  uint32_t prevX;
  uint32_t prevY;

  bool mustInitBuffers;
  int interlaceStart;

  // modif by jeko : fixedpoint : buffration = (16:16) (donc 0<=buffration<=2^16)
  int buffratio;
  int* firedec;

  // modif d'optim by Jeko : precalcul des 4 coefs resultant des 2 pos
  uint32_t precalCoef[BUFFPOINTNB][BUFFPOINTNB];
};

// pure c version of the zoom filter
static void c_zoom(Pixel* expix1, Pixel* expix2, const FilterDataWrapper*);

inline v2g zoomVector(PluginInfo* goomInfo, const float x, const float y)
{
  stats.doZoomVector();

  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(goomInfo->zoomFilter_fx.fx_data);

  /* sx = (x < 0.0f) ? -1.0f : 1.0f;
     sy = (y < 0.0f) ? -1.0f : 1.0f;
   */
  float coefVitesse = (1.0f + data->generalSpeed) / 50.0f;

  // Effects

  // Centralized FX
  switch (data->filterData.mode)
  {
    case ZoomFilterMode::crystalBallMode:
    {
      stats.doZoomVectorCrystalBallMode();
      coefVitesse -= data->filterData.crystalBallAmplitude * (sq_distance(x, y) - 0.3f);
      break;
    }
    case ZoomFilterMode::amuletteMode:
    {
      stats.doZoomVectorAmuletteMode();
      coefVitesse += data->filterData.amuletteAmplitude * sq_distance(x, y);
      break;
    }
    case ZoomFilterMode::waveMode:
    {
      stats.doZoomVectorWaveMode();
      const float angle = sq_distance(x, y) * data->filterData.waveFreqFactor;
      float periodicPart;
      switch (data->filterData.waveEffectType)
      {
        case ZoomFilterData::WaveEffect::waveSinEffect:
          periodicPart = sin(angle);
          break;
        case ZoomFilterData::WaveEffect::waveCosEffect:
          periodicPart = cos(angle);
          break;
        case ZoomFilterData::WaveEffect::waveSinCosEffect:
          periodicPart = sin(angle) + cos(angle);
          break;
        default:
          throw std::logic_error("Unknown WaveEffect enum");
      }
      coefVitesse += data->filterData.waveAmplitude * periodicPart;
      break;
    }
    case ZoomFilterMode::scrunchMode:
    {
      stats.doZoomVectorScrunchMode();
      coefVitesse += data->filterData.scrunchAmplitude * sq_distance(x, y);
      break;
    }
      //case ZoomFilterMode::HYPERCOS1_MODE:
      //break;
      //case ZoomFilterMode::HYPERCOS2_MODE:
      //break;
      //case ZoomFilterMode::YONLY_MODE:
      //break;
    case ZoomFilterMode::speedwayMode:
    {
      stats.doZoomVectorSpeedwayMode();
      coefVitesse *= data->filterData.speedwayAmplitude * y;
      break;
    }
    default:
      stats.doZoomVectorDefaultMode();
      break;
  }

  coefVitesse = std::clamp(coefVitesse, -2.01f, +2.01f);

  float vx = coefVitesse * x;
  float vy = coefVitesse * y;

  /* Amulette 2 */
  // vx = X * tan(dist);
  // vy = Y * tan(dist);

  /* Rotate */
  //vx = (X+Y)*0.1;
  //vy = (Y-X)*0.1;

  // Effects adds-on
  /* Noise */
  if (data->filterData.noisify)
  {
    stats.doZoomVectorNoisify();
    if (data->filterData.noiseFactor > 0.01)
    {
      stats.doZoomVectorNoiseFactor();
      //    const float xAmp = 1.0/getRandInRange(50.0f, 200.0f);
      //    const float yAmp = 1.0/getRandInRange(50.0f, 200.0f);
      const float amp = data->filterData.noiseFactor / getRandInRange(noiseMin, noiseMax);
      data->filterData.noiseFactor *= 0.999;
      vx += amp * (getRandInRange(0.0f, 1.0f) - 0.5f);
      vy += amp * (getRandInRange(0.0f, 1.0f) - 0.5f);
    }
  }

  // Hypercos
  if (data->filterData.hypercosEffect)
  {
    stats.doZoomVectorHypercosEffect();
    vx += data->filterData.hypercosAmplitude * sin(data->filterData.hypercosFreq * y);
    vy += data->filterData.hypercosAmplitude * sin(data->filterData.hypercosFreq * x);
  }

  // H Plane
  if (data->filterData.hPlaneEffect)
  {
    stats.doZoomVectorHPlaneEffect();

    vx += y * data->filterData.hPlaneEffectAmplitude *
          static_cast<float>(data->filterData.hPlaneEffect);
  }

  // V Plane
  if (data->filterData.vPlaneEffect)
  {
    stats.doZoomVectorVPlaneEffect();
    vy += x * data->filterData.vPlaneEffectAmplitude *
          static_cast<float>(data->filterData.vPlaneEffect);
  }

  /* TODO : Water Mode */
  //    if (data->waveEffect)

  return v2g{vx, vy};
}

/*
 * Makes a stripe of a transform buffer (brutT)
 *
 * The transform is (in order) :
 * Translation (-data->middleX, -data->middleY)
 * Homothetie (Center : 0,0   Coeff : 2/data->prevX)
 */
static void makeZoomBufferStripe(PluginInfo* goomInfo, const uint32_t INTERLACE_INCR)
{
  stats.doMakeZoomBufferStripe();

  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(goomInfo->zoomFilter_fx.fx_data);

  // Ratio from pixmap to normalized coordinates
  const float ratio = 2.0f / static_cast<float>(data->prevX);

  // Ratio from normalized to virtual pixmap coordinates
  const float inv_ratio = BUFFPOINTNBF / ratio;
  const float min = ratio / BUFFPOINTNBF;

  // Y position of the pixel to compute in normalized coordinates
  float Y =
      ratio * static_cast<float>(data->interlaceStart - static_cast<int>(data->filterData.middleY));

  // Where (vertically) to stop generating the buffer stripe
  uint32_t maxEnd = data->prevY;
  if (maxEnd > static_cast<uint32_t>(data->interlaceStart + static_cast<int>(INTERLACE_INCR)))
  {
    maxEnd = static_cast<uint32_t>(data->interlaceStart + static_cast<int>(INTERLACE_INCR));
  }

  // Position of the pixel to compute in pixmap coordinates
  uint32_t y;
  for (y = static_cast<uint32_t>(data->interlaceStart); (y < data->prevY) && (y < maxEnd); y++)
  {
    uint32_t premul_y_prevX = y * data->prevX * 2;
    float X = -static_cast<float>(data->filterData.middleX) * ratio;
    for (uint32_t x = 0; x < data->prevX; x++)
    {
      v2g vector = zoomVector(goomInfo, X, Y);
      // Finish and avoid null displacement
      if (fabs(vector.x) < min)
      {
        vector.x = (vector.x < 0.0f) ? -min : min;
      }
      if (fabs(vector.y) < min)
      {
        vector.y = (vector.y < 0.0f) ? -min : min;
      }

      data->brutT[premul_y_prevX] = static_cast<int>((X - vector.x) * inv_ratio) +
                                    static_cast<int>(data->filterData.middleX * BUFFPOINTNB);
      data->brutT[premul_y_prevX + 1] = static_cast<int>((Y - vector.y) * inv_ratio) +
                                        static_cast<int>(data->filterData.middleY * BUFFPOINTNB);
      premul_y_prevX += 2;
      X += ratio;
    }
    Y += ratio;
  }
  data->interlaceStart += static_cast<int>(INTERLACE_INCR);
  if (y >= data->prevY - 1)
  {
    data->interlaceStart = -1;
  }
}

static Pixel getMixedColor(const CoeffArray& coeffs,
                           const Pixel& col1,
                           const Pixel& col2,
                           const Pixel& col3,
                           const Pixel& col4)
{
  stats.doGetMixedColor();

  if (coeffs.intVal == 0)
  {
    stats.doGetMixedColorZero();
    return Pixel{.val = 0};
  }

  const uint32_t c1 = coeffs.vals.c1;
  const uint32_t c2 = coeffs.vals.c2;
  const uint32_t c3 = coeffs.vals.c3;
  const uint32_t c4 = coeffs.vals.c4;

  uint32_t r =
      col1.channels.r * c1 + col2.channels.r * c2 + col3.channels.r * c3 + col4.channels.r * c4;
  if (r > 5)
  {
    r -= 5;
  }
  r >>= 8;

  uint32_t g =
      col1.channels.g * c1 + col2.channels.g * c2 + col3.channels.g * c3 + col4.channels.g * c4;
  if (g > 5)
  {
    g -= 5;
  }
  g >>= 8;

  uint32_t b =
      col1.channels.b * c1 + col2.channels.b * c2 + col3.channels.b * c3 + col4.channels.b * c4;
  if (b > 5)
  {
    b -= 5;
  }
  b >>= 8;

  return Pixel{.channels = {.r = static_cast<uint8_t>(r),
                            .g = static_cast<uint8_t>(g),
                            .b = static_cast<uint8_t>(b),
                            .a = 0xff}};
}

inline Pixel getBlockyMixedColor(const CoeffArray& coeffs,
                                 const Pixel& col1,
                                 const Pixel& col2,
                                 const Pixel& col3,
                                 const Pixel& col4)
{
  stats.doGetBlockyMixedColor();

  // Changing the color order gives a strange blocky, wavy look.
  // The order col1, col3, col2, col1 gave a black tear - no so good.
  return getMixedColor(coeffs, col1, col3, col2, col4);
}

inline Pixel getPixelColor(const Pixel* buffer, const uint32_t pos)
{
  return buffer[pos];
}

inline void setPixelColor(Pixel* buffer, const uint32_t pos, const Pixel& p)
{
  buffer[pos] = p;
}

static void c_zoom(Pixel* expix1, Pixel* expix2, const FilterDataWrapper* data)
{
  stats.doCZoom();

  const uint32_t prevX = data->prevX;
  const uint32_t prevY = data->prevY;
  const int* brutS = data->brutS;
  const int* brutD = data->brutD;
  const int buffratio = data->buffratio;

  const uint32_t ax = (prevX - 1) << perteDec;
  const uint32_t ay = (prevY - 1) << perteDec;

  const uint32_t bufsize = prevX * prevY * 2;
  const uint32_t bufwidth = prevX;

  expix1[0].val = 0;
  expix1[prevX - 1].val = 0;
  expix1[prevX * prevY - 1].val = 0;
  expix1[prevX * prevY - prevX].val = 0;

  uint32_t myPos2;
  for (uint32_t myPos = 0; myPos < bufsize; myPos += 2)
  {
    myPos2 = myPos + 1;

    const int brutSmypos = brutS[myPos];
    const uint32_t px = static_cast<uint32_t>(
        brutSmypos + (((brutD[myPos] - brutSmypos) * buffratio) >> BUFFPOINTNB));

    const int brutSmypos2 = brutS[myPos2];
    const uint32_t py = static_cast<uint32_t>(
        brutSmypos2 + (((brutD[myPos2] - brutSmypos2) * buffratio) >> BUFFPOINTNB));

    const uint32_t pix2Pos = static_cast<uint32_t>(myPos >> 1);

    if ((px >= ax) || (py >= ay))
    {
      setPixelColor(expix2, pix2Pos, Pixel{.val = 0});
    }
    else
    {
      // coeff en modulo 15
      const CoeffArray coeffs{.intVal = data->precalCoef[px & perteMask][py & perteMask]};

      const uint32_t pix1Pos = (px >> perteDec) + prevX * (py >> perteDec);
      const Pixel col1 = getPixelColor(expix1, pix1Pos);
      const Pixel col2 = getPixelColor(expix1, pix1Pos + 1);
      const Pixel col3 = getPixelColor(expix1, pix1Pos + bufwidth);
      const Pixel col4 = getPixelColor(expix1, pix1Pos + bufwidth + 1);

      const Pixel newColor = data->filterData.blockyWavy
                                 ? getBlockyMixedColor(coeffs, col1, col2, col3, col4)
                                 : getMixedColor(coeffs, col1, col2, col3, col4);
      //      const Pixel newColor = getMixedColor(coeffs, col1, col2, col3, col4);

      setPixelColor(expix2, pix2Pos, newColor);
    }
  }
}

static void generateWaterFXHorizontalBuffer(FilterDataWrapper* data)
{
  stats.doGenerateWaterFXHorizontalBuffer();

  int decc = getRandInRange(-4, +4);
  int spdc = getRandInRange(-4, +4);
  int accel = getRandInRange(-4, +4);

  for (size_t loopv = data->prevY; loopv != 0;)
  {
    loopv--;
    data->firedec[loopv] = decc;
    decc += spdc / 10;
    spdc += getRandInRange(-2, +3);

    if (decc > 4)
    {
      spdc -= 1;
    }
    if (decc < -4)
    {
      spdc += 1;
    }

    if (spdc > 30)
    {
      spdc = spdc - static_cast<int>(getNRand(3)) + accel / 10;
    }
    if (spdc < -30)
    {
      spdc = spdc + static_cast<int>(getNRand(3)) + accel / 10;
    }

    if (decc > 8 && spdc > 1)
    {
      spdc -= getRandInRange(-2, +1);
    }
    if (decc < -8 && spdc < -1)
    {
      spdc += static_cast<int>(getNRand(3)) + 2;
    }
    if (decc > 8 || decc < -8)
    {
      decc = decc * 8 / 9;
    }

    accel += getRandInRange(-1, +2);
    if (accel > 20)
    {
      accel -= 2;
    }
    if (accel < -20)
    {
      accel += 2;
    }
  }
}

static void initBuffers(PluginInfo* goomInfo, const uint32_t resx, const uint32_t resy)
{
  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(goomInfo->zoomFilter_fx.fx_data);

  if ((data->prevX != resx) || (data->prevY != resy))
  {
    data->prevX = resx;
    data->prevY = resy;

    if (data->brutS)
    {
      delete[] data->freebrutS;
    }
    data->brutS = nullptr;
    if (data->brutD)
    {
      delete[] data->freebrutD;
    }
    data->brutD = nullptr;
    if (data->brutT)
    {
      delete[] data->freebrutT;
    }
    data->brutT = nullptr;

    data->filterData.middleX = resx / 2;
    data->filterData.middleY = resy / 2;
    data->mustInitBuffers = 1;
    if (data->firedec)
    {
      delete[] data->firedec;
    }
    data->firedec = nullptr;
  }

  data->mustInitBuffers = 0;
  data->freebrutS = new int[resx * resy * 2 + 128]{};
  data->brutS = (int*)((1 + (uintptr_t((data->freebrutS))) / 128) * 128);

  data->freebrutD = new int[resx * resy * 2 + 128]{};
  data->brutD = (int*)((1 + (uintptr_t((data->freebrutD))) / 128) * 128);

  data->freebrutT = new int[resx * resy * 2 + 128]{};
  data->brutT = (int32_t*)((1 + (uintptr_t((data->freebrutT))) / 128) * 128);

  data->buffratio = 0;

  data->firedec = new int[data->prevY];
  generateWaterFXHorizontalBuffer(data);

  data->interlaceStart = 0;
  makeZoomBufferStripe(goomInfo, resy);

  /* Copy the data from temp to dest and source */
  memcpy(data->brutS, data->brutT, resx * resy * 2 * sizeof(int));
  memcpy(data->brutD, data->brutT, resx * resy * 2 * sizeof(int));
}

/**
 * Main work for the dynamic displacement map.
 *
 * Reads data from pix1, write to pix2.
 *
 * Useful datas for this FX are stored in ZoomFilterData.
 *
 * If you think that this is a strange function name, let me say that a long time ago,
 *  there has been a slow version and a gray-level only one. Then came these function,
 *  fast and workin in RGB colorspace ! nice but it only was applying a zoom to the image.
 *  So that is why you have this name, for the nostalgy of the first days of goom
 *  when it was just a tiny program writen in Turbo Pascal on my i486...
 */
void zoomFilterFastRGB(PluginInfo* goomInfo,
                       Pixel* pix1,
                       Pixel* pix2,
                       ZoomFilterData* zf,
                       const uint16_t resx,
                       const uint16_t resy,
                       const int switchIncr,
                       const float switchMult)
{
  stats.doZoomFilterFastRGB();

  logDebug("resx = {}, resy = {}, switchIncr = {}, switchMult = {:.2}", resx, resy, switchIncr,
           switchMult);

  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(goomInfo->zoomFilter_fx.fx_data);

  if (!BVAL(data->enabled_bp))
  {
    return;
  }

  // changement de taille
  logDebug("data->prevX = {}, data->prevY = {}", data->prevX, data->prevY);
  if (data->mustInitBuffers)
  {
    logDebug("Calling initBuffers");
    initBuffers(goomInfo, resx, resy);
  }

  if (data->interlaceStart != -2)
  {
    zf = nullptr;
  }

  // changement de config
  if (zf)
  {
    stats.doZoomFilterFastRGBChangeConfig();
    data->filterData = *zf;
    data->generalSpeed = static_cast<float>(zf->vitesse - 128) / 128.0f;
    if (data->filterData.reverse)
    {
      data->generalSpeed = -data->generalSpeed;
    }
    data->interlaceStart = 0;
  }

  // generation du buffer de trans
  logDebug("data->interlaceStart = {}", data->interlaceStart);
  if (data->interlaceStart == -1)
  {
    stats.doZoomFilterFastRGBInterlaceStartEqualMinus1_1();
    /* sauvegarde de l'etat actuel dans la nouvelle source
     * TODO: write that in MMX (has been done in previous version, but did not follow
     * some new fonctionnalities) */
    const uint32_t y = 2 * data->prevX * data->prevY;
    for (uint32_t x = 0; x < y; x += 2)
    {
      int brutSmypos = data->brutS[x];
      const uint32_t x2 = x + 1;

      data->brutS[x] =
          brutSmypos + (((data->brutD[x] - brutSmypos) * data->buffratio) >> BUFFPOINTNB);
      brutSmypos = data->brutS[x2];
      data->brutS[x2] =
          brutSmypos + (((data->brutD[x2] - brutSmypos) * data->buffratio) >> BUFFPOINTNB);
    }
    data->buffratio = 0;
  }

  logDebug("data->interlaceStart = {}", data->interlaceStart);
  if (data->interlaceStart == -1)
  {
    stats.doZoomFilterFastRGBInterlaceStartEqualMinus1_2();
    int* tmp = data->brutD;
    data->brutD = data->brutT;
    data->brutT = tmp;
    tmp = data->freebrutD;
    data->freebrutD = data->freebrutT;
    data->freebrutT = tmp;
    data->interlaceStart = -2;
  }

  logDebug("data->interlaceStart = {}", data->interlaceStart);
  if (data->interlaceStart >= 0)
  {
    // creation de la nouvelle destination
    makeZoomBufferStripe(goomInfo, resy / 16);
  }

  if (switchIncr != 0)
  {
    stats.doZoomFilterFastRGBSwitchIncrNotZero();

    data->buffratio += switchIncr;
    if (data->buffratio > BUFFPOINTMASK)
    {
      data->buffratio = BUFFPOINTMASK;
    }
  }

  // Equal was interesting but not correct!?
  //  if (std::fabs(1.0f - switchMult) < 0.0001)
  if (std::fabs(1.0f - switchMult) > 0.0001)
  {
    stats.doZoomFilterFastRGBSwitchIncrNotEqual1();

    data->buffratio = static_cast<int>(static_cast<float>(BUFFPOINTMASK) * (1.0f - switchMult) +
                                       static_cast<float>(data->buffratio) * switchMult);
  }

  c_zoom(pix1, pix2, data);
}

static void generatePrecalCoef(uint32_t precalCoef[16][16])
{
  for (uint32_t coefh = 0; coefh < 16; coefh++)
  {
    for (uint32_t coefv = 0; coefv < 16; coefv++)
    {
      const uint32_t diffcoeffh = sqrtperte - coefh;
      const uint32_t diffcoeffv = sqrtperte - coefv;

      if (!(coefh || coefv))
      {
        precalCoef[coefh][coefv] = 255;
      }
      else
      {
        uint32_t i1 = diffcoeffh * diffcoeffv;
        uint32_t i2 = coefh * diffcoeffv;
        uint32_t i3 = diffcoeffh * coefv;
        uint32_t i4 = coefh * coefv;

        // TODO: faire mieux...
        if (i1)
        {
          i1--;
        }
        if (i2)
        {
          i2--;
        }
        if (i3)
        {
          i3--;
        }
        if (i4)
        {
          i4--;
        }

        precalCoef[coefh][coefv] = CoeffArray{
            .vals{
                .c1 = static_cast<uint8_t>(i1),
                .c2 = static_cast<uint8_t>(i2),
                .c3 = static_cast<uint8_t>(i3),
                .c4 = static_cast<uint8_t>(i4),
            }}.intVal;
      }
    }
  }
}

/* VisualFX Wrapper */

static const char* const vfxname = "ZoomFilter";

static void zoomFilterSave(VisualFX* _this, const PluginInfo*, const char* file)
{
  FILE* f = fopen(file, "w");

  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(_this->fx_data);

  save_int_setting(f, vfxname, "data->prevX", static_cast<int>(data->prevX));
  save_int_setting(f, vfxname, "data->prevY", static_cast<int>(data->prevY));
  save_float_setting(f, vfxname, "data->generalSpeed", data->generalSpeed);
  save_int_setting(f, vfxname, "data->reverse", data->filterData.reverse);
  save_int_setting(f, vfxname, "data->mode", static_cast<int>(data->filterData.mode));
  save_int_setting(f, vfxname, "data->waveEffect", data->filterData.waveEffect);
  save_int_setting(f, vfxname, "data->hypercosEffect", data->filterData.hypercosEffect);
  save_int_setting(f, vfxname, "data->vPlaneEffect", data->filterData.vPlaneEffect);
  save_int_setting(f, vfxname, "data->hPlaneEffect", data->filterData.hPlaneEffect);
  save_int_setting(f, vfxname, "data->noisify", static_cast<int>(data->filterData.noisify));
  save_int_setting(f, vfxname, "data->middleX", static_cast<int>(data->filterData.middleX));
  save_int_setting(f, vfxname, "data->middleY", static_cast<int>(data->filterData.middleY));

  save_int_setting(f, vfxname, "data->mustInitBuffers", data->mustInitBuffers);
  save_int_setting(f, vfxname, "data->interlaceStart", data->interlaceStart);

  save_int_setting(f, vfxname, "data->buffratio", data->buffratio);
  //    int precalCoef[BUFFPOINTNB][BUFFPOINTNB];

  fclose(f);
}

static void zoomFilterRestore(VisualFX* _this, PluginInfo*, const char* file)
{
  FILE* f = fopen(file, "r");
  if (f == NULL)
  {
    exit(EXIT_FAILURE);
  }

  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(_this->fx_data);

  data->prevX = static_cast<uint32_t>(get_int_setting(f, vfxname, "data->prevX"));
  data->prevY = static_cast<uint32_t>(get_int_setting(f, vfxname, "data->prevY"));
  data->generalSpeed = get_float_setting(f, vfxname, "data->generalSpeed");
  data->filterData.reverse = get_int_setting(f, vfxname, "data->reverse");
  data->filterData.mode = static_cast<ZoomFilterMode>(get_int_setting(f, vfxname, "data->mode"));
  data->filterData.waveEffect = get_int_setting(f, vfxname, "data->waveEffect");
  data->filterData.hypercosEffect = get_int_setting(f, vfxname, "data->hypercosEffect");
  data->filterData.vPlaneEffect = get_int_setting(f, vfxname, "data->vPlaneEffect");
  data->filterData.hPlaneEffect = get_int_setting(f, vfxname, "data->hPlaneEffect");
  data->filterData.noisify = get_int_setting(f, vfxname, "data->noisify");
  data->filterData.middleX = static_cast<uint32_t>(get_int_setting(f, vfxname, "data->middleX"));
  data->filterData.middleY = static_cast<uint32_t>(get_int_setting(f, vfxname, "data->middleY"));

  data->mustInitBuffers = get_int_setting(f, vfxname, "data->mustInitBuffers");
  data->interlaceStart = get_int_setting(f, vfxname, "data->interlaceStart");

  data->buffratio = get_int_setting(f, vfxname, "data->buffratio");

  fclose(f);
}

static void zoomFilterVisualFXWrapper_init(VisualFX* _this, PluginInfo* info)
{
  FilterDataWrapper* data = new FilterDataWrapper{};

  data->coeffs = 0;
  data->freecoeffs = 0;
  data->brutS = 0;
  data->freebrutS = 0;
  data->brutD = 0;
  data->freebrutD = 0;
  data->brutT = 0;
  data->freebrutT = 0;
  data->prevX = 0;
  data->prevY = 0;

  data->mustInitBuffers = 1;
  data->interlaceStart = -2;

  data->generalSpeed = 0.0f;
  data->filterData = info->update.zoomFilterData;

  /** modif by jeko : fixedpoint : buffration = (16:16) (donc 0<=buffration<=2^16) */
  data->buffratio = 0;
  data->firedec = nullptr;

  data->enabled_bp = secure_b_param("Enabled", 1);

  data->params = plugin_parameters("ZoomFilter", 1);
  data->params.params[0] = &data->enabled_bp;

  _this->params = &data->params;
  _this->fx_data = data;

  /** modif d'optim by Jeko : precalcul des 4 coefs resultant des 2 pos */
  generatePrecalCoef(data->precalCoef);

  initBuffers(info, info->screen.width, info->screen.height);
}

static void zoomFilterVisualFXWrapper_free(VisualFX* _this)
{
  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(_this->fx_data);

  delete[] data->freebrutS;
  delete[] data->freebrutD;
  delete[] data->freebrutT;
  delete[] data->firedec;
  free(data->params.params);

  delete data;
}

static void zoomFilterVisualFXWrapper_apply(VisualFX*, Pixel*, Pixel*, PluginInfo*)
{
}

VisualFX zoomFilterVisualFXWrapper_create()
{
  VisualFX fx;
  fx.init = zoomFilterVisualFXWrapper_init;
  fx.free = zoomFilterVisualFXWrapper_free;
  fx.apply = zoomFilterVisualFXWrapper_apply;
  fx.save = zoomFilterSave;
  fx.restore = zoomFilterRestore;
  return fx;
}

void filter_log_stats(VisualFX* _this, const StatsLogValueFunc logVal)
{
  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(_this->fx_data);
  stats.setLastGeneralSpeed(data->generalSpeed);
  stats.setLastPrevX(data->prevX);
  stats.setLastPrevY(data->prevY);
  stats.setLastInterlaceStart(data->interlaceStart);
  stats.setLastBuffratio(data->buffratio);
  stats.log(logVal);
}

} // namespace goom
