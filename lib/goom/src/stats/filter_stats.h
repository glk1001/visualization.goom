#ifndef VISUALIZATION_GOOM_STATS_FILTER_STATS_H
#define VISUALIZATION_GOOM_STATS_FILTER_STATS_H

#include "goom/filters.h"
#include "goom/goom_config.h"

#include <chrono>
#include <cstdint>
#include <string>

namespace GOOM
{

class FilterStats
{
public:
  FilterStats() noexcept = default;

  void Reset();
  void Log(const StatsLogValueFunc& l) const;

  void SetLastMode(ZoomFilterMode mode);
  void SetLastJustChangedFilterSettings(bool val);
  void SetLastGeneralSpeed(float val);
  void SetLastPrevX(uint32_t val);
  void SetLastPrevY(uint32_t val);
  void SetLastTranBuffYLineStart(uint32_t val);
  void SetLastTranDiffFactor(int val);

  void UpdateStart();
  void UpdateEnd();

  void DoChangeFilterSettings();
  void DoZoomVector();
  void DoZoomVectorCrystalBallMode();
  void DoZoomVectorAmuletMode();
  void DoZoomVectorWaveMode();
  void DoZoomVectorScrunchMode();
  void DoZoomVectorSpeedwayMode();
  void DoZoomVectorDefaultMode();
  void DoZoomVectorNoisify();
  void DoZoomVectorNoiseFactor();
  void DoZoomVectorHypercosEffect();
  void DoZoomVectorHPlaneEffect();
  void DoZoomVectorVPlaneEffect();
  void DoZoomVectorTanEffect();
  void DoZoomVectorNegativeRotate();
  void DoZoomVectorPositiveRotate();
  void DoGetMixedColor();
  void DoGetBlockyMixedColor();
  void DoCZoom();
  void DoZoomFilterFastRgb();
  void DoRestartTranBuffer();
  void DoSwitchIncrNotZero();
  void DoSwitchMultNotOne();
  void DoTranPointClipped();
  void DoZoomVectorCoeffVitesseBelowMin();
  void DoZoomVectorCoeffVitesseAboveMax();

private:
  uint32_t m_numUpdates = 0;
  uint64_t m_totalTimeInUpdatesMs = 0;
  uint32_t m_minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  uint32_t m_maxTimeInUpdatesMs = 0;
  std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> m_timeNowHiRes{};

  ZoomFilterMode m_lastMode{};
  bool m_lastJustChangedFilterSettings = false;
  float m_lastGeneralSpeed = -1000.0;
  uint32_t m_lastPrevX = 0;
  uint32_t m_lastPrevY = 0;
  uint32_t m_lastTranBuffYLineStart = +1000000;
  int32_t m_lastTranDiffFactor = -1000;

  uint32_t m_numChangeFilterSettings = 0;
  uint64_t m_numZoomVectors = 0;
  uint64_t m_numZoomVectorCrystalBallMode = 0;
  uint64_t m_numZoomVectorAmuletMode = 0;
  uint64_t m_numZoomVectorWaveMode = 0;
  uint64_t m_numZoomVectorScrunchMode = 0;
  uint64_t m_numZoomVectorSpeedwayMode = 0;
  uint64_t m_numZoomVectorDefaultMode = 0;
  uint64_t m_numZoomVectorNoisify = 0;
  uint64_t m_numZoomVectorChangeNoiseFactor = 0;
  uint64_t m_numZoomVectorHypercosEffect = 0;
  uint64_t m_numZoomVectorHPlaneEffect = 0;
  uint64_t m_numZoomVectorVPlaneEffect = 0;
  uint64_t m_numGetMixedColor = 0;
  uint64_t m_numGetBlockyMixedColor = 0;
  uint64_t m_numCZoom = 0;
  uint64_t m_numGenerateWaterFxHorizontalBuffer = 0;
  uint64_t m_numZoomFilterFastRgb = 0;
  uint64_t m_numRestartTranBuffer = 0;
  uint64_t m_numResetTranBuffer = 0;
  uint64_t m_numSwitchIncrNotZero = 0;
  uint64_t m_numSwitchMultNotOne = 0;
  uint64_t m_numZoomVectorTanEffect = 0;
  uint64_t m_numZoomVectorNegativeRotate = 0;
  uint64_t m_numZoomVectorPositiveRotate = 0;
  uint64_t m_numTranPointsClipped = 0;
  uint64_t m_numZoomVectorCoeffVitesseBelowMin = 0;
  uint64_t m_numZoomVectorCoeffVitesseAboveMax = 0;
};

} // namespace GOOM

#endif //VISUALIZATION_GOOM_STATS_FILTER_STATS_H
