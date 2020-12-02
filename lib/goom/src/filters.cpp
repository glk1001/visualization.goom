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
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_testing.h"
#include "goom_visual_fx.h"
#include "goomutils/colorutils.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
//#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "goomutils/parallel_utils.h"
#include "v3d.h"

#include <algorithm>
#include <array>
#undef NDEBUG
#include <cassert>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cmath>
#include <cstdint>
#include <execution>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

CEREAL_REGISTER_TYPE(goom::ZoomFilterFx);
CEREAL_REGISTER_POLYMORPHIC_RELATION(goom::VisualFx, goom::ZoomFilterFx);

static constexpr bool serializeBuffers = false;

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
  void doGetBlockyMixedColor();
  void doCZoom();
  void doGenerateWaterFXHorizontalBuffer();
  void doZoomFilterFastRGB();
  void doZoomFilterFastRGBChangeConfig();
  void doZoomFilterFastRGBInterlaceStartEqualMinus1_1();
  void doZoomFilterFastRGBInterlaceStartEqualMinus1_2();
  void doZoomFilterFastRGBSwitchIncrNotZero();
  void doZoomFilterFastRGBSwitchIncrNotEqual1();
  void doCZoomOutOfRange();
  void coeffVitesseBelowMin();
  void coeffVitesseAboveMax();

  void setLastGeneralSpeed(const float val);
  void setLastPrevX(const uint32_t val);
  void setLastPrevY(const uint32_t val);
  void setLastInterlaceStart(const int val);
  void setLastTranDiffFactor(const int val);

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
  uint64_t numGetBlockyMixedColor = 0;
  uint64_t numCZoom = 0;
  uint64_t numGenerateWaterFXHorizontalBuffer = 0;
  uint64_t numZoomFilterFastRGB = 0;
  uint64_t numZoomFilterFastRGBChangeConfig = 0;
  uint64_t numZoomFilterFastRGBInterlaceStartEqualMinus1_1 = 0;
  uint64_t numZoomFilterFastRGBInterlaceStartEqualMinus1_2 = 0;
  uint64_t numZoomFilterFastRGBSwitchIncrNotZero = 0;
  uint64_t numZoomFilterFastRGBSwitchIncrNotEqual1 = 0;
  uint64_t numCZoomOutOfRange = 0;
  uint64_t numCoeffVitesseBelowMin = 0;
  uint64_t numCoeffVitesseAboveMax = 0;

  float lastGeneralSpeed = -1000;
  uint32_t lastPrevX = 0;
  uint32_t lastPrevY = 0;
  int32_t lastInterlaceStart = -1000;
  int32_t lastTranDiffFactor = -1000;
};

