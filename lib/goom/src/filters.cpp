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
 *
 *	- converted to C++14 2021-02-01 (glk)
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

static constexpr size_t NUM_NEIGHBOR_COEFFS = 4;
using NeighborhoodCoeffArray = union
{
  std::array<uint8_t, NUM_NEIGHBOR_COEFFS> c;
  uint32_t intVal = 0;
};
using NeighborhoodPixelArray = std::array<Pixel, NUM_NEIGHBOR_COEFFS>;

constexpr int32_t DIM_FILTER_COEFFS = 16;
static const int32_t MAX_TRAN_DIFF_FACTOR =
    static_cast<int32_t>(std::lround(std::pow(2, DIM_FILTER_COEFFS))) - 1;
using FilterCoeff2dArray =
    std::array<std::array<NeighborhoodCoeffArray, DIM_FILTER_COEFFS>, DIM_FILTER_COEFFS>;

constexpr float MIN_SCREEN_COORD_VAL = 1.0F / static_cast<float>(DIM_FILTER_COEFFS);

inline auto TranToScreenCoord(const uint32_t tranCoord) -> uint32_t
{
  return tranCoord / DIM_FILTER_COEFFS;
}

inline auto ScreenToTranCoord(const uint32_t screenCoord) -> uint32_t
{
  return screenCoord * DIM_FILTER_COEFFS;
}

inline auto ScreenToTranCoord(const float screenCoord) -> uint32_t
{
  // IMPORTANT: Without 'lround' a faint cross artifact appears in the centre of the screen.
  return static_cast<uint32_t>(std::lround(screenCoord * static_cast<float>(DIM_FILTER_COEFFS)));
}

inline auto TranToCoeffIndexCoord(const uint32_t tranCoord)
{
  return tranCoord % DIM_FILTER_COEFFS;
}

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

  [[nodiscard]] auto GetResourcesDirectory() const -> const std::string&;
  void SetResourcesDirectory(const std::string& dirName);
  void SetBuffSettings(const FXBuffSettings& settings);

  void ZoomFilterFastRgb(const PixelBuffer& pix1,
                         PixelBuffer& pix2,
                         const ZoomFilterData* zf,
                         int32_t switchIncr,
                         float switchMult,
                         uint32_t& numClipped);

  auto GetFilterData() const -> const ZoomFilterData&;
  auto GetGeneralSpeed() const -> float;
  auto GetTranBuffYLineStart() const -> uint32_t;

  void Log(const StatsLogValueFunc& l) const;

