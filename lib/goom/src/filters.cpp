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
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
#include "goomutils/mathutils.h"
// #undef NO_LOGGING
#include "goomutils/logging.h"
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
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

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

// For noise amplitude, take the reciprocal of these.
constexpr float noiseMin = 70;
constexpr float noiseMax = 120;

constexpr size_t buffPointNum = 16;
constexpr float buffPointNumFlt = static_cast<float>(buffPointNum);
constexpr int buffPointMask = 0xffff;

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

struct ZoomFilterImpl
{
  explicit ZoomFilterImpl(const PluginInfo*);

  void setBuffSettings(const FXBuffSettings&);

  void zoomFilterFastRGB(Pixel* pix1,
                         Pixel* pix2,
                         const ZoomFilterData* zf,
                         const int switchIncr,
                         const float switchMult);

  void log(const StatsLogValueFunc& logVal) const;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(CEREAL_NVP(filterData), CEREAL_NVP(buffSettings), CEREAL_NVP(generalSpeed),
       CEREAL_NVP(interlaceStart), CEREAL_NVP(buffRatio));
  };

private:
  const uint32_t screenWidth;
  const uint32_t screenHeight;
  const float ratioPixmapToNormalizedCoords;
  const float minNormCoordVal;
  ZoomFilterData filterData;
  FXBuffSettings buffSettings{};

  mutable FilterStats stats{};

  float generalSpeed = 0;
  int interlaceStart = 0;

  std::vector<int32_t> freebrutS{}; // source
  int32_t* brutS = nullptr;
  std::vector<int32_t> freebrutD{}; // dest
  int32_t* brutD = nullptr;
  std::vector<int32_t> freebrutT{}; // temp (en cours de generation)
  int32_t* brutT = nullptr;

  // modification by jeko : fixedpoint : buffRatio = (16:16) (donc 0<=buffRatio<=2^16)
  int buffRatio = 0;
  std::vector<int32_t> firedec{};

  // modif d'optim by Jeko : precalcul des 4 coefs resultant des 2 pos
  uint32_t precalCoef[buffPointNum][buffPointNum];

  void makeZoomBufferStripe(const uint32_t interlaceIncrement);
  void c_zoom(Pixel* expix1, Pixel* expix2);
  void generateWaterFXHorizontalBuffer();
  v2g getZoomVector(const float x, const float y);
  static void generatePrecalCoef(uint32_t precalCoef[16][16]);
  Pixel getMixedColor(const CoeffArray& coeffs, const PixelArray& colors) const;
  Pixel getBlockyMixedColor(const CoeffArray& coeffs, const PixelArray& colors) const;
};

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

ZoomFilterImpl::ZoomFilterImpl(const PluginInfo* goomInfo)
  : screenWidth{goomInfo->screen.width},
    screenHeight{goomInfo->screen.height},
    ratioPixmapToNormalizedCoords{2.0F / static_cast<float>(screenWidth)},
    minNormCoordVal{ratioPixmapToNormalizedCoords / buffPointNumFlt},
    filterData{},
    freebrutS(goomInfo->screen.width * goomInfo->screen.height * 2 + 128),
    brutS{(int32_t*)((1 + (uintptr_t((freebrutS.data()))) / 128) * 128)},
    freebrutD(goomInfo->screen.width * goomInfo->screen.height * 2 + 128),
    brutD{(int32_t*)((1 + (uintptr_t((freebrutD.data()))) / 128) * 128)},
    freebrutT(goomInfo->screen.width * goomInfo->screen.height * 2 + 128),
    brutT{(int32_t*)((1 + (uintptr_t((freebrutT.data()))) / 128) * 128)},
    firedec(screenHeight)
{
  filterData.middleX = goomInfo->screen.width / 2;
  filterData.middleY = goomInfo->screen.height / 2;

  generatePrecalCoef(precalCoef);
  generateWaterFXHorizontalBuffer();
  makeZoomBufferStripe(goomInfo->screen.height);

  // Copy the data from temp to dest and source
  memcpy(brutS, brutT, goomInfo->screen.width * goomInfo->screen.height * 2 * sizeof(int));
  memcpy(brutD, brutT, goomInfo->screen.width * goomInfo->screen.height * 2 * sizeof(int));
}

