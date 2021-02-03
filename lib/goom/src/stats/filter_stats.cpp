#include "filter_stats.h"

#include "goom/goom_config.h"

#include <chrono>
#include <cmath>
#include <cstdint>

namespace GOOM
{

void FilterStats::Reset()
{
  m_numUpdates = 0;
  m_totalTimeInUpdatesMs = 0;
  m_minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  m_maxTimeInUpdatesMs = 0;
  m_timeNowHiRes = std::chrono::high_resolution_clock::now();

  m_numZoomVectors = 0;
  m_numZoomVectorCrystalBallMode = 0;
  m_numZoomVectorAmuletMode = 0;
  m_numZoomVectorWaveMode = 0;
  m_numZoomVectorScrunchMode = 0;
  m_numZoomVectorSpeedwayMode = 0;
  m_numZoomVectorDefaultMode = 0;
  m_numZoomVectorNoisify = 0;
  m_numChangeZoomVectorNoiseFactor = 0;
  m_numZoomVectorHypercosEffect = 0;
  m_numZoomVectorHPlaneEffect = 0;
  m_numZoomVectorVPlaneEffect = 0;
  m_numMakeZoomBufferStripe = 0;
  m_numGetMixedColor = 0;
  m_numGetBlockyMixedColor = 0;
  m_numCZoom = 0;
  m_numGenerateWaterFxHorizontalBuffer = 0;
  m_numZoomFilterFastRgb = 0;
  m_numZoomFilterFastRgbChangeConfig = 0;
  m_numZoomFilterFastRgbInterlaceStartEqualMinus11 = 0;
  m_numZoomFilterFastRgbInterlaceStartEqualMinus12 = 0;
  m_numZoomFilterFastRgbSwitchIncrNotZero = 0;
  m_numZoomFilterFastRgbSwitchIncrNotEqual1 = 0;
  m_numCZoomOutOfRange = 0;
  m_numCoeffVitesseBelowMin = 0;
  m_numCoeffVitesseAboveMax = 0;
}

#if __cplusplus <= 201402L
void FilterStats::Log(const StatsLogValueFunc&) const
{
}
#else
void FilterStats::Log(const StatsLogValueFunc& logVal) const
{
  const constexpr char* MODULE = "Filter";

  const int32_t avTimeInUpdateMs =
      std::lround(m_numUpdates == 0 ? -1.0
                                    : static_cast<float>(m_totalTimeInUpdatesMs) /
                                          static_cast<float>(m_numUpdates));
  logVal(MODULE, "avTimeInUpdateMs", avTimeInUpdateMs);
  logVal(MODULE, "minTimeInUpdatesMs", m_minTimeInUpdatesMs);
  logVal(MODULE, "maxTimeInUpdatesMs", m_maxTimeInUpdatesMs);

  logVal(MODULE, "lastGeneralSpeed", m_lastGeneralSpeed);
  logVal(MODULE, "lastPrevX", m_lastPrevX);
  logVal(MODULE, "lastPrevY", m_lastPrevY);
  logVal(MODULE, "lastInterlaceStart", m_lastInterlaceStart);
  logVal(MODULE, "lastTranDiffFactor", m_lastTranDiffFactor);

  logVal(MODULE, "numZoomVectors", m_numZoomVectors);
  logVal(MODULE, "numZoomVectorCrystalBallMode", m_numZoomVectorCrystalBallMode);
  logVal(MODULE, "numZoomVectorAmuletMode", m_numZoomVectorAmuletMode);
  logVal(MODULE, "numZoomVectorWaveMode", m_numZoomVectorWaveMode);
  logVal(MODULE, "numZoomVectorScrunchMode", m_numZoomVectorScrunchMode);
  logVal(MODULE, "numZoomVectorSpeedwayMode", m_numZoomVectorSpeedwayMode);
  logVal(MODULE, "numZoomVectorDefaultMode", m_numZoomVectorDefaultMode);
  logVal(MODULE, "numZoomVectorNoisify", m_numZoomVectorNoisify);
  logVal(MODULE, "numChangeZoomVectorNoiseFactor", m_numChangeZoomVectorNoiseFactor);
  logVal(MODULE, "numZoomVectorHypercosEffect", m_numZoomVectorHypercosEffect);
  logVal(MODULE, "numZoomVectorHPlaneEffect", m_numZoomVectorHPlaneEffect);
  logVal(MODULE, "numZoomVectorVPlaneEffect", m_numZoomVectorVPlaneEffect);
  logVal(MODULE, "numMakeZoomBufferStripe", m_numMakeZoomBufferStripe);
  logVal(MODULE, "numGetMixedColor", m_numGetMixedColor);
  logVal(MODULE, "numGetBlockyMixedColor", m_numGetBlockyMixedColor);
  logVal(MODULE, "numCZoom", m_numCZoom);
  logVal(MODULE, "numGenerateWaterFXHorizontalBuffer", m_numGenerateWaterFxHorizontalBuffer);
  logVal(MODULE, "numZoomFilterFastRGBChangeConfig", m_numZoomFilterFastRgbChangeConfig);
  logVal(MODULE, "numZoomFilterFastRGBInterlaceStartEqualMinus1_1",
         m_numZoomFilterFastRgbInterlaceStartEqualMinus11);
  logVal(MODULE, "numZoomFilterFastRGBInterlaceStartEqualMinus1_2",
         m_numZoomFilterFastRgbInterlaceStartEqualMinus12);
  logVal(MODULE, "numZoomFilterFastRGBSwitchIncrNotZero", m_numZoomFilterFastRgbSwitchIncrNotZero);
  logVal(MODULE, "numZoomFilterFastRGBSwitchIncrNotEqual1",
         m_numZoomFilterFastRgbSwitchIncrNotEqual1);
  logVal(MODULE, "numCZoomOutOfRange", m_numCZoomOutOfRange);
  logVal(MODULE, "numCoeffVitesseBelowMin", m_numCoeffVitesseBelowMin);
  logVal(MODULE, "numCoeffVitesseAboveMax", m_numCoeffVitesseAboveMax);
}
#endif

void FilterStats::UpdateStart()
{
  m_timeNowHiRes = std::chrono::high_resolution_clock::now();
  m_numUpdates++;
}

void FilterStats::UpdateEnd()
{
  const auto timeNow = std::chrono::high_resolution_clock::now();

  using Ms = std::chrono::milliseconds;
  const Ms diff = std::chrono::duration_cast<Ms>(timeNow - m_timeNowHiRes);
  const auto timeInUpdateMs = static_cast<uint32_t>(diff.count());
  if (timeInUpdateMs < m_minTimeInUpdatesMs)
  {
    m_minTimeInUpdatesMs = timeInUpdateMs;
  }
  else if (timeInUpdateMs > m_maxTimeInUpdatesMs)
  {
    m_maxTimeInUpdatesMs = timeInUpdateMs;
  }
  m_totalTimeInUpdatesMs += timeInUpdateMs;
}

void FilterStats::DoZoomVector()
{
  m_numZoomVectors++;
}

void FilterStats::DoZoomVectorCrystalBallMode()
{
  m_numZoomVectorCrystalBallMode++;
}

void FilterStats::DoZoomVectorAmuletMode()
{
  m_numZoomVectorAmuletMode++;
}

void FilterStats::DoZoomVectorWaveMode()
{
  m_numZoomVectorWaveMode++;
}

void FilterStats::DoZoomVectorScrunchMode()
{
  m_numZoomVectorScrunchMode++;
}

void FilterStats::DoZoomVectorSpeedwayMode()
{
  m_numZoomVectorSpeedwayMode++;
}

void FilterStats::DoZoomVectorDefaultMode()
{
  m_numZoomVectorDefaultMode++;
}

void FilterStats::DoZoomVectorNoisify()
{
  m_numZoomVectorNoisify++;
}

void FilterStats::DoZoomVectorNoiseFactor()
{
  m_numChangeZoomVectorNoiseFactor++;
}

void FilterStats::DoZoomVectorHypercosEffect()
{
  m_numZoomVectorHypercosEffect++;
}

void FilterStats::DoZoomVectorHPlaneEffect()
{
  m_numZoomVectorHPlaneEffect++;
}

void FilterStats::DoZoomVectorVPlaneEffect()
{
  m_numZoomVectorVPlaneEffect++;
}

void FilterStats::DoMakeZoomBufferStripe()
{
  m_numMakeZoomBufferStripe++;
}

void FilterStats::DoGetMixedColor()
{
  m_numGetMixedColor++;
}

void FilterStats::DoGetBlockyMixedColor()
{
  m_numGetBlockyMixedColor++;
}

void FilterStats::DoCZoom()
{
  m_numCZoom++;
}

void FilterStats::DoGenerateWaterFxHorizontalBuffer()
{
  m_numGenerateWaterFxHorizontalBuffer++;
}

void FilterStats::DoZoomFilterFastRgb()
{
  m_numZoomFilterFastRgb++;
}

void FilterStats::DoZoomFilterFastRgbChangeConfig()
{
  m_numZoomFilterFastRgbChangeConfig++;
}

void FilterStats::DoZoomFilterFastRgbInterlaceStartEqualMinus11()
{
  m_numZoomFilterFastRgbInterlaceStartEqualMinus11++;
}

void FilterStats::DoZoomFilterFastRgbInterlaceStartEqualMinus12()
{
  m_numZoomFilterFastRgbInterlaceStartEqualMinus12++;
}

void FilterStats::DoZoomFilterFastRgbSwitchIncrNotEqual1()
{
  m_numZoomFilterFastRgbSwitchIncrNotEqual1++;
}

void FilterStats::DoZoomFilterFastRgbSwitchIncrNotZero()
{
  m_numZoomFilterFastRgbSwitchIncrNotZero++;
}

void FilterStats::DoCZoomOutOfRange()
{
  m_numCZoomOutOfRange++;
}

void FilterStats::CoeffVitesseBelowMin()
{
  m_numCoeffVitesseBelowMin++;
}

void FilterStats::CoeffVitesseAboveMax()
{
  m_numCoeffVitesseAboveMax++;
}

void FilterStats::SetLastGeneralSpeed(const float val)
{
  m_lastGeneralSpeed = val;
}

void FilterStats::SetLastPrevX(const uint32_t val)
{
  m_lastPrevX = val;
}

void FilterStats::SetLastPrevY(const uint32_t val)
{
  m_lastPrevY = val;
}

void FilterStats::SetLastInterlaceStart(const int32_t val)
{
  m_lastInterlaceStart = val;
}

void FilterStats::SetLastTranDiffFactor(const int32_t val)
{
  m_lastTranDiffFactor = val;
}

} // namespace GOOM