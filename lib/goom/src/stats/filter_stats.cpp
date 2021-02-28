#include "filter_stats.h"

#include "goom/goom_config.h"
#include "goomutils/strutils.h"

#include <chrono>
#include <cmath>
#include <cstdint>


namespace GOOM
{

using GOOM::UTILS::EnumToString;

void FilterStats::Reset()
{
  m_numUpdates = 0;
  m_totalTimeInUpdatesMs = 0;
  m_minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  m_maxTimeInUpdatesMs = 0;
  m_timeNowHiRes = std::chrono::high_resolution_clock::now();

  m_numChangeFilterSettings = 0;
  m_numZoomVectors = 0;
  m_numZoomVectorCrystalBallMode = 0;
  m_numZoomVectorAmuletMode = 0;
  m_numZoomVectorWaveMode = 0;
  m_numZoomVectorScrunchMode = 0;
  m_numZoomVectorSpeedwayMode = 0;
  m_numZoomVectorDefaultMode = 0;
  m_numZoomVectorNoisify = 0;
  m_numZoomVectorChangeNoiseFactor = 0;
  m_numZoomVectorHypercosEffect = 0;
  m_numZoomVectorHPlaneEffect = 0;
  m_numZoomVectorVPlaneEffect = 0;
  m_numGetMixedColor = 0;
  m_numGetBlockyMixedColor = 0;
  m_numCZoom = 0;
  m_numGenerateWaterFxHorizontalBuffer = 0;
  m_numZoomFilterFastRgb = 0;
  m_numRestartTranBuffer = 0;
  m_numResetTranBuffer = 0;
  m_numSwitchIncrNotZero = 0;
  m_numSwitchMultNotOne = 0;
  m_numZoomVectorTanEffect = 0;
  m_numZoomVectorNegativeRotate = 0;
  m_numZoomVectorPositiveRotate = 0;
  m_numTranPointsClipped = 0;
  m_numZoomVectorCoeffVitesseBelowMin = 0;
  m_numZoomVectorCoeffVitesseAboveMax = 0;
}

void FilterStats::Log(const StatsLogValueFunc& logVal) const
{
  const constexpr char* MODULE = "Filter";

  const auto avTimeInUpdateMs = static_cast<int32_t>(std::lround(
      m_numUpdates == 0
          ? -1.0
          : static_cast<float>(m_totalTimeInUpdatesMs) / static_cast<float>(m_numUpdates)));
  logVal(MODULE, "avTimeInUpdateMs", avTimeInUpdateMs);
  logVal(MODULE, "minTimeInUpdatesMs", m_minTimeInUpdatesMs);
  logVal(MODULE, "maxTimeInUpdatesMs", m_maxTimeInUpdatesMs);

  logVal(MODULE, "lastMode", EnumToString(m_lastMode));
  logVal(MODULE, "lastJustChangedFilterSettings", static_cast<uint32_t>(m_lastJustChangedFilterSettings));
  logVal(MODULE, "lastGeneralSpeed", m_lastGeneralSpeed);
  logVal(MODULE, "lastPrevX", m_lastPrevX);
  logVal(MODULE, "lastPrevY", m_lastPrevY);
  logVal(MODULE, "lastTranBuffYLineStart", m_lastTranBuffYLineStart);
  logVal(MODULE, "lastTranDiffFactor", m_lastTranDiffFactor);

  logVal(MODULE, "numChangeFilterSettings", m_numChangeFilterSettings);
  logVal(MODULE, "numGetMixedColor", m_numGetMixedColor);
  logVal(MODULE, "numGetBlockyMixedColor", m_numGetBlockyMixedColor);
  logVal(MODULE, "numCZoom", m_numCZoom);
  logVal(MODULE, "numGenerateWaterFXHorizontalBuffer", m_numGenerateWaterFxHorizontalBuffer);
  logVal(MODULE, "numZoomVectors", m_numZoomVectors);
  logVal(MODULE, "numZoomVectorCrystalBallMode", m_numZoomVectorCrystalBallMode);
  logVal(MODULE, "numZoomVectorAmuletMode", m_numZoomVectorAmuletMode);
  logVal(MODULE, "numZoomVectorWaveMode", m_numZoomVectorWaveMode);
  logVal(MODULE, "numZoomVectorScrunchMode", m_numZoomVectorScrunchMode);
  logVal(MODULE, "numZoomVectorSpeedwayMode", m_numZoomVectorSpeedwayMode);
  logVal(MODULE, "numZoomVectorDefaultMode", m_numZoomVectorDefaultMode);
  logVal(MODULE, "numZoomVectorNoisify", m_numZoomVectorNoisify);
  logVal(MODULE, "numZoomVectorChangeNoiseFactor", m_numZoomVectorChangeNoiseFactor);
  logVal(MODULE, "numZoomVectorHypercosEffect", m_numZoomVectorHypercosEffect);
  logVal(MODULE, "numZoomVectorHPlaneEffect", m_numZoomVectorHPlaneEffect);
  logVal(MODULE, "numZoomVectorVPlaneEffect", m_numZoomVectorVPlaneEffect);
  logVal(MODULE, "numZoomVectorNegativeRotate", m_numZoomVectorNegativeRotate);
  logVal(MODULE, "numZoomVectorPositiveRotate", m_numZoomVectorPositiveRotate);
  logVal(MODULE, "numZoomVectorTanEffect", m_numZoomVectorTanEffect);
  logVal(MODULE, "numZoomVectorCoeffVitesseBelowMin", m_numZoomVectorCoeffVitesseBelowMin);
  logVal(MODULE, "numZoomVectorCoeffVitesseAboveMax", m_numZoomVectorCoeffVitesseAboveMax);
  logVal(MODULE, "numResetTranBuffer", m_numResetTranBuffer);
  logVal(MODULE, "numRestartTranBuffer", m_numRestartTranBuffer);
  logVal(MODULE, "numSwitchIncrNotZero", m_numSwitchIncrNotZero);
  logVal(MODULE, "numSwitchMultNotOne", m_numSwitchMultNotOne);
  logVal(MODULE, "numTranPointsClipped", m_numTranPointsClipped);
}

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

void FilterStats::DoChangeFilterSettings()
{
  m_numChangeFilterSettings++;
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
  m_numZoomVectorChangeNoiseFactor++;
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

void FilterStats::DoZoomFilterFastRgb()
{
  m_numZoomFilterFastRgb++;
}

void FilterStats::DoRestartTranBuffer()
{
  m_numRestartTranBuffer++;
}

void FilterStats::DoSwitchMultNotOne()
{
  m_numSwitchMultNotOne++;
}

void FilterStats::DoSwitchIncrNotZero()
{
  m_numSwitchIncrNotZero++;
}

void FilterStats::DoZoomVectorTanEffect()
{
  m_numZoomVectorTanEffect++;
}

void FilterStats::DoZoomVectorNegativeRotate()
{
  m_numZoomVectorNegativeRotate++;
}

void FilterStats::DoZoomVectorPositiveRotate()
{
  m_numZoomVectorPositiveRotate++;
}

void FilterStats::DoTranPointClipped()
{
  m_numTranPointsClipped++;
}

void FilterStats::DoZoomVectorCoeffVitesseBelowMin()
{
  m_numZoomVectorCoeffVitesseBelowMin++;
}

void FilterStats::DoZoomVectorCoeffVitesseAboveMax()
{
  m_numZoomVectorCoeffVitesseAboveMax++;
}

void FilterStats::SetLastMode(ZoomFilterMode mode)
{
  m_lastMode = mode;
}

void FilterStats::SetLastJustChangedFilterSettings(bool val)
{
  m_lastJustChangedFilterSettings = val;
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

void FilterStats::SetLastTranBuffYLineStart(const uint32_t val)
{
  m_lastTranBuffYLineStart = val;
}

void FilterStats::SetLastTranDiffFactor(const int32_t val)
{
  m_lastTranDiffFactor = val;
}

} // namespace GOOM