void ZoomFilterImpl::setBuffSettings(const FXBuffSettings& settings)
{
  buffSettings = settings;
}

void ZoomFilterImpl::log(const StatsLogValueFunc& logVal) const
{
  stats.setLastGeneralSpeed(generalSpeed);
  stats.setLastPrevX(screenWidth);
  stats.setLastPrevY(screenHeight);
  stats.setLastInterlaceStart(interlaceStart);
  stats.setLastBuffratio(buffRatio);

  stats.log(logVal);
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
void ZoomFilterImpl::zoomFilterFastRGB(Pixel* pix1,
                                       Pixel* pix2,
                                       const ZoomFilterData* zf,
                                       const int switchIncr,
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

  // generation du buffer de trans
  logDebug("interlaceStart = {}", interlaceStart);
  if (interlaceStart == -1)
  {
    stats.doZoomFilterFastRGBInterlaceStartEqualMinus1_1();
    // sauvegarde de l'etat actuel dans la nouvelle source
    // TODO: write that in MMX (has been done in previous version, but did not follow
    //   some new fonctionnalities)
    const uint32_t y = 2 * screenWidth * screenHeight;
    for (uint32_t x = 0; x < y; x += 2)
    {
      int brutSmypos = brutS[x];
      const uint32_t x2 = x + 1;

      brutS[x] = brutSmypos + (((brutD[x] - brutSmypos) * buffRatio) >> buffPointNum);
      brutSmypos = brutS[x2];
      brutS[x2] = brutSmypos + (((brutD[x2] - brutSmypos) * buffRatio) >> buffPointNum);
    }
    buffRatio = 0;
  }

  logDebug("interlaceStart = {}", interlaceStart);
  if (interlaceStart == -1)
  {
    stats.doZoomFilterFastRGBInterlaceStartEqualMinus1_2();

    std::swap(brutD, brutT);
    std::swap(freebrutD, freebrutT);

    interlaceStart = -2;
  }

  logDebug("interlaceStart = {}", interlaceStart);
  if (interlaceStart >= 0)
  {
    // creation de la nouvelle destination
    makeZoomBufferStripe(screenHeight / 16);
  }

  if (switchIncr != 0)
  {
    stats.doZoomFilterFastRGBSwitchIncrNotZero();

    buffRatio += switchIncr;
    if (buffRatio > buffPointMask)
    {
      buffRatio = buffPointMask;
    }
  }

  // Equal was interesting but not correct!?
  //  if (std::fabs(1.0f - switchMult) < 0.0001)
  if (std::fabs(1.0F - switchMult) > 0.00001F)
  {
    stats.doZoomFilterFastRGBSwitchIncrNotEqual1();

    buffRatio = static_cast<int>(static_cast<float>(buffPointMask) * (1.0F - switchMult) +
                                 static_cast<float>(buffRatio) * switchMult);
  }

  c_zoom(pix1, pix2);
}

void ZoomFilterImpl::generatePrecalCoef(uint32_t precalCoef[16][16])
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

/*
 * Makes a stripe of a transform buffer (brutT)
 *
 * The transform is (in order) :
 * Translation (-data->middleX, -data->middleY)
 * Homothetie (Center : 0,0   Coeff : 2/data->prevX)
 */
void ZoomFilterImpl::makeZoomBufferStripe(const uint32_t interlaceIncrement)
{
  stats.doMakeZoomBufferStripe();

  assert(interlaceStart >= 0);

  // Ratio from normalized to virtual pixmap coordinates
  const float invRatio = buffPointNumFlt / ratioPixmapToNormalizedCoords;

  // Y position of the pixel to compute in normalized coordinates
  float normY = ratioPixmapToNormalizedCoords *
                static_cast<float>(interlaceStart - static_cast<int>(filterData.middleY));

  // Where (vertically) to stop generating the buffer stripe
  const uint32_t maxEnd =
      std::min(screenHeight, static_cast<uint32_t>(interlaceStart) + interlaceIncrement);

  // Position of the pixel to compute in pixmap coordinates
  for (uint32_t y = static_cast<uint32_t>(interlaceStart); y < maxEnd; y++)
  {
    uint32_t brutPos = y * screenWidth * 2;
    float normX = -static_cast<float>(filterData.middleX) * ratioPixmapToNormalizedCoords;
    for (uint32_t x = 0; x < screenWidth; x++)
    {
      const v2g vector = getZoomVector(normX, normY);

      brutT[brutPos] = static_cast<int>((normX - vector.x) * invRatio) +
                       static_cast<int>(filterData.middleX * buffPointNum);
      brutT[brutPos + 1] = static_cast<int>((normY - vector.y) * invRatio) +
                           static_cast<int>(filterData.middleY * buffPointNum);

      brutPos += 2;
      normX += ratioPixmapToNormalizedCoords;
    }
    normY += ratioPixmapToNormalizedCoords;
  }

  interlaceStart += static_cast<int>(interlaceIncrement);
  if (maxEnd == screenHeight)
  {
    interlaceStart = -1;
  }
}

void ZoomFilterImpl::generateWaterFXHorizontalBuffer()
{
  stats.doGenerateWaterFXHorizontalBuffer();

  int decc = getRandInRange(-4, +4);
  int spdc = getRandInRange(-4, +4);
  int accel = getRandInRange(-4, +4);

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

v2g ZoomFilterImpl::getZoomVector(const float x, const float y)
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
      coefVitesse -= filterData.crystalBallAmplitude * (sq_distance(x, y) - 0.3f);
      break;
    }
    case ZoomFilterMode::amuletteMode:
    {
      stats.doZoomVectorAmuletteMode();
      coefVitesse += filterData.amuletteAmplitude * sq_distance(x, y);
      break;
    }
    case ZoomFilterMode::waveMode:
    {
      stats.doZoomVectorWaveMode();
      const float angle = sq_distance(x, y) * filterData.waveFreqFactor;
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
      coefVitesse += filterData.scrunchAmplitude * sq_distance(x, y);
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
      coefVitesse *= filterData.speedwayAmplitude * y;
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

  if (filterData.hypercosEffect)
  {
    stats.doZoomVectorHypercosEffect();
    vx += filterData.hypercosAmplitude * sin(filterData.hypercosFreq * y);
    vy += filterData.hypercosAmplitude * sin(filterData.hypercosFreq * x);
  }

  if (filterData.hPlaneEffect)
  {
    stats.doZoomVectorHPlaneEffect();

    vx += y * filterData.hPlaneEffectAmplitude * static_cast<float>(filterData.hPlaneEffect);
  }

  if (filterData.vPlaneEffect)
  {
    stats.doZoomVectorVPlaneEffect();
    vy += x * filterData.vPlaneEffectAmplitude * static_cast<float>(filterData.vPlaneEffect);
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

inline Pixel ZoomFilterImpl::getMixedColor(const CoeffArray& coeffs, const PixelArray& colors) const
{
  if (coeffs.intVal == 0)
  {
    return Pixel{.val = 0};
  }

  uint32_t newR = 0;
  uint32_t newG = 0;
  uint32_t newB = 0;
  for (size_t i = 0; i < numCoeffs; i++)
  {
    const uint32_t coeff = static_cast<uint32_t>(coeffs.c[i]);
    newR += static_cast<uint32_t>(colors[i].channels.r) * coeff;
    newG += static_cast<uint32_t>(colors[i].channels.g) * coeff;
    newB += static_cast<uint32_t>(colors[i].channels.b) * coeff;
  }
  newR >>= 8;
  newG >>= 8;
  newB >>= 8;

  if (buffSettings.allowOverexposed)
  {
    return Pixel{.channels = {.r = static_cast<uint8_t>((newR & 0xffffff00) ? 0xff : newR),
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

  return Pixel{.channels = {.r = static_cast<uint8_t>(newR),
                            .g = static_cast<uint8_t>(newG),
                            .b = static_cast<uint8_t>(newB),
                            .a = 0xff}};
}

inline Pixel ZoomFilterImpl::getBlockyMixedColor(const CoeffArray& coeffs,
                                                 const PixelArray& colors) const
{
  // Changing the color order gives a strange blocky, wavy look.
  // The order col4, col3, col2, col1 gave a black tear - no so good.
  const PixelArray reorderedColors{colors[0], colors[2], colors[1], colors[3]};
  return getMixedColor(coeffs, reorderedColors);
}

inline Pixel getPixelColor(const Pixel* buffer, const uint32_t pos)
{
  return buffer[pos];
}

inline void setPixelColor(Pixel* buffer, const uint32_t pos, const Pixel& p)
{
  buffer[pos] = p;
}

// pure c version of the zoom filter
void ZoomFilterImpl::c_zoom(Pixel* expix1, Pixel* expix2)
{
  stats.doCZoom();

  const uint32_t ax = (screenWidth - 1) << perteDec;
  const uint32_t ay = (screenHeight - 1) << perteDec;

  const uint32_t buffSize = screenWidth * screenHeight * 2;
  const uint32_t buffWidth = screenWidth;

  expix1[0].val = 0;
  expix1[screenWidth - 1].val = 0;
  expix1[screenWidth * screenHeight - 1].val = 0;
  expix1[screenWidth * screenHeight - screenWidth].val = 0;

  for (uint32_t myPos = 0; myPos < buffSize; myPos += 2)
  {
    const uint32_t myPos2 = myPos + 1;

    const int brutSmypos = brutS[myPos];
    const uint32_t px = static_cast<uint32_t>(
        brutSmypos + (((brutD[myPos] - brutSmypos) * buffRatio) >> buffPointNum));

    const int brutSmypos2 = brutS[myPos2];
    const uint32_t py = static_cast<uint32_t>(
        brutSmypos2 + (((brutD[myPos2] - brutSmypos2) * buffRatio) >> buffPointNum));

    const uint32_t pix2Pos = static_cast<uint32_t>(myPos >> 1);

    if ((px >= ax) || (py >= ay))
    {
      setPixelColor(expix2, pix2Pos, Pixel{.val = 0});
    }
    else
    {
      // coeff en modulo 15
      const CoeffArray coeffs{.intVal = precalCoef[px & perteMask][py & perteMask]};

      const uint32_t pix1Pos = (px >> perteDec) + screenWidth * (py >> perteDec);
      const PixelArray colors = {
          getPixelColor(expix1, pix1Pos),
          getPixelColor(expix1, pix1Pos + 1),
          getPixelColor(expix1, pix1Pos + buffWidth),
          getPixelColor(expix1, pix1Pos + buffWidth + 1),
      };

      if (filterData.blockyWavy)
      {
        stats.doGetBlockyMixedColor();
        const Pixel newColor = getBlockyMixedColor(coeffs, colors);
        setPixelColor(expix2, pix2Pos, newColor);
      }
      else
      {
        stats.doGetMixedColor();
        const Pixel newColor = getMixedColor(coeffs, colors);
        setPixelColor(expix2, pix2Pos, newColor);
      }
    }
  }
}

ZoomFilterFx::ZoomFilterFx(PluginInfo* info) : goomInfo{info}, fxImpl{new ZoomFilterImpl{goomInfo}}
{
}

ZoomFilterFx::~ZoomFilterFx() noexcept
{
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
  std::ofstream f("/tmp/zoom-filter.json");
  saveState(f);
  f << std::endl;
  f.close();
}

void ZoomFilterFx::log(const StatsLogValueFunc& logVal) const
{
  fxImpl->log(logVal);
}

std::string ZoomFilterFx::getFxName() const
{
  return "ZoomFilter FX";
}

void ZoomFilterFx::saveState(std::ostream& f)
{
  cereal::JSONOutputArchive archiveOut(f);
  archiveOut(*fxImpl);
}

void ZoomFilterFx::loadState(std::istream& f)
{
  cereal::JSONInputArchive archive_in(f);
  archive_in(*fxImpl);
}

void ZoomFilterFx::apply(Pixel*, Pixel*)
{
  throw std::logic_error("ZoomFilterFx::apply should never be called.");
}
void ZoomFilterFx::zoomFilterFastRGB(Pixel* pix1,
                                     Pixel* pix2,
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

} // namespace goom