private:
  const uint32_t m_screenWidth;
  const uint32_t m_screenHeight;
  const uint32_t m_bufferSize;
  const uint32_t m_maxTranX;
  const uint32_t m_maxTranY;
  const float m_ratioScreenToNormalizedCoord;
  const float m_ratioNormalizedToScreenCoord;
  const float m_minNormalizedCoordVal;
  ZoomFilterData m_filterData{};
  FXBuffSettings m_buffSettings{};
  std::string m_resourcesDirectory{};
  float m_generalSpeed;
  mutable FilterStats m_stats{};

  Parallel* const m_parallel;

  auto NormalizedToScreenCoordFlt(float normalizedCoord) const -> float;
  auto NormalizedToTranCoord(float normalizedCoord) const -> int32_t;
  auto ScreenToNormalizedCoord(int32_t screenCoord) const -> float;
  void IncNormalizedCoord(float& normalizedCoord) const;

  std::vector<int32_t> m_tranXSrce{};
  std::vector<int32_t> m_tranYSrce{};
  std::vector<int32_t> m_tranXDest{};
  std::vector<int32_t> m_tranYDest{};
  std::vector<int32_t> m_tranXTemp{};
  std::vector<int32_t> m_tranYTemp{};
  uint32_t m_tranBuffYLineStart;
  enum class TranBuffState
  {
    RESET_FILTER_CONFIG,
    RESTART_BUFF_YLINE,
    BUFF_READY,
  };
  TranBuffState m_tranBuffState;
  // modification by jeko : fixedpoint : tranDiffFactor = (16:16) (0 <= tranDiffFactor <= 2^16)
  int32_t m_tranDiffFactor; // in [0, BUFF_POINT_MASK]
  auto GetTranXBuffSrceDestLerp(size_t buffPos) -> uint32_t;
  auto GetTranYBuffSrceDestLerp(size_t buffPos) -> uint32_t;
  static auto GetTranBuffLerp(int32_t srceBuffVal, int32_t destBuffVal, int32_t t) -> uint32_t;
  auto GetSourceInfo(uint32_t tranX, uint32_t tranY) const
      -> std::tuple<uint32_t, uint32_t, NeighborhoodCoeffArray>;
  auto GetNewColor(const NeighborhoodCoeffArray& coeffs, const NeighborhoodPixelArray& pixels) const
      -> Pixel;

  // modif d'optim by Jeko : precalcul des 4 coefs resultant des 2 pos
  const FilterCoeff2dArray m_precalculatedCoeffs;
  static auto GetPrecalculatedCoeffs() -> FilterCoeff2dArray;

  std::vector<int32_t> m_firedec{};

  void CZoom(const PixelBuffer& srceBuff, PixelBuffer& destBuff, uint32_t& numDestClipped);
  void AssignBufferStripeToTranTemp(uint32_t tranBuffYLineIncrement);
  void GenerateWaterFxHorizontalBuffer();
  auto GetZoomVector(float xNormalized, float yNormalized) -> V2dFlt;
  auto GetStandardVelocity(float xNormalized, float yNormalized) -> V2dFlt;
  auto GetHypercosVelocity(float xNormalized, float yNormalized) const -> V2dFlt;
  auto GetHPlaneEffectVelocity(float xNormalized, float yNormalized) const -> float;
  auto GetVPlaneEffectVelocity(float xNormalized, float yNormalized) const -> float;
  auto GetNoiseVelocity() -> V2dFlt;
  void AvoidNullDisplacement(V2dFlt& velocity) const;
  auto GetMixedColor(const NeighborhoodCoeffArray& coeffs,
                     const NeighborhoodPixelArray& colors) const -> Pixel;
  auto GetBlockyMixedColor(const NeighborhoodCoeffArray& coeffs,
                           const NeighborhoodPixelArray& colors) const -> Pixel;
};

ZoomFilterFx::ZoomFilterFx(Parallel& p, const std::shared_ptr<const PluginInfo>& info) noexcept
  : m_fxImpl{new ZoomFilterImpl{p, info}}
{
}

ZoomFilterFx::~ZoomFilterFx() noexcept = default;

auto ZoomFilterFx::GetResourcesDirectory() const -> const std::string&
{
  return m_fxImpl->GetResourcesDirectory();
}

void ZoomFilterFx::SetResourcesDirectory(const std::string& dirName)
{
  m_fxImpl->SetResourcesDirectory(dirName);
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
                                     const float switchMult,
                                     uint32_t& numClipped)
{
  if (!m_enabled)
  {
    return;
  }

  m_fxImpl->ZoomFilterFastRgb(pix1, pix2, zf, switchIncr, switchMult, numClipped);
}

auto ZoomFilterFx::GetFilterData() const -> const ZoomFilterData&
{
  return m_fxImpl->GetFilterData();
}

auto ZoomFilterFx::GetGeneralSpeed() const -> float
{
  return m_fxImpl->GetGeneralSpeed();
}

auto ZoomFilterFx::GetTranBuffYLineStart() const -> uint32_t
{
  return m_fxImpl->GetTranBuffYLineStart();
}