void FilterStats::log(const StatsLogValueFunc logVal) const
{
  const constexpr char* module = "Filter";

  logVal(module, "lastGeneralSpeed", lastGeneralSpeed);
  logVal(module, "lastPrevX", lastPrevX);
  logVal(module, "lastPrevY", lastPrevY);
  logVal(module, "lastInterlaceStart", lastInterlaceStart);
  logVal(module, "lastTranDiffFactor", lastTranDiffFactor);

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
  logVal(module, "numCZoomOutOfRange", numCZoomOutOfRange);
  logVal(module, "numCoeffVitesseBelowMin", numCoeffVitesseBelowMin);
  logVal(module, "numCoeffVitesseAboveMax", numCoeffVitesseAboveMax);
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
  numGetBlockyMixedColor = 0;
  numCZoom = 0;
  numGenerateWaterFXHorizontalBuffer = 0;
  numZoomFilterFastRGB = 0;
  numZoomFilterFastRGBChangeConfig = 0;
  numZoomFilterFastRGBInterlaceStartEqualMinus1_1 = 0;
  numZoomFilterFastRGBInterlaceStartEqualMinus1_2 = 0;
  numZoomFilterFastRGBSwitchIncrNotZero = 0;
  numZoomFilterFastRGBSwitchIncrNotEqual1 = 0;
  numCZoomOutOfRange = 0;
  numCoeffVitesseBelowMin = 0;
  numCoeffVitesseAboveMax = 0;
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

inline void FilterStats::doCZoomOutOfRange()
{
  numCZoomOutOfRange++;
}

inline void FilterStats::coeffVitesseBelowMin()
{
  numCoeffVitesseBelowMin++;
}

void FilterStats::coeffVitesseAboveMax()
{
  numCoeffVitesseAboveMax++;
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

void FilterStats::setLastInterlaceStart(const int32_t val)
{
  lastInterlaceStart = val;
}

void FilterStats::setLastTranDiffFactor(const int32_t val)
{
  lastTranDiffFactor = val;
}

// For noise amplitude, take the reciprocal of these.
constexpr float noiseMin = 70;
constexpr float noiseMax = 120;

constexpr int32_t buffPointNum = 16;
constexpr float buffPointNumFlt = static_cast<float>(buffPointNum);
constexpr int32_t buffPointMask = 0xffff;

constexpr uint32_t sqrtperte = 16;
// faire : a % sqrtperte <=> a & pertemask
constexpr uint32_t perteMask = 0xf;
// faire : a / sqrtperte <=> a >> PERTEDEC
constexpr uint32_t perteDec = 4;

static constexpr size_t numCoeffs = 4;
using CoeffArray = union
{
  uint8_t c[numCoeffs];
  uint32_t intVal = 0;
};
using PixelArray = std::array<Pixel, numCoeffs>;

constexpr float minCoefVitesse = -2.01;
constexpr float maxCoefVitesse = +2.01;

class ZoomFilterFx::ZoomFilterImpl
{
public:
  ZoomFilterImpl() noexcept;
  explicit ZoomFilterImpl(Parallel&, const std::shared_ptr<const PluginInfo>&) noexcept;
  ~ZoomFilterImpl() noexcept;
  ZoomFilterImpl(const ZoomFilterImpl&) = delete;
  ZoomFilterImpl& operator=(const ZoomFilterImpl&) = delete;

  void setBuffSettings(const FXBuffSettings&);

  void zoomFilterFastRGB(const PixelBuffer& pix1,
                         PixelBuffer& pix2,
                         const ZoomFilterData* zf,
                         const int32_t switchIncr,
                         const float switchMult);

  void log(const StatsLogValueFunc&) const;

  bool operator==(const ZoomFilterImpl&) const;

private:
  uint32_t screenWidth = 0;
  uint32_t screenHeight = 0;
  uint32_t bufferSize = 0;
  float ratioPixmapToNormalizedCoord = 0;
  float ratioNormalizedCoordToPixmap = 0;
  float minNormCoordVal = 0;
  ZoomFilterData filterData{};
  FXBuffSettings buffSettings{};

  Parallel* parallel = nullptr;

  float toNormalizedCoord(const int32_t pixmapCoord);
  int32_t toPixmapCoord(const float normalizedCoord);

  void incNormalizedCoord(float& normalizedCoord);

  mutable FilterStats stats{};

  float generalSpeed = 0;
  int32_t interlaceStart = 0;

  std::vector<int32_t> freeTranXSrce{}; // source
  std::vector<int32_t> freeTranYSrce{}; // source
  int32_t* tranXSrce = nullptr;
  int32_t* tranYSrce = nullptr;
  std::vector<int32_t> freeTranXDest{}; // dest
  std::vector<int32_t> freeTranYDest{}; // dest
  int32_t* tranXDest = nullptr;
  int32_t* tranYDest = nullptr;
  std::vector<int32_t> freeTranXTemp{}; // temp (en cours de generation)
  std::vector<int32_t> freeTranYTemp{}; // temp (en cours de generation)
  int32_t* tranXTemp = nullptr;
  int32_t* tranYTemp = nullptr;

  // modification by jeko : fixedpoint : tranDiffFactor = (16:16) (0 <= tranDiffFactor <= 2^16)
  int32_t tranDiffFactor = 0;
  std::vector<int32_t> firedec{};

  // modif d'optim by Jeko : precalcul des 4 coefs resultant des 2 pos
  uint32_t precalcCoeffs[buffPointNum][buffPointNum];

  void init();
  void initBuffers();

  void makeZoomBufferStripe(const uint32_t interlaceIncrement);
  void c_zoom(const PixelBuffer& srceBuff, PixelBuffer& destBuff);
  void generateWaterFXHorizontalBuffer();
  v2g getZoomVector(const float normX, const float normY);
  static void generatePrecalCoef(uint32_t precalcCoeffs[16][16]);
  Pixel getMixedColor(const CoeffArray& coeffs, const PixelArray& colors) const;
  Pixel getBlockyMixedColor(const CoeffArray& coeffs, const PixelArray& colors) const;

  friend class cereal::access;
  template<class Archive>
  void save(Archive&) const;
  template<class Archive>
  void load(Archive&);
};

/**
bool ZoomFilterData::operator==(const ZoomFilterData& f) const
{
  return mode == f.mode &&
         vitesse == f.vitesse &&
         middleX == f.middleX && ), CEREAL_NVP(middleY),
         CEREAL_NVP(reverse), CEREAL_NVP(hPlaneEffect), CEREAL_NVP(vPlaneEffect),
         CEREAL_NVP(waveEffect), CEREAL_NVP(hypercosEffect), CEREAL_NVP(noisify),
         CEREAL_NVP(noiseFactor), CEREAL_NVP(blockyWavy), CEREAL_NVP(waveFreqFactor),
         CEREAL_NVP(waveAmplitude), CEREAL_NVP(waveEffectType), CEREAL_NVP(scrunchAmplitude),
         CEREAL_NVP(speedwayAmplitude), CEREAL_NVP(amuletteAmplitude), CEREAL_NVP(crystalBallAmplitude),
         CEREAL_NVP(hypercosFreq), CEREAL_NVP(hypercosAmplitude), CEREAL_NVP(hPlaneEffectAmplitude),
         CEREAL_NVP(vPlaneEffectAmplitude));
}
**/

template<class Archive>
void ZoomFilterData::serialize(Archive& ar)
{
  ar(CEREAL_NVP(mode), CEREAL_NVP(vitesse), CEREAL_NVP(middleX), CEREAL_NVP(middleY),
     CEREAL_NVP(reverse), CEREAL_NVP(hPlaneEffect), CEREAL_NVP(vPlaneEffect),
     CEREAL_NVP(waveEffect), CEREAL_NVP(hypercosEffect), CEREAL_NVP(noisify),
     CEREAL_NVP(noiseFactor), CEREAL_NVP(blockyWavy), CEREAL_NVP(waveFreqFactor),
     CEREAL_NVP(waveAmplitude), CEREAL_NVP(waveEffectType), CEREAL_NVP(scrunchAmplitude),
     CEREAL_NVP(speedwayAmplitude), CEREAL_NVP(amuletteAmplitude), CEREAL_NVP(crystalBallAmplitude),
     CEREAL_NVP(hypercosFreq), CEREAL_NVP(hypercosAmplitude), CEREAL_NVP(hPlaneEffectAmplitude),
     CEREAL_NVP(vPlaneEffectAmplitude));
}

template<class Archive>
void ZoomFilterFx::serialize(Archive& ar)
{
  ar(CEREAL_NVP(enabled), CEREAL_NVP(fxImpl));
}

// Need to explicitly instantiate template functions for serialization.
template void ZoomFilterFx::serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);
template void ZoomFilterFx::serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template void ZoomFilterFx::ZoomFilterImpl::save<cereal::JSONOutputArchive>(
    cereal::JSONOutputArchive&) const;
template void ZoomFilterFx::ZoomFilterImpl::load<cereal::JSONInputArchive>(
    cereal::JSONInputArchive&);

template<class Archive>
void ZoomFilterFx::ZoomFilterImpl::save(Archive& ar) const
{
  ar(CEREAL_NVP(screenWidth), CEREAL_NVP(screenHeight), CEREAL_NVP(bufferSize),
     CEREAL_NVP(ratioPixmapToNormalizedCoord), CEREAL_NVP(ratioNormalizedCoordToPixmap),
     CEREAL_NVP(minNormCoordVal), CEREAL_NVP(filterData), CEREAL_NVP(buffSettings),
     CEREAL_NVP(generalSpeed), CEREAL_NVP(interlaceStart), CEREAL_NVP(tranDiffFactor));

  if (serializeBuffers)
  {
    ar(CEREAL_NVP(freeTranXSrce), CEREAL_NVP(freeTranYSrce), CEREAL_NVP(freeTranXDest),
       CEREAL_NVP(freeTranYDest), CEREAL_NVP(freeTranXTemp), CEREAL_NVP(freeTranYTemp),
       CEREAL_NVP(firedec));
  }
}

template<class Archive>
void ZoomFilterFx::ZoomFilterImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(screenWidth), CEREAL_NVP(screenHeight), CEREAL_NVP(bufferSize),
     CEREAL_NVP(ratioPixmapToNormalizedCoord), CEREAL_NVP(ratioNormalizedCoordToPixmap),
     CEREAL_NVP(minNormCoordVal), CEREAL_NVP(filterData), CEREAL_NVP(buffSettings),
     CEREAL_NVP(generalSpeed), CEREAL_NVP(interlaceStart), CEREAL_NVP(tranDiffFactor));

  if (serializeBuffers)
  {
    ar(CEREAL_NVP(freeTranXSrce), CEREAL_NVP(freeTranYSrce), CEREAL_NVP(freeTranXDest),
       CEREAL_NVP(freeTranYDest), CEREAL_NVP(freeTranXTemp), CEREAL_NVP(freeTranYTemp),
       CEREAL_NVP(firedec));
  }
  else
  {
    freeTranXSrce.resize(bufferSize + 128);
    freeTranYSrce.resize(bufferSize + 128);
    freeTranXDest.resize(bufferSize + 128);
    freeTranYDest.resize(bufferSize + 128);
    freeTranXTemp.resize(bufferSize + 128);
    freeTranYTemp.resize(bufferSize + 128);
    firedec.resize(screenHeight);
  }

  initBuffers();
}

