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
  void SetNumThreadsUsedValue(size_t numThreads);
  void Reset();
  void Log(const StatsLogValueFunc& val) const;
  void UpdateChange(size_t currentState, ZoomFilterMode currentFilterMode);
  void StateChange(uint32_t timeInState);
  void StateChange(size_t index, uint32_t timeInState);
  void FilterModeChange();
  void FilterModeChange(ZoomFilterMode mode);
  void LockChange();
  void DoIfs();
  void DoDots();
  void DoLines();
  void SwitchLines();
  void DoStars();
  void DoTentacles();
  void TentaclesDisabled();
  void DoBigUpdate();
  void LastTimeGoomChange();
  void MegaLentChange();
  void DoNoise();
  void SetLastNoiseFactor(float val);
  void IfsRenew();
  void ChangeLineColor();
  void DoBlockyWavy();
  void DoZoomFilterAllowOverexposed();
  void SetFontFileUsed(const std::string& f);

private:
  std::string m_songTitle{};
  uint32_t m_startingState = 0;
  ZoomFilterMode m_startingFilterMode = ZoomFilterMode::_size;
  uint32_t m_lastState = 0;
  const ZoomFilterData* m_lastZoomFilterData = nullptr;
  uint64_t m_startingSeed = 0;
  uint64_t m_lastSeed = 0;
  size_t m_numThreadsUsed = 0;
  std::string m_fontFileUsed{};

  uint64_t m_totalTimeInUpdatesMs = 0;
  uint32_t m_minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  uint32_t m_maxTimeInUpdatesMs = 0;
  std::chrono::high_resolution_clock::time_point m_timeNowHiRes{};
  size_t m_stateAtMin = 0;
  size_t m_stateAtMax = 0;
  ZoomFilterMode m_filterModeAtMin = ZoomFilterMode::_null;
  ZoomFilterMode m_filterModeAtMax = ZoomFilterMode::_null;

  uint32_t m_numUpdates = 0;
  uint32_t m_totalStateChanges = 0;
  uint64_t m_totalStateDurations = 0;
  uint32_t m_totalFilterModeChanges = 0;
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
  uint32_t m_numIfsRenew = 0;
  uint32_t m_numChangeLineColor = 0;
  uint32_t m_numSwitchLines = 0;
  uint32_t m_numBlockyWavy = 0;
  uint32_t m_numZoomFilterAllowOverexposed = 0;
  std::array<uint32_t, static_cast<size_t>(ZoomFilterMode::_size)> m_numFilterModeChanges{0};
  std::vector<uint32_t> m_numStateChanges{};
  std::vector<uint64_t> m_stateDurations{};
};

} // namespace GOOM

#endif //VISUALIZATION_GOOM_GOOM_CONTROL_STATS_H
