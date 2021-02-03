#ifndef VISUALIZATION_GOOM_STATS_FILTER_STATS_H
#define VISUALIZATION_GOOM_STATS_FILTER_STATS_H

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
  void UpdateStart();
  void UpdateEnd();
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
  void DoMakeZoomBufferStripe();
  void DoGetMixedColor();
  void DoGetBlockyMixedColor();
  void DoCZoom();
  void DoGenerateWaterFxHorizontalBuffer();
  void DoZoomFilterFastRgb();
  void DoZoomFilterFastRgbChangeConfig();
  void DoZoomFilterFastRgbInterlaceStartEqualMinus11();
  void DoZoomFilterFastRgbInterlaceStartEqualMinus12();
  void DoZoomFilterFastRgbSwitchIncrNotZero();
  void DoZoomFilterFastRgbSwitchIncrNotEqual1();
  void DoCZoomOutOfRange();
  void CoeffVitesseBelowMin();
  void CoeffVitesseAboveMax();

  void SetLastGeneralSpeed(float val);
  void SetLastPrevX(uint32_t val);
  void SetLastPrevY(uint32_t val);
  void SetLastInterlaceStart(int val);
  void SetLastTranDiffFactor(int val);

private:
  uint32_t m_numUpdates = 0;
  uint64_t m_totalTimeInUpdatesMs = 0;
  uint32_t m_minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  uint32_t m_maxTimeInUpdatesMs = 0;
  std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> m_timeNowHiRes{};

  uint64_t m_numZoomVectors = 0;
  uint64_t m_numZoomVectorCrystalBallMode = 0;
  uint64_t m_numZoomVectorAmuletMode = 0;
  uint64_t m_numZoomVectorWaveMode = 0;
  uint64_t m_numZoomVectorScrunchMode = 0;
  uint64_t m_numZoomVectorSpeedwayMode = 0;
  uint64_t m_numZoomVectorDefaultMode = 0;
  uint64_t m_numZoomVectorNoisify = 0;
  uint64_t m_numChangeZoomVectorNoiseFactor = 0;
  uint64_t m_numZoomVectorHypercosEffect = 0;
  uint64_t m_numZoomVectorHPlaneEffect = 0;
  uint64_t m_numZoomVectorVPlaneEffect = 0;
  uint64_t m_numMakeZoomBufferStripe = 0;
  uint64_t m_numGetMixedColor = 0;
  uint64_t m_numGetBlockyMixedColor = 0;
  uint64_t m_numCZoom = 0;
  uint64_t m_numGenerateWaterFxHorizontalBuffer = 0;
  uint64_t m_numZoomFilterFastRgb = 0;
  uint64_t m_numZoomFilterFastRgbChangeConfig = 0;
  uint64_t m_numZoomFilterFastRgbInterlaceStartEqualMinus11 = 0;
  uint64_t m_numZoomFilterFastRgbInterlaceStartEqualMinus12 = 0;
  uint64_t m_numZoomFilterFastRgbSwitchIncrNotZero = 0;
  uint64_t m_numZoomFilterFastRgbSwitchIncrNotEqual1 = 0;
  uint64_t m_numCZoomOutOfRange = 0;
  uint64_t m_numCoeffVitesseBelowMin = 0;
  uint64_t m_numCoeffVitesseAboveMax = 0;

  float m_lastGeneralSpeed = -1000.0;
  uint32_t m_lastPrevX = 0;
  uint32_t m_lastPrevY = 0;
  int32_t m_lastInterlaceStart = -1000;
  int32_t m_lastTranDiffFactor = -1000;
};

} // namespace GOOM

#endif //VISUALIZATION_GOOM_STATS_FILTER_STATS_H