bool ZoomFilterFx::ZoomFilterImpl::operator==(const ZoomFilterImpl& f) const
{
  bool result = screenWidth == f.screenWidth && screenHeight == f.screenHeight &&
                bufferSize == f.bufferSize &&
                ratioPixmapToNormalizedCoord == f.ratioPixmapToNormalizedCoord &&
                ratioNormalizedCoordToPixmap == f.ratioNormalizedCoordToPixmap &&
                minNormCoordVal == f.minNormCoordVal && filterData == f.filterData &&
                buffSettings == f.buffSettings && generalSpeed == f.generalSpeed &&
                interlaceStart == f.interlaceStart && tranDiffFactor == f.tranDiffFactor;

  if (serializeBuffers)
  {
    result = result && freeTranXSrce == f.freeTranXSrce && freeTranYSrce == f.freeTranYSrce &&
             freeTranXDest == f.freeTranXDest && freeTranYDest == f.freeTranYDest &&
             freeTranXTemp == f.freeTranXTemp && freeTranYTemp == f.freeTranYTemp &&
             firedec == f.firedec;
  }

  if (!result)
  {
    logInfo("result == {}", result);
    logInfo("screenWidth = {}, f.screenWidth = {}", screenWidth, f.screenWidth);
    logInfo("screenHeight = {}, f.screenHeight = {}", screenHeight, f.screenHeight);
    logInfo("bufferSize = {}, f.bufferSize = {}", bufferSize, f.bufferSize);
    logInfo("ratioPixmapToNormalizedCoord = {}, f.ratioPixmapToNormalizedCoord = {}",
            ratioPixmapToNormalizedCoord, f.ratioPixmapToNormalizedCoord);
    logInfo("ratioNormalizedCoordToPixmap = {}, f.ratioNormalizedCoordToPixmap = {}",
            ratioNormalizedCoordToPixmap, f.ratioNormalizedCoordToPixmap);
    logInfo("minNormCoordVal = {}, f.minNormCoordVal = {}", minNormCoordVal, f.minNormCoordVal);
    logInfo("buffSettings.allowOverexposed = {}, f.buffSettings.allowOverexposed = {}",
            buffSettings.allowOverexposed, f.buffSettings.allowOverexposed);
    logInfo("buffSettings.buffIntensity = {}, f.buffSettings.buffIntensity = {}",
            buffSettings.buffIntensity, f.buffSettings.buffIntensity);
    logInfo("generalSpeed = {}, f.generalSpeed = {}", generalSpeed, f.generalSpeed);
    logInfo("interlaceStart = {}, f.interlaceStart = {}", interlaceStart, f.interlaceStart);
    if (serializeBuffers)
    {
      logInfo("freeTranXSrce == f.freeTranXSrce = {}", freeTranXSrce == f.freeTranXSrce);
      logInfo("freeTranYSrce == f.freeTranYSrce = {}", freeTranYSrce == f.freeTranYSrce);
      logInfo("freeTranXDest == f.freeTranXDest = {}", freeTranXDest == f.freeTranXDest);
      logInfo("freeTranYDest == f.freeTranYDest = {}", freeTranYDest == f.freeTranYDest);
      logInfo("freeTranXTemp == f.freeTranXTemp = {}", freeTranXTemp == f.freeTranXTemp);
      logInfo("freeTranYTemp == f.freeTranYTemp = {}", freeTranYTemp == f.freeTranYTemp);
      logInfo("firedec == f.firedec = {}", firedec == f.firedec);
    }
  }
  return result;
}

