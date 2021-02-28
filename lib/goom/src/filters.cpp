// --- CHUI EN TRAIN DE SUPPRIMER LES EXTERN RESOLX ET C_RESOLY ---
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

#include "filter_buffers.h"
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

//#include <valgrind/callgrind.h>

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

#if __cplusplus <= 201402L
const uint8_t ZoomFilterData::pertedec = 8; // NEVER SEEMS TO CHANGE
#endif

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

  void ChangeFilterSettings(const ZoomFilterData& filterSettings);

  auto GetFilterSettings() const -> const ZoomFilterData&;
  auto GetFilterSettingsArePending() const -> bool;

  auto GetTranDiffFactor() const -> int32_t;
  auto GetGeneralSpeed() const -> float;

  void Start();

  void ZoomFilterFastRgb(const PixelBuffer& pix1,
                         PixelBuffer& pix2,
                         int32_t switchIncr,
                         float switchMult,
                         uint32_t& numClipped);

  void Log(const StatsLogValueFunc& l) const;

private:
  const uint32_t m_screenWidth;
  const uint32_t m_screenHeight;

  ZoomFilterBuffers m_filterBuffers;

  ZoomFilterData m_currentFilterSettings{};
  ZoomFilterData m_nextFilterSettings{};
  bool m_pendingFilterSettings = false;

  FXBuffSettings m_buffSettings{};
  std::string m_resourcesDirectory{};
  float m_generalSpeed;
  uint64_t m_updateNum{0};

  Parallel* const m_parallel;
  mutable FilterStats m_stats{};

  using NeighborhoodCoeffArray = ZoomFilterBuffers::NeighborhoodCoeffArray;
  using NeighborhoodPixelArray = ZoomFilterBuffers::NeighborhoodPixelArray;
  auto GetNewColor(const NeighborhoodCoeffArray& coeffs, const NeighborhoodPixelArray& pixels) const
      -> Pixel;
  auto GetMixedColor(const NeighborhoodCoeffArray& coeffs,
                     const NeighborhoodPixelArray& colors) const -> Pixel;
  auto GetBlockyMixedColor(const NeighborhoodCoeffArray& coeffs,
                           const NeighborhoodPixelArray& colors) const -> Pixel;

  void CZoom(const PixelBuffer& srceBuff, PixelBuffer& destBuff, uint32_t& numDestClipped) const;

  void UpdateTranBuffer();
  void UpdateTranDiffFactor(int32_t switchIncr, float switchMult);
  void RestartTranBuffer();

  auto GetZoomVector(float xNormalized, float yNormalized) const -> V2dFlt;
  auto GetStandardVelocity(float xNormalized, float yNormalized) const -> V2dFlt;
  auto GetHypercosVelocity(float xNormalized, float yNormalized) const -> V2dFlt;
  auto GetHPlaneEffectVelocity(float xNormalized, float yNormalized) const -> float;
  auto GetVPlaneEffectVelocity(float xNormalized, float yNormalized) const -> float;
  auto GetNoiseVelocity() const -> V2dFlt;
  auto GetTanEffectVelocity(float sqDist, const V2dFlt& velocity) const -> V2dFlt;
  auto GetRotatedVelocity(const V2dFlt& velocity) const -> V2dFlt;
  auto GetCleanedVelocity(const V2dFlt& velocity) const -> V2dFlt;

  auto GetCoeffVitesse(float xNormalized, float yNormalized, float sqDist) const -> float;
  auto GetWaveEffectCoeffVitesse(float sqDist) const -> float;
  auto GetClampedCoeffVitesse(float coeffVitesse) const -> float;

  void LogState(const std::string& name) const;
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
  m_fxImpl->Start();
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

void ZoomFilterFx::ChangeFilterSettings(const ZoomFilterData& filterSettings)
{
  m_fxImpl->ChangeFilterSettings(filterSettings);
}

void ZoomFilterFx::ZoomFilterFastRgb(const PixelBuffer& pix1,
                                     PixelBuffer& pix2,
                                     const int switchIncr,
                                     const float switchMult,
                                     uint32_t& numClipped)
{
  if (!m_enabled)
  {
    return;
  }

  m_fxImpl->ZoomFilterFastRgb(pix1, pix2, switchIncr, switchMult, numClipped);
}

