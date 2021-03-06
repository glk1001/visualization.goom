#ifndef VISUALIZATION_GOOM_STATS_GOOM_CONTROL_STATS_H
#define VISUALIZATION_GOOM_STATS_GOOM_CONTROL_STATS_H

#include "goom/filters.h"
#include "goom/goom_config.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

namespace GOOM
{

class GoomStats
{
public:
  GoomStats() noexcept = default;
  ~GoomStats() noexcept = default;
  GoomStats(const GoomStats&) noexcept = delete;
  GoomStats(GoomStats&&) noexcept = delete;
  auto operator=(const GoomStats&) -> GoomStats& = delete;
  auto operator=(GoomStats&&) -> GoomStats& = delete;

  static void LogStatsValue(const std::string& module,
                            const std::string& name,
                            const StatsLogValue& logValue);

  void SetSongTitle(const std::string& songTitle);
  void SetStateStartValue(uint32_t stateIndex);
  void SetZoomFilterStartValue(ZoomFilterMode filterMode);
  void SetStateLastValue(uint32_t stateIndex);
  void SetZoomFilterLastValue(const ZoomFilterData* filterData);
  void SetSeedStartValue(uint64_t seed);
  void SetSeedLastValue(uint64_t seed);
  void SetLastNumClipped(uint32_t val);
  void SetNumThreadsUsedValue(size_t numThreads);
  void Reset();
  void Log(const StatsLogValueFunc& val) const;
  void UpdateChange(size_t currentState, ZoomFilterMode currentFilterMode);
  void DoStateChange(uint32_t timeInState);
  void DoStateChange(size_t index, uint32_t timeInState);
  void DoChangeFilterMode();
  void DoChangeFilterMode(ZoomFilterMode mode);
  void DoApplyChangeFilterSettings(uint32_t timeWithFilter);
  void DoLockChange();
  void DoIfs();
  void DoDots();
  void DoLines();
  void DoSwitchLines();
  void DoStars();
  void DoTentacles();
  void DoBigUpdate();
  void LastTimeGoomChange();
  void DoMegaLentChange();
  void DoNoise();
  void DoTurnOffNoise();
  void DoIfsRenew();
  void DoChangeLineColor();
  void DoBlockyWavy();
  void DoZoomFilterAllowOverexposed();
  void SetFontFileUsed(const std::string& f);
  void TooManyClipped();

private:
  std::string m_songTitle{};
  uint32_t m_startingState = 0;
  uint32_t m_lastState = 0;
  ZoomFilterMode m_startingFilterMode = ZoomFilterMode::_SIZE;
  const ZoomFilterData* m_lastZoomFilterSettings{};
  uint64_t m_startingSeed = 0;
  uint64_t m_lastSeed = 0;
  uint32_t m_lastNumClipped = 0;
  size_t m_numThreadsUsed = 0;
  std::string m_fontFileUsed{};

  uint64_t m_totalTimeInUpdatesMs = 0;
  uint32_t m_minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  uint32_t m_maxTimeInUpdatesMs = 0;
  std::chrono::high_resolution_clock::time_point m_timeNowHiRes{};
  size_t m_stateAtMin = 0;
  size_t m_stateAtMax = 0;
  ZoomFilterMode m_filterModeAtMin = ZoomFilterMode::_NULL;
  ZoomFilterMode m_filterModeAtMax = ZoomFilterMode::_NULL;

  uint32_t m_numUpdates = 0;
  uint32_t m_totalStateChanges = 0;
  uint64_t m_totalStateDurations = 0;
  uint32_t m_numChangeFilterModes = 0;
  uint32_t m_numApplyChangeFilterSettings = 0;
  uint64_t m_totalFilterDurations = 0;
  uint32_t m_lastFilterDuration = 0;
  uint32_t m_numLockChanges = 0;
  uint32_t m_numDoIFS = 0;
  uint32_t m_numDoDots = 0;
  uint32_t m_numDoLines = 0;
  uint32_t m_numDoStars = 0;
  uint32_t m_numDoTentacles = 0;
  uint32_t m_numDisabledTentacles = 0;
  uint32_t m_numBigUpdates = 0;
  uint32_t m_numLastTimeGoomChanges = 0;
  uint32_t m_numMegaLentChanges = 0;
  uint32_t m_numDoNoise = 0;
  uint32_t m_numTurnOffNoise = 0;
  uint32_t m_numIfsRenew = 0;
  uint32_t m_numChangeLineColor = 0;
  uint32_t m_numSwitchLines = 0;
  uint32_t m_numBlockyWavy = 0;
  uint32_t m_numZoomFilterAllowOverexposed = 0;
  uint32_t m_numTooManyClipped = 0;
  std::array<uint32_t, static_cast<size_t>(ZoomFilterMode::_SIZE)> m_numFilterModeChanges{0};
  std::vector<uint32_t> m_numStateChanges{};
  std::vector<uint64_t> m_stateDurations{};
};

} // namespace GOOM

#endif //VISUALIZATION_GOOM_STATS_GOOM_CONTROL_STATS_H