ZoomFilterFx::ZoomFilterImpl::ZoomFilterImpl() noexcept
{
}

ZoomFilterFx::ZoomFilterImpl::ZoomFilterImpl(
    Parallel& p, const std::shared_ptr<const PluginInfo>& goomInfo) noexcept
  : screenWidth{goomInfo->getScreenInfo().width},
    screenHeight{goomInfo->getScreenInfo().height},
    bufferSize{goomInfo->getScreenInfo().size},
    ratioPixmapToNormalizedCoord{2.0F / static_cast<float>(screenWidth)},
    ratioNormalizedCoordToPixmap{1.0F / ratioPixmapToNormalizedCoord},
    minNormCoordVal{ratioPixmapToNormalizedCoord / buffPointNumFlt},
    parallel{&p},
    freeTranXSrce(bufferSize + 128),
    freeTranYSrce(bufferSize + 128),
    freeTranXDest(bufferSize + 128),
    freeTranYDest(bufferSize + 128),
    freeTranXTemp(bufferSize + 128),
    freeTranYTemp(bufferSize + 128),
    firedec(screenHeight)
{
  init();
}

ZoomFilterFx::ZoomFilterImpl::~ZoomFilterImpl() noexcept
{
}

void ZoomFilterFx::ZoomFilterImpl::init()
{
  initBuffers();

  filterData.middleX = screenWidth / 2;
  filterData.middleY = screenHeight / 2;

  generatePrecalCoef(precalcCoeffs);
  generateWaterFXHorizontalBuffer();
  makeZoomBufferStripe(screenHeight);

  // Copy the data from temp to dest and source
  memcpy(tranXSrce, tranXTemp, bufferSize * sizeof(int32_t));
  memcpy(tranYSrce, tranYTemp, bufferSize * sizeof(int32_t));
  memcpy(tranXDest, tranXTemp, bufferSize * sizeof(int32_t));
  memcpy(tranYDest, tranYTemp, bufferSize * sizeof(int32_t));
}

void ZoomFilterFx::ZoomFilterImpl::initBuffers()
{
  tranXSrce = (int32_t*)((1 + (uintptr_t((freeTranXSrce.data()))) / 128) * 128);
  tranYSrce = (int32_t*)((1 + (uintptr_t((freeTranYSrce.data()))) / 128) * 128);

  tranXDest = (int32_t*)((1 + (uintptr_t((freeTranXDest.data()))) / 128) * 128);
  tranYDest = (int32_t*)((1 + (uintptr_t((freeTranYDest.data()))) / 128) * 128);

  tranXTemp = (int32_t*)((1 + (uintptr_t((freeTranXTemp.data()))) / 128) * 128);
  tranYTemp = (int32_t*)((1 + (uintptr_t((freeTranYTemp.data()))) / 128) * 128);
}

void ZoomFilterFx::ZoomFilterImpl::setBuffSettings(const FXBuffSettings& settings)
{
  buffSettings = settings;
}

inline void ZoomFilterFx::ZoomFilterImpl::incNormalizedCoord(float& normalizedCoord)
{
  normalizedCoord += ratioPixmapToNormalizedCoord;
}