auto ZoomFilterFx::GetFilterSettings() const -> const ZoomFilterData&
{
  return m_fxImpl->GetFilterSettings();
}

auto ZoomFilterFx::GetFilterSettingsArePending() const -> bool
{
  return m_fxImpl->GetFilterSettingsArePending();
}

auto ZoomFilterFx::GetTranDiffFactor() const -> int32_t
{
  return m_fxImpl->GetTranDiffFactor();
}

auto ZoomFilterFx::GetGeneralSpeed() const -> float
{
  return m_fxImpl->GetGeneralSpeed();
}

ZoomFilterFx::ZoomFilterImpl::ZoomFilterImpl(Parallel& p,
                                             const std::shared_ptr<const PluginInfo>& goomInfo)
  : m_screenWidth{goomInfo->GetScreenInfo().width},
    m_screenHeight{goomInfo->GetScreenInfo().height},
    m_filterBuffers{p, goomInfo},
    m_generalSpeed{0},
    m_parallel{&p}
{
  m_currentFilterSettings.middleX = m_screenWidth / 2;
  m_currentFilterSettings.middleY = m_screenHeight / 2;

  m_filterBuffers.SetGetZoomVectorFunc([this](const float xNormalized, const float yNormalized) {
    return this->GetZoomVector(xNormalized, yNormalized);
  });
}

ZoomFilterFx::ZoomFilterImpl::~ZoomFilterImpl() noexcept = default;

