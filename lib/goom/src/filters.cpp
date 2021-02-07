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
//#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "goomutils/parallel_utils.h"
#include "stats/filter_stats.h"
#include "v2d.h"

#include <array>
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace GOOM
{

using namespace GOOM::UTILS;

// For noise amplitude, take the reciprocal of these.
constexpr float NOISE_MIN = 70;
constexpr float NOISE_MAX = 120;

constexpr int32_t BUFF_POINT_NUM = 16;
constexpr float BUFF_POINT_NUM_FLT = static_cast<float>(BUFF_POINT_NUM);
constexpr int32_t BUFF_POINT_MASK = 0xffff;

constexpr uint32_t SQRT_PERTE = 16;
// faire : a % SQRT_PERTE <=> a & PERTE_MASK
constexpr uint32_t PERTE_MASK = 0xf;
// faire : a / SQRT_PERTE <=> a >> PERTE_DEC
constexpr uint32_t PERTE_DEC = 4;

static constexpr size_t NUM_COEFFS = 4;
using CoeffArray = union
{
  std::array<uint8_t, NUM_COEFFS> c;
  uint32_t intVal = 0;
};
using PixelArray = std::array<Pixel, NUM_COEFFS>;

constexpr float MIN_COEF_VITESSE = -2.01;
constexpr float MAX_COEF_VITESSE = +2.01;

class ZoomFilterFx::ZoomFilterImpl
{
public:
  ZoomFilterImpl(Parallel& p, const std::shared_ptr<const PluginInfo>& goomInfo);
  ~ZoomFilterImpl() noexcept;
  ZoomFilterImpl(const ZoomFilterImpl&) noexcept = delete;
  ZoomFilterImpl(ZoomFilterImpl&&) noexcept = delete;
  auto operator=(const ZoomFilterImpl&) -> ZoomFilterImpl& = delete;
  auto operator=(ZoomFilterImpl&&) -> ZoomFilterImpl& = delete;

  void SetBuffSettings(const FXBuffSettings& settings);

  void ZoomFilterFastRgb(const PixelBuffer& pix1,
                         PixelBuffer& pix2,
                         const ZoomFilterData* zf,
                         int32_t switchIncr,
                         float switchMult);

  auto GetFilterData() const -> const ZoomFilterData&;
  auto GetGeneralSpeed() const -> float;
  auto GetInterlaceStart() const -> int32_t;

  void Log(const StatsLogValueFunc& l) const;

private:
  const uint32_t m_screenWidth;
  const uint32_t m_screenHeight;
  const uint32_t m_bufferSize;
  const float m_ratioPixmapToNormalizedCoord;
  const float m_ratioNormalizedCoordToPixmap;
  const float m_minNormCoordVal;
  ZoomFilterData m_filterData{};
  FXBuffSettings m_buffSettings{};

  Parallel* const m_parallel;

  auto ToNormalizedCoord(int32_t pixmapCoord) const -> float;
  auto ToPixmapCoord(float normalizedCoord) const -> int32_t;

  void IncNormalizedCoord(float& normalizedCoord) const;

  mutable FilterStats m_stats{};

  float m_generalSpeed = 0;
  int32_t m_interlaceStart = 0;

  std::vector<int32_t> m_freeTranXSrce{}; // source
  std::vector<int32_t> m_freeTranYSrce{}; // source
  int32_t* m_tranXSrce{};
  int32_t* m_tranYSrce{};
  std::vector<int32_t> m_freeTranXDest{}; // dest
  std::vector<int32_t> m_freeTranYDest{}; // dest
  int32_t* m_tranXDest{};
  int32_t* m_tranYDest{};
  std::vector<int32_t> m_freeTranXTemp{}; // temp (en cours de generation)
  std::vector<int32_t> m_freeTranYTemp{}; // temp (en cours de generation)
  int32_t* m_tranXTemp{};
  int32_t* m_tranYTemp{};
  auto GetTransformedPoint(size_t buffPos) const -> std::tuple<uint32_t, uint32_t>;
  auto GetSourceInfo(uint32_t tranPx, uint32_t tranPy) const
      -> std::tuple<uint32_t, uint32_t, CoeffArray>;
  auto GetNewColor(const CoeffArray& coeffs, const PixelArray& pixels) const -> Pixel;

  // modification by jeko : fixedpoint : tranDiffFactor = (16:16) (0 <= tranDiffFactor <= 2^16)
  int32_t m_tranDiffFactor = 0;
  std::vector<int32_t> m_firedec{};

  // modif d'optim by Jeko : precalcul des 4 coefs resultant des 2 pos
  using Coeff2dArray = std::array<std::array<uint32_t, BUFF_POINT_NUM>, BUFF_POINT_NUM>;
  Coeff2dArray m_precalcCoeffs{};

  void Init();
  void InitBuffers();

  void MakeZoomBufferStripe(uint32_t interlaceIncrement);
  void CZoom(const PixelBuffer& srceBuff, PixelBuffer& destBuff);
  void GenerateWaterFxHorizontalBuffer();
  auto GetZoomVector(float normX, float normY) -> V2dFlt;
  static void GeneratePrecalCoef(Coeff2dArray& precalcCoeffs);
  auto GetMixedColor(const CoeffArray& coeffs, const PixelArray& colors) const -> Pixel;
  auto GetBlockyMixedColor(const CoeffArray& coeffs, const PixelArray& colors) const -> Pixel;
};

ZoomFilterFx::ZoomFilterImpl::ZoomFilterImpl(Parallel& p,
                                             const std::shared_ptr<const PluginInfo>& goomInfo)
  : m_screenWidth{goomInfo->GetScreenInfo().width},
    m_screenHeight{goomInfo->GetScreenInfo().height},
    m_bufferSize{goomInfo->GetScreenInfo().size},
    m_ratioPixmapToNormalizedCoord{2.0F / static_cast<float>(m_screenWidth)},
    m_ratioNormalizedCoordToPixmap{1.0F / m_ratioPixmapToNormalizedCoord},
    m_minNormCoordVal{m_ratioPixmapToNormalizedCoord / BUFF_POINT_NUM_FLT},
    m_parallel{&p},
    m_freeTranXSrce(m_bufferSize + 128),
    m_freeTranYSrce(m_bufferSize + 128),
    m_freeTranXDest(m_bufferSize + 128),
    m_freeTranYDest(m_bufferSize + 128),
    m_freeTranXTemp(m_bufferSize + 128),
    m_freeTranYTemp(m_bufferSize + 128),
    m_firedec(m_screenHeight)
{
  Init();
}

ZoomFilterFx::ZoomFilterImpl::~ZoomFilterImpl() noexcept = default;

void ZoomFilterFx::ZoomFilterImpl::Init()
{
  InitBuffers();

  m_filterData.middleX = m_screenWidth / 2;
  m_filterData.middleY = m_screenHeight / 2;

  GeneratePrecalCoef(m_precalcCoeffs);
  GenerateWaterFxHorizontalBuffer();
  MakeZoomBufferStripe(m_screenHeight);

  // Copy the data from temp to dest and source
  memcpy(m_tranXSrce, m_tranXTemp, m_bufferSize * sizeof(int32_t));
  memcpy(m_tranYSrce, m_tranYTemp, m_bufferSize * sizeof(int32_t));
  memcpy(m_tranXDest, m_tranXTemp, m_bufferSize * sizeof(int32_t));
  memcpy(m_tranYDest, m_tranYTemp, m_bufferSize * sizeof(int32_t));
}

void ZoomFilterFx::ZoomFilterImpl::InitBuffers()
{
  m_tranXSrce = (int32_t*)((1 + (uintptr_t((m_freeTranXSrce.data()))) / 128) * 128);
  m_tranYSrce = (int32_t*)((1 + (uintptr_t((m_freeTranYSrce.data()))) / 128) * 128);

  m_tranXDest = (int32_t*)((1 + (uintptr_t((m_freeTranXDest.data()))) / 128) * 128);
  m_tranYDest = (int32_t*)((1 + (uintptr_t((m_freeTranYDest.data()))) / 128) * 128);

  m_tranXTemp = (int32_t*)((1 + (uintptr_t((m_freeTranXTemp.data()))) / 128) * 128);
  m_tranYTemp = (int32_t*)((1 + (uintptr_t((m_freeTranYTemp.data()))) / 128) * 128);
}

void ZoomFilterFx::ZoomFilterImpl::SetBuffSettings(const FXBuffSettings& settings)
{
  m_buffSettings = settings;
}

inline void ZoomFilterFx::ZoomFilterImpl::IncNormalizedCoord(float& normalizedCoord) const
{
  normalizedCoord += m_ratioPixmapToNormalizedCoord;
}

inline auto ZoomFilterFx::ZoomFilterImpl::ToNormalizedCoord(const int32_t pixmapCoord) const
    -> float
{
  return m_ratioPixmapToNormalizedCoord * static_cast<float>(pixmapCoord);
}

inline auto ZoomFilterFx::ZoomFilterImpl::ToPixmapCoord(const float normalizedCoord) const
    -> int32_t
{
  return static_cast<int32_t>(std::lround(m_ratioNormalizedCoordToPixmap * normalizedCoord));
}

void ZoomFilterFx::ZoomFilterImpl::Log(const StatsLogValueFunc& l) const
{
  m_stats.SetLastGeneralSpeed(m_generalSpeed);
  m_stats.SetLastPrevX(m_screenWidth);
  m_stats.SetLastPrevY(m_screenHeight);
  m_stats.SetLastInterlaceStart(m_interlaceStart);
  m_stats.SetLastTranDiffFactor(m_tranDiffFactor);

  m_stats.Log(l);
}

ZoomFilterFx::ZoomFilterFx(Parallel& p, const std::shared_ptr<const PluginInfo>& info) noexcept
  : m_fxImpl{new ZoomFilterImpl{p, info}}
{
}

ZoomFilterFx::~ZoomFilterFx() noexcept = default;

auto ZoomFilterFx::GetResourcesDirectory() const -> const std::string&
{
  return "";
}

void ZoomFilterFx::SetResourcesDirectory([[maybe_unused]] const std::string& dirName)
{
}

void ZoomFilterFx::SetBuffSettings(const FXBuffSettings& settings)
{
  m_fxImpl->SetBuffSettings(settings);
}

void ZoomFilterFx::Start()
{
}

void ZoomFilterFx::Finish()
{
}

void ZoomFilterFx::Log(const StatsLogValueFunc& logVal) const
{
  m_fxImpl->Log(logVal);
}

auto ZoomFilterFx::GetFxName() const -> std::string
{
  return "ZoomFilter FX";
}

void ZoomFilterFx::ZoomFilterFastRgb(const PixelBuffer& pix1,
                                     PixelBuffer& pix2,
                                     const ZoomFilterData* zf,
                                     const int switchIncr,
                                     const float switchMult)
{
  if (!m_enabled)
  {
    return;
  }

  m_fxImpl->ZoomFilterFastRgb(pix1, pix2, zf, switchIncr, switchMult);
}

auto ZoomFilterFx::GetFilterData() const -> const ZoomFilterData&
{
  return m_fxImpl->GetFilterData();
}

auto ZoomFilterFx::GetGeneralSpeed() const -> float
{
  return m_fxImpl->GetGeneralSpeed();
}

auto ZoomFilterFx::GetInterlaceStart() const -> int32_t
{
  return m_fxImpl->GetInterlaceStart();
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
void ZoomFilterFx::ZoomFilterImpl::ZoomFilterFastRgb(const PixelBuffer& pix1,
                                                     PixelBuffer& pix2,
                                                     const ZoomFilterData* zf,
                                                     const int32_t switchIncr,
                                                     const float switchMult)
{
  logDebug("switchIncr = {}, switchMult = {:.2}", switchIncr, switchMult);

  m_stats.UpdateStart();
  m_stats.DoZoomFilterFastRgb();

  // changement de taille
  if (m_interlaceStart != -2)
  {
    zf = nullptr;
  }

  // changement de config
  if (zf)
  {
    m_stats.DoZoomFilterFastRgbChangeConfig();
    m_filterData = *zf;
    m_generalSpeed = static_cast<float>(zf->vitesse - 128) / 128.0F;
    if (m_filterData.reverse)
    {
      m_generalSpeed = -m_generalSpeed;
    }
    m_interlaceStart = 0;
  }

  // generation du buffer de transform
  if (m_interlaceStart == -1)
  {
    m_stats.DoZoomFilterFastRgbInterlaceStartEqualMinus11();
    // sauvegarde de l'etat actuel dans la nouvelle source
    // save the current state in the new source
    // TODO: write that in MMX (has been done in previous version, but did not follow
    //   some new fonctionnalities)
    for (size_t i = 0; i < m_screenWidth * m_screenHeight; i++)
    {
      m_tranXSrce[i] += ((m_tranXDest[i] - m_tranXSrce[i]) * m_tranDiffFactor) >> BUFF_POINT_NUM;
      m_tranYSrce[i] += ((m_tranYDest[i] - m_tranYSrce[i]) * m_tranDiffFactor) >> BUFF_POINT_NUM;
    }
    m_tranDiffFactor = 0;
  }

  // TODO Why if repeated?
  if (m_interlaceStart == -1)
  {
    m_stats.DoZoomFilterFastRgbInterlaceStartEqualMinus12();

    std::swap(m_tranXDest, m_tranXTemp);
    std::swap(m_freeTranXDest, m_freeTranXTemp);

    std::swap(m_tranYDest, m_tranYTemp);
    std::swap(m_freeTranYDest, m_freeTranYTemp);

    m_interlaceStart = -2;
  }

  if (m_interlaceStart >= 0)
  {
    // creation de la nouvelle destination
    MakeZoomBufferStripe(m_screenHeight / BUFF_POINT_NUM);
  }

  if (switchIncr != 0)
  {
    m_stats.DoZoomFilterFastRgbSwitchIncrNotZero();

    m_tranDiffFactor += switchIncr;
    if (m_tranDiffFactor > BUFF_POINT_MASK)
    {
      m_tranDiffFactor = BUFF_POINT_MASK;
    }
  }

  // Equal was interesting but not correct!?
  //  if (std::fabs(1.0f - switchMult) < 0.0001)
  if (std::fabs(1.0F - switchMult) > 0.00001F)
  {
    m_stats.DoZoomFilterFastRgbSwitchIncrNotEqual1();

    m_tranDiffFactor =
        static_cast<int32_t>(static_cast<float>(BUFF_POINT_MASK) * (1.0F - switchMult) +
                             static_cast<float>(m_tranDiffFactor) * switchMult);
  }

  CZoom(pix1, pix2);

  m_stats.UpdateEnd();
}

void ZoomFilterFx::ZoomFilterImpl::GeneratePrecalCoef(Coeff2dArray& precalcCoeffs)
{
  for (uint32_t coefh = 0; coefh < BUFF_POINT_NUM; coefh++)
  {
    for (uint32_t coefv = 0; coefv < BUFF_POINT_NUM; coefv++)
    {
      const uint32_t diffcoeffh = SQRT_PERTE - coefh;
      const uint32_t diffcoeffv = SQRT_PERTE - coefv;

      if (!(coefh || coefv))
      {
        precalcCoeffs[coefh][coefv] = MAX_COLOR_VAL;
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
            /*.c*/ {
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
void ZoomFilterFx::ZoomFilterImpl::CZoom(const PixelBuffer& srceBuff, PixelBuffer& destBuff)
{
  m_stats.DoCZoom();

  const uint32_t tranAx = (m_screenWidth - 1) << PERTE_DEC;
  const uint32_t tranAy = (m_screenHeight - 1) << PERTE_DEC;

  const auto setDestPixelRow = [&](const uint32_t destY) {
    uint32_t destPos = m_screenWidth * destY;
#if __cplusplus <= 201402L
    const auto rowIter = destBuff.GetRowIter(destY);
    const auto rowBegin = std::get<0>(rowIter);
    const auto rowEnd = std::get<1>(rowIter);
#else
    const auto [rowBegin, rowEnd] = destBuff.GetRowIter(destY);
#endif
    for (auto rowBuff = rowBegin; rowBuff != rowEnd; ++rowBuff)
    {
#if __cplusplus <= 201402L
      const auto tranP = GetTransformedPoint(destPos);
      const auto tranPx = std::get<0>(tranP);
      const auto tranPy = std::get<1>(tranP);
#else
      const auto [tranPx, tranPy] = GetTransformedPoint(destPos);
#endif

      /**
      if ((tranPx >= tranAx) || (tranPy >= tranAy))
      {
        m_stats.DoCZoomOutOfRange();
        const size_t xIndex = GetRandInRange(0U, 1U + (tranAx & PERTE_MASK));
        const size_t yIndex = GetRandInRange(0U, 1U + (tranAy & PERTE_MASK));
        return std::make_tuple(destPos, CoeffArray{.intVal = m_precalcCoeffs[xIndex][yIndex]});
      }
     **/

      if ((tranPx >= tranAx) || (tranPy >= tranAy))
      {
        m_stats.DoCZoomOutOfRange();
        *rowBuff = Pixel::BLACK;
      }
      else
      {
#if __cplusplus <= 201402L
        const auto srceInfo = GetSourceInfo(tranPx, tranPy);
        const auto srceX = std::get<0>(srceInfo);
        const auto srceY = std::get<1>(srceInfo);
        const auto coeffs = std::get<2>(srceInfo);
#else
        const auto [srceX, srceY, coeffs] = GetSourceInfo(tranPx, tranPy);
#endif
        const PixelArray pixelNeighbours = srceBuff.Get4RHBNeighbours(srceX, srceY);
        *rowBuff = GetNewColor(coeffs, pixelNeighbours);
#ifndef NO_LOGGING
        if (colors[0].rgba() > 0xFF000000)
        {
          logInfo("srcePos == {}", srcePos);
          logInfo("destPos == {}", destPos);
          logInfo("tranPx >> perteDec == {}", tranPx >> perteDec);
          logInfo("tranPy >> perteDec == {}", tranPy >> perteDec);
          logInfo("tranPx == {}", tranPx);
          logInfo("tranPy == {}", tranPy);
          logInfo("tranPx & perteMask == {}", tranPx & perteMask);
          logInfo("tranPy & perteMask == {}", tranPy & perteMask);
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
      destPos++;
    }
  };

  m_parallel->ForLoop(m_screenHeight, setDestPixelRow);
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetTransformedPoint(const size_t buffPos) const
    -> std::tuple<uint32_t, uint32_t>
{
  return std::make_tuple(
      static_cast<uint32_t>(
          m_tranXSrce[buffPos] +
          (((m_tranXDest[buffPos] - m_tranXSrce[buffPos]) * m_tranDiffFactor) >> BUFF_POINT_NUM)),
      static_cast<uint32_t>(
          m_tranYSrce[buffPos] +
          (((m_tranYDest[buffPos] - m_tranYSrce[buffPos]) * m_tranDiffFactor) >> BUFF_POINT_NUM)));
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetSourceInfo(const uint32_t tranPx,
                                                        const uint32_t tranPy) const
    -> std::tuple<uint32_t, uint32_t, CoeffArray>
{
  const uint32_t srceX = tranPx >> PERTE_DEC;
  const uint32_t srceY = tranPy >> PERTE_DEC;
  const size_t xIndex = tranPx & PERTE_MASK;
  const size_t yIndex = tranPy & PERTE_MASK;
#if __cplusplus <= 201402L
  CoeffArray coeffs{};
  coeffs.intVal = m_precalcCoeffs[xIndex][yIndex];
  return std::make_tuple(srceX, srceY, coeffs);
#else
  return std::make_tuple(srceX, srceY, CoeffArray{.intVal = m_precalcCoeffs[xIndex][yIndex]});
#endif
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetNewColor(const CoeffArray& coeffs,
                                                      const PixelArray& pixels) const -> Pixel
{
  if (m_filterData.blockyWavy)
  {
    m_stats.DoGetBlockyMixedColor();
    return GetBlockyMixedColor(coeffs, pixels);
  }

  m_stats.DoGetMixedColor();
  return GetMixedColor(coeffs, pixels);
}

/*
 * Makes a stripe of a transform buffer
 *
 * The transform is (in order) :
 * Translation (-data->middleX, -data->middleY)
 * Homothetie (Center : 0,0   Coeff : 2/data->screenWidth)
 */
void ZoomFilterFx::ZoomFilterImpl::MakeZoomBufferStripe(const uint32_t interlaceIncrement)
{
  m_stats.DoMakeZoomBufferStripe();

  assert(m_interlaceStart >= 0);

  const float normMiddleX = ToNormalizedCoord(static_cast<int32_t>(m_filterData.middleX));
  const float normMiddleY = ToNormalizedCoord(static_cast<int32_t>(m_filterData.middleY));

  // Where (vertically) to stop generating the buffer stripe
  const int32_t maxEnd = std::min(static_cast<int32_t>(m_screenHeight),
                                  m_interlaceStart + static_cast<int32_t>(interlaceIncrement));

  // Position of the pixel to compute in pixmap coordinates
  const auto doStripeLine = [&](const uint32_t y) {
    const uint32_t yOffset = y + static_cast<uint32_t>(m_interlaceStart);
    const uint32_t yTimesScreenWidth = yOffset * m_screenWidth;
    const float normY = ToNormalizedCoord(static_cast<int32_t>(yOffset) -
                                          static_cast<int32_t>(m_filterData.middleY));
    float normX = -ToNormalizedCoord(static_cast<int32_t>(m_filterData.middleX));

    for (uint32_t x = 0; x < m_screenWidth; x++)
    {
      const V2dFlt vector = GetZoomVector(normX, normY);
      const uint32_t tranPos = yTimesScreenWidth + x;

      m_tranXTemp[tranPos] = ToPixmapCoord((normX - vector.x + normMiddleX) * BUFF_POINT_NUM_FLT);
      m_tranYTemp[tranPos] = ToPixmapCoord((normY - vector.y + normMiddleY) * BUFF_POINT_NUM_FLT);

      IncNormalizedCoord(normX);
    }
  };

  m_parallel->ForLoop(static_cast<uint32_t>(maxEnd - m_interlaceStart), doStripeLine);

  m_interlaceStart += static_cast<int32_t>(interlaceIncrement);
  if (maxEnd == static_cast<int32_t>(m_screenHeight))
  {
    m_interlaceStart = -1;
  }
}

void ZoomFilterFx::ZoomFilterImpl::GenerateWaterFxHorizontalBuffer()
{
  m_stats.DoGenerateWaterFxHorizontalBuffer();

  int32_t decc = GetRandInRange(-4, +4);
  int32_t spdc = GetRandInRange(-4, +4);
  int32_t accel = GetRandInRange(-4, +4);

  for (size_t loopv = m_screenHeight; loopv != 0;)
  {
    loopv--;
    m_firedec[loopv] = decc;
    decc += spdc / 10;
    spdc += GetRandInRange(-2, +3);

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
      spdc = spdc - static_cast<int32_t>(GetNRand(3)) + accel / 10;
    }
    if (spdc < -30)
    {
      spdc = spdc + static_cast<int32_t>(GetNRand(3)) + accel / 10;
    }

    if (decc > 8 && spdc > 1)
    {
      spdc -= GetRandInRange(-2, +1);
    }
    if (decc < -8 && spdc < -1)
    {
      spdc += static_cast<int32_t>(GetNRand(3)) + 2;
    }
    if (decc > 8 || decc < -8)
    {
      decc = decc * 8 / 9;
    }

    accel += GetRandInRange(-1, +2);
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

auto ZoomFilterFx::ZoomFilterImpl::GetZoomVector(const float normX, const float normY) -> V2dFlt
{
  m_stats.DoZoomVector();

  /* sx = (x < 0.0f) ? -1.0f : 1.0f;
     sy = (y < 0.0f) ? -1.0f : 1.0f;
   */
  float coefVitesse = (1.0F + m_generalSpeed) / 50.0F;

  // The Effects
  switch (m_filterData.mode)
  {
    case ZoomFilterMode::crystalBallMode:
    {
      m_stats.DoZoomVectorCrystalBallMode();
      coefVitesse -= m_filterData.crystalBallAmplitude * (SqDistance(normX, normY) - 0.3F);
      break;
    }
    case ZoomFilterMode::amuletMode:
    {
      m_stats.DoZoomVectorAmuletMode();
      coefVitesse += m_filterData.amuletAmplitude * SqDistance(normX, normY);
      break;
    }
    case ZoomFilterMode::waveMode:
    {
      m_stats.DoZoomVectorWaveMode();
      const float angle = SqDistance(normX, normY) * m_filterData.waveFreqFactor;
      float periodicPart;
      switch (m_filterData.waveEffectType)
      {
        case ZoomFilterData::WaveEffect::WAVE_SIN_EFFECT:
          periodicPart = std::sin(angle);
          break;
        case ZoomFilterData::WaveEffect::WAVE_COS_EFFECT:
          periodicPart = std::cos(angle);
          break;
        case ZoomFilterData::WaveEffect::WAVE_SIN_COS_EFFECT:
          periodicPart = std::sin(angle) + std::cos(angle);
          break;
        default:
          throw std::logic_error("Unknown WaveEffect enum");
      }
      coefVitesse += m_filterData.waveAmplitude * periodicPart;
      break;
    }
    case ZoomFilterMode::scrunchMode:
    {
      m_stats.DoZoomVectorScrunchMode();
      coefVitesse += m_filterData.scrunchAmplitude * SqDistance(normX, normY);
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
      m_stats.DoZoomVectorSpeedwayMode();
      coefVitesse *= m_filterData.speedwayAmplitude * normY;
      break;
    }
    default:
      m_stats.DoZoomVectorDefaultMode();
      break;
  }

  if (coefVitesse < MIN_COEF_VITESSE)
  {
    coefVitesse = MIN_COEF_VITESSE;
    m_stats.CoeffVitesseBelowMin();
  }
  else if (coefVitesse > MAX_COEF_VITESSE)
  {
    coefVitesse = MAX_COEF_VITESSE;
    m_stats.CoeffVitesseAboveMax();
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
  if (m_filterData.noisify)
  {
    m_stats.DoZoomVectorNoisify();
    if (m_filterData.noiseFactor > 0.01)
    {
      m_stats.DoZoomVectorNoiseFactor();
      //    const float xAmp = 1.0/getRandInRange(50.0f, 200.0f);
      //    const float yAmp = 1.0/getRandInRange(50.0f, 200.0f);
      const float amp = m_filterData.noiseFactor / GetRandInRange(NOISE_MIN, NOISE_MAX);
      m_filterData.noiseFactor *= 0.999;
      vx += amp * (GetRandInRange(0.0F, 1.0F) - 0.5F);
      vy += amp * (GetRandInRange(0.0F, 1.0F) - 0.5F);
    }
  }

  if (m_filterData.hypercosEffect != ZoomFilterData::HypercosEffect::none)
  {
    m_stats.DoZoomVectorHypercosEffect();
    float xVal{};
    float yVal{};
    switch (m_filterData.hypercosEffect)
    {
      case ZoomFilterData::HypercosEffect::none:
        break;
      case ZoomFilterData::HypercosEffect::sinRectangular:
        xVal = std::sin(m_filterData.hypercosFreqX * normX);
        yVal = std::sin(m_filterData.hypercosFreqY * normY);
        break;
      case ZoomFilterData::HypercosEffect::cosRectangular:
        xVal = std::cos(m_filterData.hypercosFreqX * normX);
        yVal = std::cos(m_filterData.hypercosFreqY * normY);
        break;
      case ZoomFilterData::HypercosEffect::sinCurlSwirl:
        xVal = std::sin(m_filterData.hypercosFreqY * normY);
        yVal = std::sin(m_filterData.hypercosFreqX * normX);
        break;
      case ZoomFilterData::HypercosEffect::cosCurlSwirl:
        xVal = std::cos(m_filterData.hypercosFreqY * normY);
        yVal = std::cos(m_filterData.hypercosFreqX * normX);
        break;
      case ZoomFilterData::HypercosEffect::sinCosCurlSwirl:
        xVal = std::sin(m_filterData.hypercosFreqX * normY);
        yVal = std::cos(m_filterData.hypercosFreqY * normX);
        break;
      case ZoomFilterData::HypercosEffect::cosSinCurlSwirl:
        xVal = std::cos(m_filterData.hypercosFreqY * normY);
        yVal = std::sin(m_filterData.hypercosFreqX * normX);
        break;
      default:
        throw std::logic_error("Unknown filterData.hypercosEffect value");
    }
    vx += m_filterData.hypercosAmplitudeX * xVal;
    vy += m_filterData.hypercosAmplitudeY * yVal;
  }

  if (m_filterData.hPlaneEffect)
  {
    m_stats.DoZoomVectorHPlaneEffect();

    // TODO - try normX
    vx +=
        normY * m_filterData.hPlaneEffectAmplitude * static_cast<float>(m_filterData.hPlaneEffect);
  }

  if (m_filterData.vPlaneEffect)
  {
    m_stats.DoZoomVectorVPlaneEffect();
    vy +=
        normX * m_filterData.vPlaneEffectAmplitude * static_cast<float>(m_filterData.vPlaneEffect);
  }

  /* TODO : Water Mode */
  //    if (data->waveEffect)

  // avoid null displacement
  if (std::fabs(vx) < m_minNormCoordVal)
  {
    vx = (vx < 0.0F) ? -m_minNormCoordVal : m_minNormCoordVal;
  }
  if (std::fabs(vy) < m_minNormCoordVal)
  {
    vy = (vy < 0.0F) ? -m_minNormCoordVal : m_minNormCoordVal;
  }

  return V2dFlt{vx, vy};
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetMixedColor(const CoeffArray& coeffs,
                                                        const PixelArray& colors) const -> Pixel
{
  if (coeffs.intVal == 0)
  {
    return Pixel::BLACK;
  }

  uint32_t newR = 0;
  uint32_t newG = 0;
  uint32_t newB = 0;
  for (size_t i = 0; i < NUM_COEFFS; i++)
  {
    const auto coeff = static_cast<uint32_t>(coeffs.c[i]);
    newR += static_cast<uint32_t>(colors[i].R()) * coeff;
    newG += static_cast<uint32_t>(colors[i].G()) * coeff;
    newB += static_cast<uint32_t>(colors[i].B()) * coeff;
  }
  newR >>= 8;
  newG >>= 8;
  newB >>= 8;

  if (m_buffSettings.allowOverexposed)
  {
    return Pixel{{/*.r = */ static_cast<uint8_t>((newR & 0xffffff00) ? 0xff : newR),
                  /*.g = */ static_cast<uint8_t>((newG & 0xffffff00) ? 0xff : newG),
                  /*.b = */ static_cast<uint8_t>((newB & 0xffffff00) ? 0xff : newB),
                  /*.a = */ 0xff}};
  }

  const uint32_t maxVal = std::max({newR, newG, newB});
  if (maxVal > channel_limits<uint32_t>::max())
  {
    // scale all channels back
    newR = (newR << 8) / maxVal;
    newG = (newG << 8) / maxVal;
    newB = (newB << 8) / maxVal;
  }

  return Pixel{{/*.r = */ static_cast<uint8_t>(newR),
                /*.g = */ static_cast<uint8_t>(newG),
                /*.b = */ static_cast<uint8_t>(newB),
                /*.a = */ 0xff}};
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetBlockyMixedColor(const CoeffArray& coeffs,
                                                              const PixelArray& colors) const
    -> Pixel
{
  // Changing the color order gives a strange blocky, wavy look.
  // The order col4, col3, col2, col1 gave a black tear - no so good.
  const PixelArray reorderedColors{colors[0], colors[2], colors[1], colors[3]};
  return GetMixedColor(coeffs, reorderedColors);
}

auto ZoomFilterFx::ZoomFilterImpl::GetFilterData() const -> const ZoomFilterData&
{
  return m_filterData;
}

auto ZoomFilterFx::ZoomFilterImpl::GetGeneralSpeed() const -> float
{
  return m_generalSpeed;
}

auto ZoomFilterFx::ZoomFilterImpl::GetInterlaceStart() const -> int32_t
{
  return m_interlaceStart;
}

} // namespace GOOM