inline float ZoomFilterFx::ZoomFilterImpl::toNormalizedCoord(const int32_t pixmapCoord)
{
  return ratioPixmapToNormalizedCoord * static_cast<float>(pixmapCoord);
}

inline int32_t ZoomFilterFx::ZoomFilterImpl::toPixmapCoord(const float normalizedCoord)
{
  return static_cast<int32_t>(std::lround(ratioNormalizedCoordToPixmap * normalizedCoord));
}

void ZoomFilterFx::ZoomFilterImpl::log(const StatsLogValueFunc& logVal) const
{
  stats.setLastGeneralSpeed(generalSpeed);
  stats.setLastPrevX(screenWidth);
  stats.setLastPrevY(screenHeight);
  stats.setLastInterlaceStart(interlaceStart);
  stats.setLastTranDiffFactor(tranDiffFactor);

  stats.log(logVal);
}

ZoomFilterFx::ZoomFilterFx() noexcept : fxImpl{new ZoomFilterImpl{}}
{
}

ZoomFilterFx::ZoomFilterFx(Parallel& p, const std::shared_ptr<const PluginInfo>& info) noexcept
  : fxImpl{new ZoomFilterImpl{p, info}}
{
}

ZoomFilterFx::~ZoomFilterFx() noexcept
{
}

bool ZoomFilterFx::operator==(const ZoomFilterFx& f) const
{
  return fxImpl->operator==(*f.fxImpl);
}

void ZoomFilterFx::setBuffSettings(const FXBuffSettings& settings)
{
  fxImpl->setBuffSettings(settings);
}

void ZoomFilterFx::start()
{
}

void ZoomFilterFx::finish()
{
}

void ZoomFilterFx::log(const StatsLogValueFunc& logVal) const
{
  fxImpl->log(logVal);
}

std::string ZoomFilterFx::getFxName() const
{
  return "ZoomFilter FX";
}

void ZoomFilterFx::apply(PixelBuffer&, PixelBuffer&)
{
  throw std::logic_error("ZoomFilterFx::apply should never be called.");
}

void ZoomFilterFx::zoomFilterFastRGB(const PixelBuffer& pix1,
                                     PixelBuffer& pix2,
                                     const ZoomFilterData* zf,
                                     const int switchIncr,
                                     const float switchMult)
{
  if (!enabled)
  {
    return;
  }

  fxImpl->zoomFilterFastRGB(pix1, pix2, zf, switchIncr, switchMult);
}

/**
 * Main work for the dynamic displacement map.
 *
 * Reads data from pix1, write to pix2.
 *
 * Useful data for this FX are stored in ZoomFilterData.
 *
 * If you think that this is a strange function name, let me say that a long time ago,
 *  there has been a slow version and a gray-level only one. Then came these function,
 *  fast and working in RGB colorspace ! nice but it only was applying a zoom to the image.
 *  So that is why you have this name, for the nostalgy of the first days of goom
 *  when it was just a tiny program writen in Turbo Pascal on my i486...
 */
void ZoomFilterFx::ZoomFilterImpl::zoomFilterFastRGB(const PixelBuffer& pix1,
                                                     PixelBuffer& pix2,
                                                     const ZoomFilterData* zf,
                                                     const int32_t switchIncr,
                                                     const float switchMult)
{
  logDebug("switchIncr = {}, switchMult = {:.2}", switchIncr, switchMult);

  stats.doZoomFilterFastRGB();

  // changement de taille
  if (interlaceStart != -2)
  {
    zf = nullptr;
  }

  // changement de config
  if (zf)
  {
    stats.doZoomFilterFastRGBChangeConfig();
    filterData = *zf;
    generalSpeed = static_cast<float>(zf->vitesse - 128) / 128.0f;
    if (filterData.reverse)
    {
      generalSpeed = -generalSpeed;
    }
    interlaceStart = 0;
  }

  // generation du buffer de transform
  if (interlaceStart == -1)
  {
    stats.doZoomFilterFastRGBInterlaceStartEqualMinus1_1();
    // sauvegarde de l'etat actuel dans la nouvelle source
    // save the current state in the new source
    // TODO: write that in MMX (has been done in previous version, but did not follow
    //   some new fonctionnalities)
    for (size_t i = 0; i < screenWidth * screenHeight; i++)
    {
      tranXSrce[i] += ((tranXDest[i] - tranXSrce[i]) * tranDiffFactor) >> buffPointNum;
      tranYSrce[i] += ((tranYDest[i] - tranYSrce[i]) * tranDiffFactor) >> buffPointNum;
    }
    tranDiffFactor = 0;
  }

  // TODO Why if repeated?
  if (interlaceStart == -1)
  {
    stats.doZoomFilterFastRGBInterlaceStartEqualMinus1_2();

    std::swap(tranXDest, tranXTemp);
    std::swap(freeTranXDest, freeTranXTemp);

    std::swap(tranYDest, tranYTemp);
    std::swap(freeTranYDest, freeTranYTemp);

    interlaceStart = -2;
  }

  if (interlaceStart >= 0)
  {
    // creation de la nouvelle destination
    makeZoomBufferStripe(screenHeight / 16);
  }

  if (switchIncr != 0)
  {
    stats.doZoomFilterFastRGBSwitchIncrNotZero();

    tranDiffFactor += switchIncr;
    if (tranDiffFactor > buffPointMask)
    {
      tranDiffFactor = buffPointMask;
    }
  }

  // Equal was interesting but not correct!?
  //  if (std::fabs(1.0f - switchMult) < 0.0001)
  if (std::fabs(1.0F - switchMult) > 0.00001F)
  {
    stats.doZoomFilterFastRGBSwitchIncrNotEqual1();

    tranDiffFactor = static_cast<int32_t>(static_cast<float>(buffPointMask) * (1.0F - switchMult) +
                                          static_cast<float>(tranDiffFactor) * switchMult);
  }

  c_zoom(pix1, pix2);
}