ZoomFilterFx::ZoomFilterImpl::ZoomFilterImpl(Parallel& p,
                                             const std::shared_ptr<const PluginInfo>& goomInfo)
  : m_screenWidth{goomInfo->GetScreenInfo().width},
    m_screenHeight{goomInfo->GetScreenInfo().height},
    m_bufferSize{goomInfo->GetScreenInfo().size},
    m_maxTranX{ScreenToTranCoord(m_screenWidth - 1)},
    m_maxTranY{ScreenToTranCoord(m_screenHeight - 1)},
    m_ratioScreenToNormalizedCoord{2.0F / static_cast<float>(m_screenWidth)},
    m_ratioNormalizedToScreenCoord{1.0F / m_ratioScreenToNormalizedCoord},
    m_minNormalizedCoordVal{m_ratioScreenToNormalizedCoord * MIN_SCREEN_COORD_VAL},
    m_generalSpeed{0},
    m_parallel{&p},
    m_tranXSrce(m_bufferSize),
    m_tranYSrce(m_bufferSize),
    m_tranXDest(m_bufferSize),
    m_tranYDest(m_bufferSize),
    m_tranXTemp(m_bufferSize),
    m_tranYTemp(m_bufferSize),
    m_tranBuffYLineStart{0},
    m_tranBuffState{TranBuffState::BUFF_READY},
    m_tranDiffFactor{0},
    m_precalculatedCoeffs{GetPrecalculatedCoeffs()},
    m_firedec(m_screenHeight)
{
  m_filterData.middleX = m_screenWidth / 2;
  m_filterData.middleY = m_screenHeight / 2;

  GenerateWaterFxHorizontalBuffer();
  AssignBufferStripeToTranTemp(m_screenHeight);

  // Copy the data from temp to dest and source
  std::copy(m_tranXTemp.begin(), m_tranXTemp.end(), m_tranXSrce.begin());
  std::copy(m_tranYTemp.begin(), m_tranYTemp.end(), m_tranYSrce.begin());
  std::copy(m_tranXTemp.begin(), m_tranXTemp.end(), m_tranXDest.begin());
  std::copy(m_tranYTemp.begin(), m_tranYTemp.end(), m_tranYDest.begin());
}

ZoomFilterFx::ZoomFilterImpl::~ZoomFilterImpl() noexcept = default;