void ZoomFilterFx::ZoomFilterImpl::Log(const StatsLogValueFunc& l) const
{
  m_stats.SetLastMode(m_currentFilterSettings.mode);
  m_stats.SetLastJustChangedFilterSettings(m_pendingFilterSettings);
  m_stats.SetLastGeneralSpeed(m_generalSpeed);
  m_stats.SetLastPrevX(m_screenWidth);
  m_stats.SetLastPrevY(m_screenHeight);
  m_stats.SetLastTranBuffYLineStart(m_filterBuffers.GetTranBuffYLineStart());
  m_stats.SetLastTranDiffFactor(m_filterBuffers.GetTranDiffFactor());

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

auto ZoomFilterFx::ZoomFilterImpl::GetFilterSettings() const -> const ZoomFilterData&
{
  return m_currentFilterSettings;
}

auto ZoomFilterFx::ZoomFilterImpl::GetFilterSettingsArePending() const -> bool
{
  return m_pendingFilterSettings;
}

auto ZoomFilterFx::ZoomFilterImpl::GetTranDiffFactor() const -> int32_t
{
  return m_filterBuffers.GetTranDiffFactor();
}

auto ZoomFilterFx::ZoomFilterImpl::GetGeneralSpeed() const -> float
{
  return m_generalSpeed;
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetNewColor(const NeighborhoodCoeffArray& coeffs,
                                                      const NeighborhoodPixelArray& pixels) const
    -> Pixel
{
  if (m_currentFilterSettings.blockyWavy)
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
  static_assert(ZoomFilterBuffers::NUM_NEIGHBOR_COEFFS == 4, "NUM_NEIGHBOR_COEFFS must be 4.");
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
  for (size_t i = 0; i < ZoomFilterBuffers::NUM_NEIGHBOR_COEFFS; i++)
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

void ZoomFilterFx::ZoomFilterImpl::ChangeFilterSettings(const ZoomFilterData& filterSettings)
{
  m_stats.DoChangeFilterSettings();

  m_nextFilterSettings = filterSettings;
  m_pendingFilterSettings = true;
}

void ZoomFilterFx::ZoomFilterImpl::Start()
{
  ChangeFilterSettings(m_currentFilterSettings);
  m_filterBuffers.Start();
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
                                                     const int32_t switchIncr,
                                                     const float switchMult,
                                                     uint32_t& numClipped)
{
  m_updateNum++;

  logInfo("Starting ZoomFilterFastRgb, update {}", m_updateNum);
  logInfo("switchIncr = {}, switchMult = {}", switchIncr, switchMult);

  m_stats.UpdateStart();
  m_stats.DoZoomFilterFastRgb();

  UpdateTranBuffer();
  UpdateTranDiffFactor(switchIncr, switchMult);

  LogState("Before CZoom");
  CZoom(pix1, pix2, numClipped);
  LogState("After CZoom");

  m_stats.UpdateEnd();
}

void ZoomFilterFx::ZoomFilterImpl::UpdateTranBuffer()
{
  m_filterBuffers.UpdateTranBuffer();

  if (m_filterBuffers.GetTranBufferState() ==
      ZoomFilterBuffers::TranBufferState::RESTART_TRAN_BUFFER)
  {
    RestartTranBuffer();
  }
}

void ZoomFilterFx::ZoomFilterImpl::RestartTranBuffer()
{
  // Don't start making new stripes until filter settings change.
  if (!m_pendingFilterSettings)
  {
    return;
  }

  m_stats.DoRestartTranBuffer();

  m_pendingFilterSettings = false;
  m_currentFilterSettings = m_nextFilterSettings;

  m_filterBuffers.SetBuffMidPoint({static_cast<int32_t>(m_currentFilterSettings.middleX),
                                   static_cast<int32_t>(m_currentFilterSettings.middleY)});
  m_filterBuffers.SettingsChanged();

  m_generalSpeed =
      static_cast<float>(m_currentFilterSettings.vitesse - ZoomFilterData::MAX_VITESSE) /
      static_cast<float>(ZoomFilterData::MAX_VITESSE);
  if (m_currentFilterSettings.reverse)
  {
    m_generalSpeed = -m_generalSpeed;
  }
}

void ZoomFilterFx::ZoomFilterImpl::UpdateTranDiffFactor(const int32_t switchIncr,
                                                        const float switchMult)
{
  int32_t tranDiffFactor = m_filterBuffers.GetTranDiffFactor();

  logInfo("before switchIncr = {} tranDiffFactor = {}", switchIncr, tranDiffFactor);
  if (switchIncr != 0)
  {
    m_stats.DoSwitchIncrNotZero();

    tranDiffFactor += switchIncr;

    if (tranDiffFactor < 0)
    {
      tranDiffFactor = 0;
    }
    else if (tranDiffFactor > ZoomFilterBuffers::GetMaxTranDiffFactor())
    {
      tranDiffFactor = ZoomFilterBuffers::GetMaxTranDiffFactor();
    }
  }
  logInfo("after switchIncr = {} m_tranDiffFactor = {}", switchIncr, tranDiffFactor);

  if (!floats_equal(switchMult, 1.0F))
  {
    m_stats.DoSwitchMultNotOne();

    tranDiffFactor = static_cast<int32_t>(
        stdnew::lerp(static_cast<float>(ZoomFilterBuffers::GetMaxTranDiffFactor()),
                     static_cast<float>(tranDiffFactor), switchMult));
  }
  logInfo("after switchMult = {} m_tranDiffFactor = {}", switchMult, tranDiffFactor);

  m_filterBuffers.SetTranDiffFactor(tranDiffFactor);
}

#ifdef NO_PARALLEL
// pure c version of the zoom filter
void ZoomFilterFx::ZoomFilterImpl::CZoom(const PixelBuffer& srceBuff,
                                         PixelBuffer& destBuff,
                                         uint32_t& numDestClipped) const
{
  //  CALLGRIND_START_INSTRUMENTATION;

  m_stats.DoCZoom();

  numDestClipped = 0;

  for (uint32_t destY = 0; destY < m_screenHeight; destY++)
  {
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

      if ((tranX >= m_maxTranX) || (tranY >= m_maxTranY))
      {
        m_stats.DoTranPointClipped();
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
        if (43 < m_filterNum && m_filterNum < 51 && (*destRowBuff).Rgba() > 0xFF000000)
        {
          logInfo("destPos == {}", destPos);
          logInfo("srceX == {}", srceX);
          logInfo("srceY == {}", srceY);
          logInfo("tranX == {}", tranX);
          logInfo("tranY == {}", tranY);
          logInfo("coeffs[0] == {:x}", coeffs.c[0]);
          logInfo("coeffs[1] == {:x}", coeffs.c[1]);
          logInfo("coeffs[2] == {:x}", coeffs.c[2]);
          logInfo("coeffs[3] == {:x}", coeffs.c[3]);
          logInfo("(*destRowBuff).Rgba == {:x}", (*destRowBuff).Rgba());
        }
#endif
      }
      destPos++;
    }
  }

  //  CALLGRIND_STOP_INSTRUMENTATION;
  //  CALLGRIND_DUMP_STATS;
}
#endif

void ZoomFilterFx::ZoomFilterImpl::CZoom(const PixelBuffer& srceBuff,
                                         PixelBuffer& destBuff,
                                         uint32_t& numDestClipped) const
{
  m_stats.DoCZoom();

  numDestClipped = 0;

  const auto setDestPixelRow = [&](const uint32_t destY)
  {
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
      const V2dInt tranPoint = m_filterBuffers.GetZoomBufferSrceDestLerp(destPos);

      if (tranPoint.x < 0 || tranPoint.y < 0 ||
          static_cast<uint32_t>(tranPoint.x) >= m_filterBuffers.GetMaxTranX() ||
          static_cast<uint32_t>(tranPoint.y) >= m_filterBuffers.GetMaxTranY())
      {
        m_stats.DoTranPointClipped();
        *destRowBuff = Pixel::BLACK;
        numDestClipped++;
      }
      else
      {
#if __cplusplus <= 201402L
        const auto srceInfo = m_filterBuffers.GetSourceInfo(tranPoint);
        const auto srceX = std::get<0>(srceInfo);
        const auto srceY = std::get<1>(srceInfo);
        const auto coeffs = std::get<2>(srceInfo);
#else
        const auto [srceX, srceY, coeffs] =
            GetSourceInfo(static_cast<uint32_t>(tranX), static_cast<uint32_t>(tranY));
#endif
        const NeighborhoodPixelArray pixelNeighbours = srceBuff.Get4RHBNeighbours(srceX, srceY);
        *destRowBuff = GetNewColor(coeffs, pixelNeighbours);
#ifndef NO_LOGGING
        if (43 < m_updateNum && m_updateNum < 51 && (*destRowBuff).Rgba() > 0xFF000000)
        {
          logInfo("destPos == {}", destPos);
          logInfo("srceX == {}", srceX);
          logInfo("srceY == {}", srceY);
          logInfo("tranX == {}", tranX);
          logInfo("tranY == {}", tranY);
          logInfo("coeffs[0] == {:x}", coeffs.c[0]);
          logInfo("coeffs[1] == {:x}", coeffs.c[1]);
          logInfo("coeffs[2] == {:x}", coeffs.c[2]);
          logInfo("coeffs[3] == {:x}", coeffs.c[3]);
          logInfo("(*destRowBuff).Rgba == {:x}", (*destRowBuff).Rgba());
        }
#endif
      }
      destPos++;
    }
  };

  m_parallel->ForLoop(m_screenHeight, setDestPixelRow);
}

auto ZoomFilterFx::ZoomFilterImpl::GetZoomVector(const float xNormalized,
                                                 const float yNormalized) const -> V2dFlt
{
  m_stats.DoZoomVector();

  V2dFlt velocity = GetStandardVelocity(xNormalized, yNormalized);

  // The Effects add-ons...
  if (m_currentFilterSettings.noisify)
  {
    m_stats.DoZoomVectorNoisify();
    velocity += GetNoiseVelocity();
  }
  if (m_currentFilterSettings.hypercosEffect != ZoomFilterData::HypercosEffect::NONE)
  {
    m_stats.DoZoomVectorHypercosEffect();
    velocity += GetHypercosVelocity(xNormalized, yNormalized);
  }
  if (m_currentFilterSettings.hPlaneEffect != 0)
  {
    m_stats.DoZoomVectorHPlaneEffect();
    velocity.x += GetHPlaneEffectVelocity(xNormalized, yNormalized);
  }
  if (m_currentFilterSettings.vPlaneEffect != 0)
  {
    m_stats.DoZoomVectorVPlaneEffect();
    velocity.y += GetVPlaneEffectVelocity(xNormalized, yNormalized);
  }
  /* TODO : Water Mode */
  //    if (data->waveEffect)

  return GetCleanedVelocity(velocity);
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetCleanedVelocity(const V2dFlt& velocity) const -> V2dFlt
{
  V2dFlt newVelocity = velocity;
  if (std::fabs(velocity.x) < m_filterBuffers.GetMinNormalizedCoordVal())
  {
    newVelocity.x = (newVelocity.x < 0.0F) ? -m_filterBuffers.GetMinNormalizedCoordVal()
                                           : m_filterBuffers.GetMinNormalizedCoordVal();
  }
  if (std::fabs(newVelocity.y) < m_filterBuffers.GetMinNormalizedCoordVal())
  {
    newVelocity.y = (newVelocity.y < 0.0F) ? -m_filterBuffers.GetMinNormalizedCoordVal()
                                           : m_filterBuffers.GetMinNormalizedCoordVal();
  }
  return newVelocity;
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetStandardVelocity(const float xNormalized,
                                                              const float yNormalized) const
    -> V2dFlt
{
  const float sqDist = SqDistance(xNormalized, yNormalized);

  const float coeffVitesse = GetCoeffVitesse(xNormalized, yNormalized, sqDist);
  logDebug("coeffVitesse = {}", coeffVitesse);

  V2dFlt velocity{coeffVitesse * xNormalized, coeffVitesse * yNormalized};
  velocity = GetTanEffectVelocity(sqDist, velocity);
  return GetRotatedVelocity(velocity);
}

auto ZoomFilterFx::ZoomFilterImpl::GetCoeffVitesse([[maybe_unused]] const float xNormalized,
                                                   const float yNormalized,
                                                   const float sqDist) const -> float
{
  float coeffVitesse = (1.0F + m_generalSpeed) / m_currentFilterSettings.coeffVitesseDenominator;

  switch (m_currentFilterSettings.mode)
  {
    case ZoomFilterMode::CRYSTAL_BALL_MODE:
    {
      m_stats.DoZoomVectorCrystalBallMode();
      coeffVitesse -= m_currentFilterSettings.crystalBallAmplitude *
                      (sqDist - m_currentFilterSettings.crystalBallSqDistOffset);
      break;
    }
    case ZoomFilterMode::AMULET_MODE:
    {
      m_stats.DoZoomVectorAmuletMode();
      coeffVitesse += m_currentFilterSettings.amuletAmplitude * sqDist;
      break;
    }
    case ZoomFilterMode::SCRUNCH_MODE:
    {
      m_stats.DoZoomVectorScrunchMode();
      coeffVitesse += m_currentFilterSettings.scrunchAmplitude * sqDist;
      break;
    }
    case ZoomFilterMode::SPEEDWAY_MODE:
    {
      m_stats.DoZoomVectorSpeedwayMode();
      coeffVitesse *= m_currentFilterSettings.speedwayAmplitude * (yNormalized + 0.01F * sqDist);
      break;
    }
    case ZoomFilterMode::WAVE_MODE0:
    {
      m_stats.DoZoomVectorWaveMode();
      coeffVitesse += GetWaveEffectCoeffVitesse(sqDist);
      break;
    }
      //case ZoomFilterMode::HYPERCOS1_MODE:
      //break;
      //case ZoomFilterMode::HYPERCOS2_MODE:
      //break;
      //case ZoomFilterMode::YONLY_MODE:
      //break;
      /* Amulette 2 */
      // vx = X * tan(dist);
      // vy = Y * tan(dist);

    default:
      m_stats.DoZoomVectorDefaultMode();
      break;
  }

  return GetClampedCoeffVitesse(coeffVitesse);
}

auto ZoomFilterFx::ZoomFilterImpl::GetWaveEffectCoeffVitesse(const float sqDist) const -> float
{
  const float angle = m_currentFilterSettings.waveFreqFactor * sqDist;
  float periodicPart;
  switch (m_currentFilterSettings.waveEffectType)
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
  return m_currentFilterSettings.waveAmplitude * periodicPart;
}

auto ZoomFilterFx::ZoomFilterImpl::GetClampedCoeffVitesse(const float coeffVitesse) const -> float
{
  if (coeffVitesse < ZoomFilterData::MIN_COEF_VITESSE)
  {
    m_stats.DoZoomVectorCoeffVitesseBelowMin();
    return ZoomFilterData::MIN_COEF_VITESSE;
  }
  if (coeffVitesse > ZoomFilterData::MAX_COEF_VITESSE)
  {
    m_stats.DoZoomVectorCoeffVitesseAboveMax();
    return ZoomFilterData::MAX_COEF_VITESSE;
  }
  return coeffVitesse;
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetHypercosVelocity(const float xNormalized,
                                                              const float yNormalized) const
    -> V2dFlt
{
  const float sign = m_currentFilterSettings.hypercosReverse ? -1.0 : +1.0;
  float xVal = 0;
  float yVal = 0;

  switch (m_currentFilterSettings.hypercosEffect)
  {
    case ZoomFilterData::HypercosEffect::NONE:
      break;
    case ZoomFilterData::HypercosEffect::SIN_RECTANGULAR:
      xVal = std::sin(sign * m_currentFilterSettings.hypercosFreqX * xNormalized);
      yVal = std::sin(sign * m_currentFilterSettings.hypercosFreqY * yNormalized);
      break;
    case ZoomFilterData::HypercosEffect::COS_RECTANGULAR:
      xVal = std::cos(sign * m_currentFilterSettings.hypercosFreqX * xNormalized);
      yVal = std::cos(sign * m_currentFilterSettings.hypercosFreqY * yNormalized);
      break;
    case ZoomFilterData::HypercosEffect::SIN_CURL_SWIRL:
      xVal = std::sin(sign * m_currentFilterSettings.hypercosFreqY * yNormalized);
      yVal = std::sin(sign * m_currentFilterSettings.hypercosFreqX * xNormalized);
      break;
    case ZoomFilterData::HypercosEffect::COS_CURL_SWIRL:
      xVal = std::cos(sign * m_currentFilterSettings.hypercosFreqY * yNormalized);
      yVal = std::cos(sign * m_currentFilterSettings.hypercosFreqX * xNormalized);
      break;
    case ZoomFilterData::HypercosEffect::SIN_COS_CURL_SWIRL:
      xVal = std::sin(sign * m_currentFilterSettings.hypercosFreqX * yNormalized);
      yVal = std::cos(sign * m_currentFilterSettings.hypercosFreqY * xNormalized);
      break;
    case ZoomFilterData::HypercosEffect::COS_SIN_CURL_SWIRL:
      xVal = std::cos(sign * m_currentFilterSettings.hypercosFreqY * yNormalized);
      yVal = std::sin(sign * m_currentFilterSettings.hypercosFreqX * xNormalized);
      break;
    case ZoomFilterData::HypercosEffect::SIN_TAN_CURL_SWIRL:
      xVal = std::sin(std::tan(sign * m_currentFilterSettings.hypercosFreqY * yNormalized));
      yVal = std::cos(std::tan(sign * m_currentFilterSettings.hypercosFreqX * xNormalized));
      break;
    case ZoomFilterData::HypercosEffect::COS_TAN_CURL_SWIRL:
      xVal = std::cos(std::tan(sign * m_currentFilterSettings.hypercosFreqY * yNormalized));
      yVal = std::sin(std::tan(sign * m_currentFilterSettings.hypercosFreqX * xNormalized));
      break;
    default:
      throw std::logic_error("Unknown filterData.hypercosEffect value");
  }

  //  xVal = stdnew::clamp(std::tan(sign * m_currentFilterSettings.hypercosFreqY * xVal), -1.0, 1.0);
  //  yVal = stdnew::clamp(std::tan(sign * m_currentFilterSettings.hypercosFreqX * yVal), -1.0, 1.0);

  return {m_currentFilterSettings.hypercosAmplitudeX * xVal,
          m_currentFilterSettings.hypercosAmplitudeY * yVal};
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetHPlaneEffectVelocity(
    [[maybe_unused]] const float xNormalized, const float yNormalized) const -> float
{
  // TODO - try xNormalized
  return yNormalized * m_currentFilterSettings.hPlaneEffectAmplitude *
         static_cast<float>(m_currentFilterSettings.hPlaneEffect);
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetVPlaneEffectVelocity(
    [[maybe_unused]] const float xNormalized, [[maybe_unused]] const float yNormalized) const
    -> float
{
  // TODO - try yNormalized
  return xNormalized * m_currentFilterSettings.vPlaneEffectAmplitude *
         static_cast<float>(m_currentFilterSettings.vPlaneEffect);
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetNoiseVelocity() const -> V2dFlt
{
  if (m_currentFilterSettings.noiseFactor < 0.01)
  {
    return {0, 0};
  }

  m_stats.DoZoomVectorNoiseFactor();
  //    const float xAmp = 1.0/getRandInRange(50.0f, 200.0f);
  //    const float yAmp = 1.0/getRandInRange(50.0f, 200.0f);
  const float amp = m_currentFilterSettings.noiseFactor /
                    GetRandInRange(ZoomFilterData::NOISE_MIN, ZoomFilterData::NOISE_MAX);
  return {amp * (GetRandInRange(0.0F, 1.0F) - 0.5F), amp * (GetRandInRange(0.0F, 1.0F) - 0.5F)};
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetTanEffectVelocity(const float sqDist,
                                                               const V2dFlt& velocity) const
    -> V2dFlt
{
  if (!m_currentFilterSettings.tanEffect)
  {
    return velocity;
  }

  m_stats.DoZoomVectorTanEffect();
  const float tanArg =
      stdnew::clamp(std::fmod(sqDist, m_half_pi), -0.85 * m_half_pi, 0.85 * m_half_pi);
  const float tanSqDist = std::tan(tanArg);
  return {tanSqDist * velocity.x, tanSqDist * velocity.y};
}

inline auto ZoomFilterFx::ZoomFilterImpl::GetRotatedVelocity(const V2dFlt& velocity) const -> V2dFlt
{
  if (std::fabs(m_currentFilterSettings.rotateSpeed) < SMALL_FLOAT)
  {
    return velocity;
  }

  if (m_currentFilterSettings.rotateSpeed < 0.0F)
  {
    m_stats.DoZoomVectorNegativeRotate();
    return {-m_currentFilterSettings.rotateSpeed * (velocity.x - velocity.y),
            -m_currentFilterSettings.rotateSpeed * (velocity.x + velocity.y)};
  }

  m_stats.DoZoomVectorPositiveRotate();
  return {m_currentFilterSettings.rotateSpeed * (velocity.y + velocity.x),
          m_currentFilterSettings.rotateSpeed * (velocity.y - velocity.x)};
}

#ifdef NO_LOGGING
void ZoomFilterFx::ZoomFilterImpl::LogState([[maybe_unused]] const std::string& name) const
{
}
#else
void ZoomFilterFx::ZoomFilterImpl::LogState(const std::string& name) const
{
  logInfo("=================================");
  logInfo("Name: {}", name);

  logInfo("m_screenWidth = {}", m_screenWidth);
  logInfo("m_screenHeight = {}", m_screenHeight);
  logInfo("m_ratioScreenToNormalizedCoord = {}", m_ratioScreenToNormalizedCoord);
  logInfo("m_ratioNormalizedToScreenCoord = {}", m_ratioNormalizedToScreenCoord);
  logInfo("m_minNormalizedCoordVal = {}", m_minNormalizedCoordVal);
  logInfo("m_maxNormalizedCoordVal = {}", m_maxNormalizedCoordVal);
  logInfo("m_buffSettings = {}", m_buffSettings.allowOverexposed);
  logInfo("m_buffSettings.buffIntensity = {}", m_buffSettings.buffIntensity);
  logInfo("m_resourcesDirectory = {}", m_resourcesDirectory);
  logInfo("m_generalSpeed = {}", m_generalSpeed);
  logInfo("m_parallel->GetNumThreadsUsed() = {}", m_parallel->GetNumThreadsUsed());

  logInfo("m_currentFilterSettings.mode = {}", EnumToString(m_currentFilterSettings.mode));
  logInfo("m_currentFilterSettings.middleX = {}", m_currentFilterSettings.middleX);
  logInfo("m_currentFilterSettings.middleY = {}", m_currentFilterSettings.middleY);
  logInfo("m_currentFilterSettings.vitesse = {}", m_currentFilterSettings.vitesse);
  logInfo("m_currentFilterSettings.coeffVitesseDenominator = {}",
          m_currentFilterSettings.coeffVitesseDenominator);
  logInfo("m_currentFilterSettings.reverse = {}", m_currentFilterSettings.reverse);
  logInfo("m_currentFilterSettings.hPlaneEffect = {}", m_currentFilterSettings.hPlaneEffect);
  logInfo("m_currentFilterSettings.hPlaneEffectAmplitude = {}",
          m_currentFilterSettings.hPlaneEffectAmplitude);
  logInfo("m_currentFilterSettings.vPlaneEffect = {}", m_currentFilterSettings.vPlaneEffect);
  logInfo("m_currentFilterSettings.vPlaneEffectAmplitude = {}",
          m_currentFilterSettings.vPlaneEffectAmplitude);
  logInfo("m_currentFilterSettings.noisify = {}", m_currentFilterSettings.noisify);
  logInfo("m_currentFilterSettings.noiseFactor = {}", m_currentFilterSettings.noiseFactor);
  logInfo("m_currentFilterSettings.tanEffect = {}", m_currentFilterSettings.tanEffect);
  logInfo("m_currentFilterSettings.rotateSpeed = {}", m_currentFilterSettings.rotateSpeed);
  logInfo("m_currentFilterSettings.blockyWavy = {}", m_currentFilterSettings.blockyWavy);
  logInfo("m_currentFilterSettings.waveEffectType = {}", m_currentFilterSettings.waveEffectType);
  logInfo("m_currentFilterSettings.waveFreqFactor = {}", m_currentFilterSettings.waveFreqFactor);
  logInfo("m_currentFilterSettings.waveAmplitude = {}", m_currentFilterSettings.waveAmplitude);
  logInfo("m_currentFilterSettings.amuletAmplitude = {}", m_currentFilterSettings.amuletAmplitude);
  logInfo("m_currentFilterSettings.crystalBallAmplitude = {}",
          m_currentFilterSettings.crystalBallAmplitude);
  logInfo("m_currentFilterSettings.scrunchAmplitude = {}",
          m_currentFilterSettings.scrunchAmplitude);
  logInfo("m_currentFilterSettings.speedwayAmplitude = {}",
          m_currentFilterSettings.speedwayAmplitude);
  logInfo("m_currentFilterSettings.hypercosEffect = {}", m_currentFilterSettings.hypercosEffect);
  logInfo("m_currentFilterSettings.hypercosFreqX = {}", m_currentFilterSettings.hypercosFreqX);
  logInfo("m_currentFilterSettings.hypercosFreqY = {}", m_currentFilterSettings.hypercosFreqY);
  logInfo("m_currentFilterSettings.hypercosAmplitudeX = {}",
          m_currentFilterSettings.hypercosAmplitudeX);
  logInfo("m_currentFilterSettings.hypercosAmplitudeY = {}",
          m_currentFilterSettings.hypercosAmplitudeY);
  logInfo("m_currentFilterSettings.hypercosReverse = {}", m_currentFilterSettings.hypercosReverse);
  logInfo("m_currentFilterSettings.waveEffect = {}", m_currentFilterSettings.waveEffect);

  logInfo("m_maxTranX = {}", m_maxTranX);
  logInfo("m_maxTranY = {}", m_maxTranY);
  logInfo("m_tranBuffStripeHeight = {}", m_tranBuffStripeHeight);
  logInfo("m_tranBuffYLineStart = {}", m_tranBuffYLineStart);
  logInfo("m_tranBufferState = {}", m_tranBufferState);
  logInfo("m_tranDiffFactor = {}", m_tranDiffFactor);

  logInfo("=================================");
}
#endif

} // namespace GOOM