void ZoomFilterFx::ZoomFilterImpl::generatePrecalCoef(uint32_t precalcCoeffs[16][16])
{
  for (uint32_t coefh = 0; coefh < 16; coefh++)
  {
    for (uint32_t coefv = 0; coefv < 16; coefv++)
    {
      const uint32_t diffcoeffh = sqrtperte - coefh;
      const uint32_t diffcoeffv = sqrtperte - coefv;

      if (!(coefh || coefv))
      {
        precalcCoeffs[coefh][coefv] = 255;
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

        precalcCoeffs[coefh][coefv] = CoeffArray{
            .c{
                static_cast<uint8_t>(i1),
                static_cast<uint8_t>(i2),
                static_cast<uint8_t>(i3),
                static_cast<uint8_t>(i4),
            }}.intVal;
      }
    }
  }
}

// pure c version of the zoom filter
void ZoomFilterFx::ZoomFilterImpl::c_zoom(const PixelBuffer& srceBuff, PixelBuffer& destBuff)
{
  stats.doCZoom();

  const uint32_t tran_ax = (screenWidth - 1) << perteDec;
  const uint32_t tran_ay = (screenHeight - 1) << perteDec;

  const auto setDestPixel = [&](const uint32_t destPos) {
    const uint32_t tran_px = static_cast<uint32_t>(
        tranXSrce[destPos] +
        (((tranXDest[destPos] - tranXSrce[destPos]) * tranDiffFactor) >> buffPointNum));
    const uint32_t tran_py = static_cast<uint32_t>(
        tranYSrce[destPos] +
        (((tranYDest[destPos] - tranYSrce[destPos]) * tranDiffFactor) >> buffPointNum));

    if ((tran_px >= tran_ax) || (tran_py >= tran_ay))
    {
      stats.doCZoomOutOfRange();
      destBuff(destPos) = Pixel{0U};
    }
    else
    {
      const uint32_t srcePos = (tran_px >> perteDec) + screenWidth * (tran_py >> perteDec);
      // coeff en modulo 15
      const size_t xIndex = tran_px & perteMask;
      const size_t yIndex = tran_py & perteMask;
      const CoeffArray coeffs{.intVal = precalcCoeffs[xIndex][yIndex]};

      const PixelArray colors = {
          srceBuff(srcePos),
          srceBuff(srcePos + 1),
          srceBuff(srcePos + screenWidth),
          srceBuff(srcePos + screenWidth + 1),
      };

      if (filterData.blockyWavy)
      {
        stats.doGetBlockyMixedColor();
        const Pixel newColor = getBlockyMixedColor(coeffs, colors);
        destBuff(destPos) = newColor;
      }
      else
      {
        stats.doGetMixedColor();
        const Pixel newColor = getMixedColor(coeffs, colors);
        destBuff(destPos) = newColor;

#ifndef NO_LOGGING
        if (colors[0].rgba() > 0xFF000000)
        {
          logInfo("srcePos == {}", srcePos);
          logInfo("destPos == {}", destPos);
          logInfo("tran_px >> perteDec == {}", tran_px >> perteDec);
          logInfo("tran_py >> perteDec == {}", tran_py >> perteDec);
          logInfo("tran_px == {}", tran_px);
          logInfo("tran_py == {}", tran_py);
          logInfo("tran_px & perteMask == {}", tran_px & perteMask);
          logInfo("tran_py & perteMask == {}", tran_py & perteMask);
          logInfo("coeffs[0] == {:x}", coeffs.c[0]);
          logInfo("coeffs[1] == {:x}", coeffs.c[1]);
          logInfo("coeffs[2] == {:x}", coeffs.c[2]);
          logInfo("coeffs[3] == {:x}", coeffs.c[3]);
          logInfo("colors[0] == {:x}", colors[0].rgba());
          logInfo("colors[1] == {:x}", colors[1].rgba());
          logInfo("colors[2] == {:x}", colors[2].rgba());
          logInfo("colors[3] == {:x}", colors[3].rgba());
          logInfo("newColor == {:x}", newColor.rgba());
        }
#endif
      }
    }
  };

  /**
  static std::vector<uint32_t> getIndexArray(const size_t bufferSize)
  {
    std::vector<uint32_t> vec(bufferSize);
    std::iota(vec.begin(), vec.end(), 0);
    return vec;
  }

  static const std::vector<uint32_t> indexArray{getIndexArray(bufferSize)};
  std::for_each(std::execution::par_unseq, indexArray.begin(), indexArray.end(), setDestPixel);
  **/

  parallel->forLoop(bufferSize, setDestPixel);
}