void ZoomFilterFx::ZoomFilterImpl::Log(const StatsLogValueFunc& l) const
{
  m_stats.SetLastGeneralSpeed(m_generalSpeed);
  m_stats.SetLastPrevX(m_screenWidth);
  m_stats.SetLastPrevY(m_screenHeight);
  m_stats.SetLastTranBuffYLineStart(m_tranBuffYLineStart);
  m_stats.SetLastTranDiffFactor(m_tranDiffFactor);

  m_stats.Log(l);
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetResourcesDirectory() const -> const std::string&
{
  return m_resourcesDirectory;
}

inline void ZoomFilterFx::ZoomFilterImpl::SetResourcesDirectory(const std::string& dirName)
{
  m_resourcesDirectory = dirName;
}

inline void ZoomFilterFx::ZoomFilterImpl::SetBuffSettings(const FXBuffSettings& settings)
{
  m_buffSettings = settings;
}

auto ZoomFilterFx::ZoomFilterImpl::GetFilterData() const -> const ZoomFilterData&
{
  return m_filterData;
}

auto ZoomFilterFx::ZoomFilterImpl::GetGeneralSpeed() const -> float
{
  return m_generalSpeed;
}

auto ZoomFilterFx::ZoomFilterImpl::GetTranBuffYLineStart() const -> uint32_t
{
  return m_tranBuffYLineStart;
}

inline void ZoomFilterFx::ZoomFilterImpl::IncNormalizedCoord(float& normalizedCoord) const
{
  normalizedCoord += m_ratioScreenToNormalizedCoord;
}

inline auto ZoomFilterFx::ZoomFilterImpl::ScreenToNormalizedCoord(const int32_t screenCoord) const
    -> float
{
  return m_ratioScreenToNormalizedCoord * static_cast<float>(screenCoord);
}

inline auto ZoomFilterFx::ZoomFilterImpl::NormalizedToScreenCoordFlt(
    const float normalizedCoord) const -> float
{
  return m_ratioNormalizedToScreenCoord * normalizedCoord;
}

inline auto ZoomFilterFx::ZoomFilterImpl::NormalizedToTranCoord(const float normalizedCoord) const
    -> int32_t
{
  // IMPORTANT: Without 'lround' a faint cross artifact appears in the centre of the screen.
  return static_cast<int32_t>(
      std::lround(ScreenToTranCoord(NormalizedToScreenCoordFlt(normalizedCoord))));
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetTranXBuffSrceDestLerp(const size_t buffPos) -> uint32_t
{
  return GetTranBuffLerp(m_tranXSrce[buffPos], m_tranXDest[buffPos], m_tranDiffFactor);
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetTranYBuffSrceDestLerp(const size_t buffPos) -> uint32_t
{
  return GetTranBuffLerp(m_tranYSrce[buffPos], m_tranYDest[buffPos], m_tranDiffFactor);
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetTranBuffLerp(const int32_t srceBuffVal,
                                                          const int32_t destBuffVal,
                                                          const int32_t t) -> uint32_t
{
  return static_cast<uint32_t>(srceBuffVal +
                               ((t * (destBuffVal - srceBuffVal)) >> DIM_FILTER_COEFFS));
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetSourceInfo(const uint32_t tranX,
                                                        const uint32_t tranY) const
    -> std::tuple<uint32_t, uint32_t, NeighborhoodCoeffArray>
{
  const uint32_t srceX = TranToScreenCoord(tranX);
  const uint32_t srceY = TranToScreenCoord(tranY);
  const size_t xIndex = TranToCoeffIndexCoord(tranX);
  const size_t yIndex = TranToCoeffIndexCoord(tranY);
  return std::make_tuple(srceX, srceY, m_precalculatedCoeffs[xIndex][yIndex]);
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetNewColor(const NeighborhoodCoeffArray& coeffs,
                                                      const NeighborhoodPixelArray& pixels) const
    -> Pixel
{
  if (m_filterData.blockyWavy)
  {
    // m_stats.DoGetBlockyMixedColor();
    return GetBlockyMixedColor(coeffs, pixels);
  }

  // m_stats.DoGetMixedColor();
  return GetMixedColor(coeffs, pixels);
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetBlockyMixedColor(
    const NeighborhoodCoeffArray& coeffs, const NeighborhoodPixelArray& colors) const -> Pixel
{
  // Changing the color order gives a strange blocky, wavy look.
  // The order col4, col3, col2, col1 gave a black tear - no so good.
  static_assert(NUM_NEIGHBOR_COEFFS == 4, "NUM_NEIGHBOR_COEFFS must be 4.");
  const NeighborhoodPixelArray reorderedColors{colors[0], colors[2], colors[1], colors[3]};
  return GetMixedColor(coeffs, reorderedColors);
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetMixedColor(const NeighborhoodCoeffArray& coeffs,
                                                        const NeighborhoodPixelArray& colors) const
    -> Pixel
{
  if (coeffs.intVal == 0)
  {
    return Pixel::BLACK;
  }

  uint32_t newR = 0;
  uint32_t newG = 0;
  uint32_t newB = 0;
  for (size_t i = 0; i < NUM_NEIGHBOR_COEFFS; i++)
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

auto ZoomFilterFx::ZoomFilterImpl::GetPrecalculatedCoeffs() -> FilterCoeff2dArray
{
  FilterCoeff2dArray precalculatedCoeffs{};

  for (uint32_t coeffH = 0; coeffH < DIM_FILTER_COEFFS; coeffH++)
  {
    for (uint32_t coeffV = 0; coeffV < DIM_FILTER_COEFFS; coeffV++)
    {
      const uint32_t diffCoeffH = DIM_FILTER_COEFFS - coeffH;
      const uint32_t diffCoeffV = DIM_FILTER_COEFFS - coeffV;

      if (!(coeffH || coeffV))
      {
        precalculatedCoeffs[coeffH][coeffV].intVal = MAX_COLOR_VAL;
      }
      else
      {
        uint32_t i1 = diffCoeffH * diffCoeffV;
        uint32_t i2 = coeffH * diffCoeffV;
        uint32_t i3 = diffCoeffH * coeffV;
        uint32_t i4 = coeffH * coeffV;

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

        precalculatedCoeffs[coeffH][coeffV] = NeighborhoodCoeffArray{/*.c*/ {
            static_cast<uint8_t>(i1),
            static_cast<uint8_t>(i2),
            static_cast<uint8_t>(i3),
            static_cast<uint8_t>(i4),
        }};
      }
    }
  }

  return precalculatedCoeffs;
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
                                                     const ZoomFilterData* const zf,
                                                     const int32_t switchIncr,
                                                     const float switchMult,
                                                     uint32_t& numClipped)
{
  m_stats.UpdateStart();
  m_stats.DoZoomFilterFastRgb();

  // changement de taille
  if (m_tranBuffState == TranBuffState::RESET_FILTER_CONFIG)
  {
    if (zf)
    {
      m_stats.DoZoomFilterChangeConfig();
      m_filterData = *zf;
      m_generalSpeed = static_cast<float>(zf->vitesse - 128) / 128.0F;
      if (m_filterData.reverse)
      {
        m_generalSpeed = -m_generalSpeed;
      }
      m_tranBuffYLineStart = 0;
      m_tranBuffState = TranBuffState::BUFF_READY;
    }
  }
  else if (m_tranBuffState == TranBuffState::RESTART_BUFF_YLINE)
  {
    // generation du buffer de transform
    m_stats.DoZoomFilterRestartTranBuffYLine();

    // sauvegarde de l'etat actuel dans la nouvelle source
    // Save the current state in the source buffs.
    for (size_t i = 0; i < m_screenWidth * m_screenHeight; i++)
    {
      m_tranXSrce[i] = static_cast<int32_t>(GetTranXBuffSrceDestLerp(i));
      m_tranYSrce[i] = static_cast<int32_t>(GetTranYBuffSrceDestLerp(i));
    }
    m_tranDiffFactor = 0;

    // Set up the next dest buffs from the last buffer stripe.
    std::swap(m_tranXDest, m_tranXTemp);
    std::swap(m_tranYDest, m_tranYTemp);

    m_tranBuffYLineStart = 0;
    m_tranBuffState = TranBuffState::RESET_FILTER_CONFIG;
  }
  else
  {
    // creation de la nouvelle destination
    AssignBufferStripeToTranTemp(m_screenHeight / DIM_FILTER_COEFFS);
  }

  if (switchIncr != 0)
  {
    m_stats.DoZoomFilterSwitchIncrNotZero();

    m_tranDiffFactor += switchIncr;
    if (m_tranDiffFactor > MAX_TRAN_DIFF_FACTOR)
    {
      m_tranDiffFactor = MAX_TRAN_DIFF_FACTOR;
    }
  }

  if (!floats_equal(switchMult, 1.0F))
  {
    m_stats.DoZoomFilterSwitchMultNotEqual1();

    m_tranDiffFactor =
        static_cast<int32_t>(stdnew::lerp(static_cast<float>(MAX_TRAN_DIFF_FACTOR),
                                          static_cast<float>(m_tranDiffFactor), switchMult));
  }

  CZoom(pix1, pix2, numClipped);

  m_stats.UpdateEnd();
}

// pure c version of the zoom filter
void ZoomFilterFx::ZoomFilterImpl::CZoom(const PixelBuffer& srceBuff,
                                         PixelBuffer& destBuff,
                                         uint32_t& numDestClipped)
{
  m_stats.DoCZoom();

  numDestClipped = 0;

  const auto setDestPixelRow = [&](const uint32_t destY) {
    uint32_t destPos = m_screenWidth * destY;
#if __cplusplus <= 201402L
    const auto destRowIter = destBuff.GetRowIter(destY);
    const auto destRowBegin = std::get<0>(destRowIter);
    const auto destRowEnd = std::get<1>(destRowIter);
#else
    const auto [destRowBegin, destRowEnd] = destBuff.GetRowIter(destY);
#endif
    for (auto destRowBuff = destRowBegin; destRowBuff != destRowEnd; ++destRowBuff)
    {
      const uint32_t tranX = GetTranXBuffSrceDestLerp(destPos);
      const uint32_t tranY = GetTranYBuffSrceDestLerp(destPos);

      /**
      if ((tranPx >= m_maxTranX) || (tranPy >= m_maxTranY))
      {
        m_stats.DoCZoomOutOfRange();
        const size_t xIndex = GetRandInRange(0U, 1U + (tranAx & PERTE_MASK));
        const size_t yIndex = GetRandInRange(0U, 1U + (tranAy & PERTE_MASK));
        return std::make_tuple(destPos, CoeffArray{.intVal = m_precalcCoeffs[xIndex][yIndex]});
      }
     **/

      if ((tranX >= m_maxTranX) || (tranY >= m_maxTranY))
      {
        m_stats.DoCZoomOutOfRange();
        *destRowBuff = Pixel::BLACK;
        numDestClipped++;
      }
      else
      {
#if __cplusplus <= 201402L
        const auto srceInfo = GetSourceInfo(tranX, tranY);
        const auto srceX = std::get<0>(srceInfo);
        const auto srceY = std::get<1>(srceInfo);
        const auto coeffs = std::get<2>(srceInfo);
#else
        const auto [srceX, srceY, coeffs] = GetSourceInfo(tranPx, tranPy);
#endif
        const NeighborhoodPixelArray pixelNeighbours = srceBuff.Get4RHBNeighbours(srceX, srceY);
        *destRowBuff = GetNewColor(coeffs, pixelNeighbours);
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

/*
 * Makes a stripe of a transform buffer
 *
 * The transform is (in order) :
 * Translation (-data->middleX, -data->middleY)
 * Homothetie (Center : 0,0   Coeff : 2/data->screenWidth)
 */
void ZoomFilterFx::ZoomFilterImpl::AssignBufferStripeToTranTemp(uint32_t tranBuffYLineIncrement)
{
  m_stats.DoMakeZoomBufferStripe();

  assert(m_tranBuffState == TranBuffState::BUFF_READY);

  const float xMidNormalized = ScreenToNormalizedCoord(static_cast<int32_t>(m_filterData.middleX));
  const float yMidNormalized = ScreenToNormalizedCoord(static_cast<int32_t>(m_filterData.middleY));

  const auto doStripeLine = [&](const uint32_t y) {
    // Position of the pixel to compute in screen coordinates
    const uint32_t yOffset = y + m_tranBuffYLineStart;
    const uint32_t tranPosStart = yOffset * m_screenWidth;
    const float yNormalized = ScreenToNormalizedCoord(static_cast<int32_t>(yOffset) -
                                                      static_cast<int32_t>(m_filterData.middleY));
    float xNormalized = ScreenToNormalizedCoord(-static_cast<int32_t>(m_filterData.middleX));

    for (uint32_t x = 0; x < m_screenWidth; x++)
    {
      const uint32_t tranPos = tranPosStart + x;
      const V2dFlt zoomVector = GetZoomVector(xNormalized, yNormalized);

      m_tranXTemp[tranPos] = NormalizedToTranCoord((xMidNormalized + xNormalized - zoomVector.x));
      m_tranYTemp[tranPos] = NormalizedToTranCoord((yMidNormalized + yNormalized - zoomVector.y));

      IncNormalizedCoord(xNormalized);
    }
  };

  // Where (vertically) to stop generating the buffer stripe
  const uint32_t tranBuffYLineEnd =
      std::min(m_screenHeight, m_tranBuffYLineStart + tranBuffYLineIncrement);

  m_parallel->ForLoop(tranBuffYLineEnd - static_cast<uint32_t>(m_tranBuffYLineStart), doStripeLine);

  m_tranBuffYLineStart += tranBuffYLineIncrement;
  if (tranBuffYLineEnd == m_screenHeight)
  {
    m_tranBuffState = TranBuffState::RESTART_BUFF_YLINE;
    m_tranBuffYLineStart = 0;
  }
}

auto ZoomFilterFx::ZoomFilterImpl::GetZoomVector(const float xNormalized, const float yNormalized)
    -> V2dFlt
{
  m_stats.DoZoomVector();

  V2dFlt velocity = GetStandardVelocity(xNormalized, yNormalized);

  // The Effects add-ons...

  if (m_filterData.noisify)
  {
    m_stats.DoZoomVectorNoisify();
    const V2dFlt noiseVelocity = GetNoiseVelocity();
    velocity.x += noiseVelocity.x;
    velocity.y += noiseVelocity.y;
  }

  if (m_filterData.hypercosEffect != ZoomFilterData::HypercosEffect::none)
  {
    m_stats.DoZoomVectorHypercosEffect();
    const V2dFlt hypercosVelocity = GetHypercosVelocity(xNormalized, yNormalized);
    velocity.x += hypercosVelocity.x;
    velocity.y += hypercosVelocity.y;
  }

  if (m_filterData.hPlaneEffect)
  {
    m_stats.DoZoomVectorHPlaneEffect();
    velocity.x += GetHPlaneEffectVelocity(xNormalized, yNormalized);
  }

  if (m_filterData.vPlaneEffect)
  {
    m_stats.DoZoomVectorVPlaneEffect();
    velocity.y += GetVPlaneEffectVelocity(xNormalized, yNormalized);
  }

  /* TODO : Water Mode */
  //    if (data->waveEffect)

  AvoidNullDisplacement(velocity);

  return velocity;
}

inline void ZoomFilterFx::ZoomFilterImpl::AvoidNullDisplacement(V2dFlt& velocity) const
{
  if (std::fabs(velocity.x) < m_minNormalizedCoordVal)
  {
    velocity.x = (velocity.x < 0.0F) ? -m_minNormalizedCoordVal : m_minNormalizedCoordVal;
  }
  if (std::fabs(velocity.y) < m_minNormalizedCoordVal)
  {
    velocity.y = (velocity.y < 0.0F) ? -m_minNormalizedCoordVal : m_minNormalizedCoordVal;
  }
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetStandardVelocity(const float xNormalized,
                                                              const float yNormalized) -> V2dFlt
{
  float coeffVitesse = (1.0F + m_generalSpeed) / 50.0F;

  // The Effects
  switch (m_filterData.mode)
  {
    case ZoomFilterMode::crystalBallMode:
    {
      m_stats.DoZoomVectorCrystalBallMode();
      coeffVitesse -=
          m_filterData.crystalBallAmplitude * (SqDistance(xNormalized, yNormalized) - 0.3F);
      break;
    }
    case ZoomFilterMode::amuletMode:
    {
      m_stats.DoZoomVectorAmuletMode();
      coeffVitesse += m_filterData.amuletAmplitude * SqDistance(xNormalized, yNormalized);
      break;
    }
    case ZoomFilterMode::waveMode:
    {
      m_stats.DoZoomVectorWaveMode();
      const float angle = SqDistance(xNormalized, yNormalized) * m_filterData.waveFreqFactor;
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
      coeffVitesse += m_filterData.waveAmplitude * periodicPart;
      break;
    }
    case ZoomFilterMode::scrunchMode:
    {
      m_stats.DoZoomVectorScrunchMode();
      coeffVitesse += m_filterData.scrunchAmplitude * SqDistance(xNormalized, yNormalized);
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
      coeffVitesse *= m_filterData.speedwayAmplitude * yNormalized;
      break;
    }
    /* Amulette 2 */
    // vx = X * tan(dist);
    // vy = Y * tan(dist);

    /* Rotate */
    //vx = (X+Y)*0.1;
    //vy = (Y-X)*0.1;
    default:
      m_stats.DoZoomVectorDefaultMode();
      break;
  }

  if (coeffVitesse < MIN_COEF_VITESSE)
  {
    coeffVitesse = MIN_COEF_VITESSE;
    m_stats.CoeffVitesseBelowMin();
  }
  else if (coeffVitesse > MAX_COEF_VITESSE)
  {
    coeffVitesse = MAX_COEF_VITESSE;
    m_stats.CoeffVitesseAboveMax();
  }

  return {coeffVitesse * xNormalized, coeffVitesse * yNormalized};
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetHypercosVelocity(const float xNormalized,
                                                              const float yNormalized) const
    -> V2dFlt
{
  float xVal = 0;
  float yVal = 0;

  switch (m_filterData.hypercosEffect)
  {
    case ZoomFilterData::HypercosEffect::none:
      break;
    case ZoomFilterData::HypercosEffect::sinRectangular:
      xVal = std::sin(m_filterData.hypercosFreqX * xNormalized);
      yVal = std::sin(m_filterData.hypercosFreqY * yNormalized);
      break;
    case ZoomFilterData::HypercosEffect::cosRectangular:
      xVal = std::cos(m_filterData.hypercosFreqX * xNormalized);
      yVal = std::cos(m_filterData.hypercosFreqY * yNormalized);
      break;
    case ZoomFilterData::HypercosEffect::sinCurlSwirl:
      xVal = std::sin(m_filterData.hypercosFreqY * yNormalized);
      yVal = std::sin(m_filterData.hypercosFreqX * xNormalized);
      break;
    case ZoomFilterData::HypercosEffect::cosCurlSwirl:
      xVal = std::cos(m_filterData.hypercosFreqY * yNormalized);
      yVal = std::cos(m_filterData.hypercosFreqX * xNormalized);
      break;
    case ZoomFilterData::HypercosEffect::sinCosCurlSwirl:
      xVal = std::sin(m_filterData.hypercosFreqX * yNormalized);
      yVal = std::cos(m_filterData.hypercosFreqY * xNormalized);
      break;
    case ZoomFilterData::HypercosEffect::cosSinCurlSwirl:
      xVal = std::cos(m_filterData.hypercosFreqY * yNormalized);
      yVal = std::sin(m_filterData.hypercosFreqX * xNormalized);
      break;
    default:
      throw std::logic_error("Unknown filterData.hypercosEffect value");
  }

  return {m_filterData.hypercosAmplitudeX * xVal, m_filterData.hypercosAmplitudeY * yVal};
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetHPlaneEffectVelocity(
    [[maybe_unused]] const float xNormalized, const float yNormalized) const -> float
{
  // TODO - try xNormalized
  return yNormalized * m_filterData.hPlaneEffectAmplitude *
         static_cast<float>(m_filterData.hPlaneEffect);
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetVPlaneEffectVelocity(
    [[maybe_unused]] const float xNormalized, [[maybe_unused]] const float yNormalized) const
    -> float
{
  // TODO - try yNormalized
  return xNormalized * m_filterData.vPlaneEffectAmplitude *
         static_cast<float>(m_filterData.vPlaneEffect);
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetNoiseVelocity() -> V2dFlt
{
  if (m_filterData.noiseFactor < 0.01)
  {
    return {0, 0};
  }

  m_stats.DoZoomVectorNoiseFactor();
  //    const float xAmp = 1.0/getRandInRange(50.0f, 200.0f);
  //    const float yAmp = 1.0/getRandInRange(50.0f, 200.0f);
  const float amp = m_filterData.noiseFactor / GetRandInRange(NOISE_MIN, NOISE_MAX);
  m_filterData.noiseFactor *= 0.999;
  return {amp * (GetRandInRange(0.0F, 1.0F) - 0.5F), amp * (GetRandInRange(0.0F, 1.0F) - 0.5F)};
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

} // namespace GOOM