/*
 * Makes a stripe of a transform buffer
 *
 * The transform is (in order) :
 * Translation (-data->middleX, -data->middleY)
 * Homothetie (Center : 0,0   Coeff : 2/data->screenWidth)
 */
void ZoomFilterFx::ZoomFilterImpl::makeZoomBufferStripe(const uint32_t interlaceIncrement)
{
  stats.doMakeZoomBufferStripe();

  assert(interlaceStart >= 0);

  const float normMiddleX = toNormalizedCoord(filterData.middleX);
  const float normMiddleY = toNormalizedCoord(filterData.middleY);

  // Where (vertically) to stop generating the buffer stripe
  const int32_t maxEnd = std::min(static_cast<int32_t>(screenHeight),
                                  interlaceStart + static_cast<int32_t>(interlaceIncrement));

  // Position of the pixel to compute in pixmap coordinates
  const auto doStripeLine = [&](const uint32_t y) {
    const uint32_t yOffset = y + static_cast<uint32_t>(interlaceStart);
    const uint32_t yTimesScreenWidth = yOffset * screenWidth;
    const float normY =
        toNormalizedCoord(static_cast<int32_t>(yOffset) - static_cast<int32_t>(filterData.middleY));
    float normX = -toNormalizedCoord(static_cast<int32_t>(filterData.middleX));

    for (uint32_t x = 0; x < screenWidth; x++)
    {
      const v2g vector = getZoomVector(normX, normY);
      const uint32_t tranPos = yTimesScreenWidth + x;

      tranXTemp[tranPos] = toPixmapCoord((normX - vector.x + normMiddleX) * buffPointNumFlt);
      tranYTemp[tranPos] = toPixmapCoord((normY - vector.y + normMiddleY) * buffPointNumFlt);

      incNormalizedCoord(normX);
    }
  };

  parallel->forLoop(static_cast<uint32_t>(maxEnd - interlaceStart), doStripeLine);

  interlaceStart += static_cast<int32_t>(interlaceIncrement);
  if (maxEnd == static_cast<int32_t>(screenHeight))
  {
    interlaceStart = -1;
  }
}

void ZoomFilterFx::ZoomFilterImpl::generateWaterFXHorizontalBuffer()
{
  stats.doGenerateWaterFXHorizontalBuffer();

  int32_t decc = getRandInRange(-4, +4);
  int32_t spdc = getRandInRange(-4, +4);
  int32_t accel = getRandInRange(-4, +4);

  for (size_t loopv = screenHeight; loopv != 0;)
  {
    loopv--;
    firedec[loopv] = decc;
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
      spdc = spdc - static_cast<int32_t>(getNRand(3)) + accel / 10;
    }
    if (spdc < -30)
    {
      spdc = spdc + static_cast<int32_t>(getNRand(3)) + accel / 10;
    }

    if (decc > 8 && spdc > 1)
    {
      spdc -= getRandInRange(-2, +1);
    }
    if (decc < -8 && spdc < -1)
    {
      spdc += static_cast<int32_t>(getNRand(3)) + 2;
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

v2g ZoomFilterFx::ZoomFilterImpl::getZoomVector(const float normX, const float normY)
{
  stats.doZoomVector();

  /* sx = (x < 0.0f) ? -1.0f : 1.0f;
     sy = (y < 0.0f) ? -1.0f : 1.0f;
   */
  float coefVitesse = (1.0f + generalSpeed) / 50.0f;

  // The Effects
  switch (filterData.mode)
  {
    case ZoomFilterMode::crystalBallMode:
    {
      stats.doZoomVectorCrystalBallMode();
      coefVitesse -= filterData.crystalBallAmplitude * (sq_distance(normX, normY) - 0.3f);
      break;
    }
    case ZoomFilterMode::amuletteMode:
    {
      stats.doZoomVectorAmuletteMode();
      coefVitesse += filterData.amuletteAmplitude * sq_distance(normX, normY);
      break;
    }
    case ZoomFilterMode::waveMode:
    {
      stats.doZoomVectorWaveMode();
      const float angle = sq_distance(normX, normY) * filterData.waveFreqFactor;
      float periodicPart;
      switch (filterData.waveEffectType)
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
      coefVitesse += filterData.waveAmplitude * periodicPart;
      break;
    }
    case ZoomFilterMode::scrunchMode:
    {
      stats.doZoomVectorScrunchMode();
      coefVitesse += filterData.scrunchAmplitude * sq_distance(normX, normY);
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
      coefVitesse *= filterData.speedwayAmplitude * normY;
      break;
    }
    default:
      stats.doZoomVectorDefaultMode();
      break;
  }

  if (coefVitesse < minCoefVitesse)
  {
    coefVitesse = minCoefVitesse;
    stats.coeffVitesseBelowMin();
  }
  else if (coefVitesse > maxCoefVitesse)
  {
    coefVitesse = maxCoefVitesse;
    stats.coeffVitesseAboveMax();
  }

  float vx = coefVitesse * normX;
  float vy = coefVitesse * normY;

  /* Amulette 2 */
  // vx = X * tan(dist);
  // vy = Y * tan(dist);

  /* Rotate */
  //vx = (X+Y)*0.1;
  //vy = (Y-X)*0.1;

  // The Effects adds-on
  if (filterData.noisify)
  {
    stats.doZoomVectorNoisify();
    if (filterData.noiseFactor > 0.01)
    {
      stats.doZoomVectorNoiseFactor();
      //    const float xAmp = 1.0/getRandInRange(50.0f, 200.0f);
      //    const float yAmp = 1.0/getRandInRange(50.0f, 200.0f);
      const float amp = filterData.noiseFactor / getRandInRange(noiseMin, noiseMax);
      filterData.noiseFactor *= 0.999;
      vx += amp * (getRandInRange(0.0f, 1.0f) - 0.5f);
      vy += amp * (getRandInRange(0.0f, 1.0f) - 0.5f);
    }
  }

  switch (filterData.hypercosEffect)
  {
    case ZoomFilterData::HypercosEffect::none:
      break;
    case ZoomFilterData::HypercosEffect::sinRectangular:
      stats.doZoomVectorHypercosEffect();
      vx += filterData.hypercosAmplitude * sin(filterData.hypercosFreq * normX);
      vy += filterData.hypercosAmplitude * sin(filterData.hypercosFreq * normY);
      break;
    case ZoomFilterData::HypercosEffect::cosRectangular:
      stats.doZoomVectorHypercosEffect();
      vx += filterData.hypercosAmplitude * cos(filterData.hypercosFreq * normX);
      vy += filterData.hypercosAmplitude * cos(filterData.hypercosFreq * normY);
      break;
    case ZoomFilterData::HypercosEffect::sinCurlSwirl:
      stats.doZoomVectorHypercosEffect();
      vx += filterData.hypercosAmplitude * sin(filterData.hypercosFreq * normY);
      vy += filterData.hypercosAmplitude * sin(filterData.hypercosFreq * normX);
      break;
    case ZoomFilterData::HypercosEffect::cosCurlSwirl:
      stats.doZoomVectorHypercosEffect();
      vx += filterData.hypercosAmplitude * cos(filterData.hypercosFreq * normY);
      vy += filterData.hypercosAmplitude * cos(filterData.hypercosFreq * normX);
      break;
    default:
      throw std::logic_error("Unknown filterData.hypercosEffect value");
  }

  if (filterData.hPlaneEffect)
  {
    stats.doZoomVectorHPlaneEffect();

    vx += normY * filterData.hPlaneEffectAmplitude * static_cast<float>(filterData.hPlaneEffect);
  }

  if (filterData.vPlaneEffect)
  {
    stats.doZoomVectorVPlaneEffect();
    vy += normX * filterData.vPlaneEffectAmplitude * static_cast<float>(filterData.vPlaneEffect);
  }

  /* TODO : Water Mode */
  //    if (data->waveEffect)

  // avoid null displacement
  if (std::fabs(vx) < minNormCoordVal)
  {
    vx = (vx < 0.0f) ? -minNormCoordVal : minNormCoordVal;
  }
  if (std::fabs(vy) < minNormCoordVal)
  {
    vy = (vy < 0.0f) ? -minNormCoordVal : minNormCoordVal;
  }

  return v2g{vx, vy};
}

inline Pixel ZoomFilterFx::ZoomFilterImpl::getMixedColor(const CoeffArray& coeffs,
                                                         const PixelArray& colors) const
{
  if (coeffs.intVal == 0)
  {
    return Pixel{0U};
  }

  uint32_t newR = 0;
  uint32_t newG = 0;
  uint32_t newB = 0;
  for (size_t i = 0; i < numCoeffs; i++)
  {
    const uint32_t coeff = static_cast<uint32_t>(coeffs.c[i]);
    newR += static_cast<uint32_t>(colors[i].r()) * coeff;
    newG += static_cast<uint32_t>(colors[i].g()) * coeff;
    newB += static_cast<uint32_t>(colors[i].b()) * coeff;
  }
  newR >>= 8;
  newG >>= 8;
  newB >>= 8;

  if (buffSettings.allowOverexposed)
  {
    return Pixel{{.r = static_cast<uint8_t>((newR & 0xffffff00) ? 0xff : newR),
                  .g = static_cast<uint8_t>((newG & 0xffffff00) ? 0xff : newG),
                  .b = static_cast<uint8_t>((newB & 0xffffff00) ? 0xff : newB),
                  .a = 0xff}};
  }

  const uint32_t maxVal = std::max({newR, newG, newB});
  if (maxVal > channel_limits<uint32_t>::max())
  {
    // scale all channels back
    newR = (newR << 8) / maxVal;
    newG = (newG << 8) / maxVal;
    newB = (newB << 8) / maxVal;
  }

  return Pixel{{.r = static_cast<uint8_t>(newR),
                .g = static_cast<uint8_t>(newG),
                .b = static_cast<uint8_t>(newB),
                .a = 0xff}};
}

inline Pixel ZoomFilterFx::ZoomFilterImpl::getBlockyMixedColor(const CoeffArray& coeffs,
                                                               const PixelArray& colors) const
{
  // Changing the color order gives a strange blocky, wavy look.
  // The order col4, col3, col2, col1 gave a black tear - no so good.
  const PixelArray reorderedColors{colors[0], colors[2], colors[1], colors[3]};
  return getMixedColor(coeffs, reorderedColors);
}

} // namespace goom
