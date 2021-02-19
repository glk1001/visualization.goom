#include "goom_stats.h"

#include "goom/filters.h"
#include "goomutils/logging.h"
#include "goomutils/strutils.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <numeric>
#include <string>

namespace GOOM
{

using GOOM::UTILS::EnumToString;
using GOOM::UTILS::Logging;

void GoomStats::Reset()
{
  m_startingState = 0;
  m_startingFilterMode = ZoomFilterMode::_SIZE;
  m_startingSeed = 0;
  m_lastState = 0;
  m_lastZoomFilterData = nullptr;
  m_lastSeed = 0;
  m_numThreadsUsed = 0;

  m_stateAtMin = 0;
  m_stateAtMax = 0;
  m_filterModeAtMin = ZoomFilterMode::_NULL;
  m_filterModeAtMax = ZoomFilterMode::_NULL;

  m_numUpdates = 0;
  m_totalTimeInUpdatesMs = 0;
  m_minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  m_maxTimeInUpdatesMs = 0;
  m_timeNowHiRes = std::chrono::high_resolution_clock::now();
  m_totalStateChanges = 0;
  m_totalStateDurations = 0;
  std::fill(m_numStateChanges.begin(), m_numStateChanges.end(), 0);
  std::fill(m_stateDurations.begin(), m_stateDurations.end(), 0);
  m_totalFilterModeChanges = 0;
  m_numFilterModeChanges.fill(0);
  m_numLockChanges = 0;
  m_numDoIFS = 0;
  m_numDoDots = 0;
  m_numDoLines = 0;
  m_numDoStars = 0;
  m_numDoTentacles = 0;
  m_numDisabledTentacles = 0;
  m_numBigUpdates = 0;
  m_numLastTimeGoomChanges = 0;
  m_numMegaLentChanges = 0;
  m_numDoNoise = 0;
  m_numTurnOffNoise = 0;
  m_numIfsRenew = 0;
  m_numChangeLineColor = 0;
  m_numSwitchLines = 0;
  m_numBlockyWavy = 0;
  m_numZoomFilterAllowOverexposed = 0;
  m_numTooManyClipped = 0;
}

void GoomStats::Log(const StatsLogValueFunc& logVal) const
{
  const constexpr char* MODULE = "goom_core";

  logVal(MODULE, "songTitle", m_songTitle);
  logVal(MODULE, "startingState", m_startingState);
  logVal(MODULE, "startingFilterMode", EnumToString(m_startingFilterMode));
  logVal(MODULE, "startingSeed", m_startingSeed);
  logVal(MODULE, "lastState", m_lastState);
  logVal(MODULE, "lastSeed", m_lastSeed);
  logVal(MODULE, "numThreadsUsed", m_numThreadsUsed);
  logVal(MODULE, "m_fontFileUsed", m_fontFileUsed);
  logVal(MODULE, "Compiler std", static_cast<uint32_t>(__cplusplus));

  if (m_lastZoomFilterData == nullptr)
  {
    logVal(MODULE, "lastZoomFilterData", 0U);
  }
  else
  {
    logVal(MODULE, "lastZoomFilterData->mode", EnumToString(m_lastZoomFilterData->mode));
    logVal(MODULE, "lastZoomFilterData->vitesse", m_lastZoomFilterData->vitesse);
    logVal(MODULE, "lastZoomFilterData->pertedec", static_cast<uint32_t>(ZoomFilterData::pertedec));
    logVal(MODULE, "lastZoomFilterData->middleX", m_lastZoomFilterData->middleX);
    logVal(MODULE, "lastZoomFilterData->middleY", m_lastZoomFilterData->middleY);
    logVal(MODULE, "lastZoomFilterData->reverse",
           static_cast<uint32_t>(m_lastZoomFilterData->reverse));
    logVal(MODULE, "lastZoomFilterData->hPlaneEffect", m_lastZoomFilterData->hPlaneEffect);
    logVal(MODULE, "lastZoomFilterData->vPlaneEffect", m_lastZoomFilterData->vPlaneEffect);
    logVal(MODULE, "lastZoomFilterData->waveEffect",
           static_cast<uint32_t>(m_lastZoomFilterData->waveEffect));
    logVal(MODULE, "lastZoomFilterData->hypercosEffect",
           EnumToString(m_lastZoomFilterData->hypercosEffect));
    logVal(MODULE, "lastZoomFilterData->noisify",
           static_cast<uint32_t>(m_lastZoomFilterData->noisify));
    logVal(MODULE, "lastZoomFilterData->noiseFactor",
           static_cast<float>(m_lastZoomFilterData->noiseFactor));
    logVal(MODULE, "lastZoomFilterData->blockyWavy",
           static_cast<uint32_t>(m_lastZoomFilterData->blockyWavy));
    logVal(MODULE, "lastZoomFilterData->waveFreqFactor", m_lastZoomFilterData->waveFreqFactor);
    logVal(MODULE, "lastZoomFilterData->waveAmplitude", m_lastZoomFilterData->waveAmplitude);
    logVal(MODULE, "lastZoomFilterData->waveEffectType",
           EnumToString(m_lastZoomFilterData->waveEffectType));
    logVal(MODULE, "lastZoomFilterData->scrunchAmplitude", m_lastZoomFilterData->scrunchAmplitude);
    logVal(MODULE, "lastZoomFilterData->speedwayAmplitude",
           m_lastZoomFilterData->speedwayAmplitude);
    logVal(MODULE, "lastZoomFilterData->amuletteAmplitude", m_lastZoomFilterData->amuletAmplitude);
    logVal(MODULE, "lastZoomFilterData->crystalBallAmplitude",
           m_lastZoomFilterData->crystalBallAmplitude);
    logVal(MODULE, "lastZoomFilterData->hypercosFreqX", m_lastZoomFilterData->hypercosFreqX);
    logVal(MODULE, "lastZoomFilterData->hypercosFreqY", m_lastZoomFilterData->hypercosFreqY);
    logVal(MODULE, "lastZoomFilterData->hypercosAmplitudeX",
           m_lastZoomFilterData->hypercosAmplitudeX);
    logVal(MODULE, "lastZoomFilterData->hypercosAmplitudeY",
           m_lastZoomFilterData->hypercosAmplitudeY);
    logVal(MODULE, "lastZoomFilterData->hPlaneEffectAmplitude",
           m_lastZoomFilterData->hPlaneEffectAmplitude);
    logVal(MODULE, "lastZoomFilterData->vPlaneEffectAmplitude",
           m_lastZoomFilterData->vPlaneEffectAmplitude);
    logVal(MODULE, "lastZoomFilterData->rotateSpeed", m_lastZoomFilterData->rotateSpeed);
    logVal(MODULE, "lastZoomFilterData->tanEffect",
           static_cast<uint32_t>(m_lastZoomFilterData->tanEffect));
  }

  logVal(MODULE, "numUpdates", m_numUpdates);
  const auto avTimeInUpdateMs = static_cast<int32_t>(std::lround(
      m_numUpdates == 0
          ? -1.0
          : static_cast<float>(m_totalTimeInUpdatesMs) / static_cast<float>(m_numUpdates)));
  logVal(MODULE, "avTimeInUpdateMs", avTimeInUpdateMs);
  logVal(MODULE, "minTimeInUpdatesMs", m_minTimeInUpdatesMs);
  logVal(MODULE, "stateAtMin", m_stateAtMin);
  logVal(MODULE, "filterModeAtMin", EnumToString(m_filterModeAtMin));
  logVal(MODULE, "maxTimeInUpdatesMs", m_maxTimeInUpdatesMs);
  logVal(MODULE, "stateAtMax", m_stateAtMax);
  logVal(MODULE, "filterModeAtMax", EnumToString(m_filterModeAtMax));
  logVal(MODULE, "totalStateChanges", m_totalStateChanges);
  const float avStateDuration =
      m_totalStateChanges == 0
          ? -1.0F
          : static_cast<float>(m_totalStateDurations) / static_cast<float>(m_totalStateChanges);
  logVal(MODULE, "averageStateDuration", avStateDuration);
  for (size_t i = 0; i < m_numStateChanges.size(); i++)
  {
    logVal(MODULE, "numState_" + std::to_string(i) + "_Changes", m_numStateChanges[i]);
  }
  for (size_t i = 0; i < m_stateDurations.size(); i++)
  {
    const float avStateTime =
        m_numStateChanges[i] == 0
            ? -1.0F
            : static_cast<float>(m_stateDurations[i]) / static_cast<float>(m_numStateChanges[i]);
    logVal(MODULE, "averageState_" + std::to_string(i) + "_Duration", avStateTime);
  }
  logVal(MODULE, "totalFilterModeChanges", m_totalFilterModeChanges);
  for (size_t i = 0; i < m_numFilterModeChanges.size(); i++)
  {
    logVal(MODULE, "numFilterMode_" + EnumToString(static_cast<ZoomFilterMode>(i)) + "_Changes",
           m_numFilterModeChanges[i]);
  }
  logVal(MODULE, "numLockChanges", m_numLockChanges);
  logVal(MODULE, "numDoIFS", m_numDoIFS);
  logVal(MODULE, "numDoDots", m_numDoDots);
  logVal(MODULE, "numDoLines", m_numDoLines);
  logVal(MODULE, "numDoStars", m_numDoStars);
  logVal(MODULE, "numDoTentacles", m_numDoTentacles);
  logVal(MODULE, "numDisabledTentacles", m_numDisabledTentacles);
  logVal(MODULE, "numLastTimeGoomChanges", m_numLastTimeGoomChanges);
  logVal(MODULE, "numMegaLentChanges", m_numMegaLentChanges);
  logVal(MODULE, "numDoNoise", m_numDoNoise);
  logVal(MODULE, "numTurnOffNoise", m_numTurnOffNoise);
  logVal(MODULE, "numIfsRenew", m_numIfsRenew);
  logVal(MODULE, "numChangeLineColor", m_numChangeLineColor);
  logVal(MODULE, "numSwitchLines", m_numSwitchLines);
  logVal(MODULE, "numBlockyWavy", m_numBlockyWavy);
  logVal(MODULE, "numZoomFilterAllowOverexposed", m_numZoomFilterAllowOverexposed);
  logVal(MODULE, "numTooManyClipped", m_numTooManyClipped);
}

void GoomStats::SetSongTitle(const std::string& s)
{
  m_songTitle = s;
}

void GoomStats::SetStateStartValue(const uint32_t stateIndex)
{
  m_startingState = stateIndex;
}

void GoomStats::SetZoomFilterStartValue(const ZoomFilterMode filterMode)
{
  m_startingFilterMode = filterMode;
}

void GoomStats::SetStateLastValue(const uint32_t stateIndex)
{
  m_lastState = stateIndex;
}

void GoomStats::SetZoomFilterLastValue(const ZoomFilterData* filterData)
{
  m_lastZoomFilterData = filterData;
}

void GoomStats::SetSeedStartValue(const uint64_t seed)
{
  m_startingSeed = seed;
}

void GoomStats::SetSeedLastValue(const uint64_t seed)
{
  m_lastSeed = seed;
}

void GoomStats::SetNumThreadsUsedValue(const size_t n)
{
  m_numThreadsUsed = n;
}

void GoomStats::SetFontFileUsed(const std::string& f)
{
  m_fontFileUsed = f;
}

void GoomStats::UpdateChange(const size_t currentState, const ZoomFilterMode currentFilterMode)
{
  const auto timeNow = std::chrono::high_resolution_clock::now();
  if (m_numUpdates > 0)
  {
    using Ms = std::chrono::milliseconds;
    const Ms diff = std::chrono::duration_cast<Ms>(timeNow - m_timeNowHiRes);
    const auto timeInUpdateMs = static_cast<uint32_t>(diff.count());
    if (timeInUpdateMs < m_minTimeInUpdatesMs)
    {
      m_minTimeInUpdatesMs = timeInUpdateMs;
      m_stateAtMin = currentState;
      m_filterModeAtMin = currentFilterMode;
    }
    else if (timeInUpdateMs > m_maxTimeInUpdatesMs)
    {
      m_maxTimeInUpdatesMs = timeInUpdateMs;
      m_stateAtMax = currentState;
      m_filterModeAtMax = currentFilterMode;
    }
    m_totalTimeInUpdatesMs += timeInUpdateMs;
  }
  m_timeNowHiRes = timeNow;

  m_numUpdates++;
}

void GoomStats::StateChange(const uint32_t timeInState)
{
  m_totalStateChanges++;
  m_totalStateDurations += timeInState;
}

void GoomStats::StateChange(const size_t index, const uint32_t timeInState)
{
  if (index >= m_numStateChanges.size())
  {
    for (size_t i = m_numStateChanges.size(); i <= index; i++)
    {
      m_numStateChanges.push_back(0);
    }
  }
  m_numStateChanges[index]++;

  if (index >= m_stateDurations.size())
  {
    for (size_t i = m_stateDurations.size(); i <= index; i++)
    {
      m_stateDurations.push_back(0);
    }
  }
  m_stateDurations[index] += timeInState;
}

void GoomStats::FilterModeChange()
{
  m_totalFilterModeChanges++;
}

void GoomStats::FilterModeChange(const ZoomFilterMode mode)
{
  m_numFilterModeChanges.at(static_cast<size_t>(mode))++;
}

void GoomStats::LockChange()
{
  m_numLockChanges++;
}

void GoomStats::DoIfs()
{
  m_numDoIFS++;
}

void GoomStats::DoDots()
{
  m_numDoDots++;
}

void GoomStats::DoLines()
{
  m_numDoLines++;
}

void GoomStats::DoStars()
{
  m_numDoStars++;
}

void GoomStats::DoTentacles()
{
  m_numDoTentacles++;
}

void GoomStats::TentaclesDisabled()
{
  m_numDisabledTentacles++;
}

void GoomStats::DoBigUpdate()
{
  m_numBigUpdates++;
}

void GoomStats::LastTimeGoomChange()
{
  m_numLastTimeGoomChanges++;
}

void GoomStats::MegaLentChange()
{
  m_numMegaLentChanges++;
}

void GoomStats::DoNoise()
{
  m_numDoNoise++;
}

void GoomStats::DoTurnOffNoise()
{
  m_numTurnOffNoise++;
}

void GoomStats::IfsRenew()
{
  m_numIfsRenew++;
}

void GoomStats::ChangeLineColor()
{
  m_numChangeLineColor++;
}

void GoomStats::SwitchLines()
{
  m_numSwitchLines++;
}

void GoomStats::DoBlockyWavy()
{
  m_numBlockyWavy++;
}

void GoomStats::DoZoomFilterAllowOverexposed()
{
  m_numZoomFilterAllowOverexposed++;
}

void GoomStats::TooManyClipped()
{
  m_numTooManyClipped++;
}

class LogStatsVisitor
{
public:
  LogStatsVisitor(const std::string& m, const std::string& n) : m_module{m}, m_name{n} {}
  void operator()(const std::string& s) const { logInfo("{}.{} = '{}'", m_module, m_name, s); }
  void operator()(const uint32_t i) const { logInfo("{}.{} = {}", m_module, m_name, i); }
  void operator()(const int32_t i) const { logInfo("{}.{} = {}", m_module, m_name, i); }
  void operator()(const uint64_t i) const { logInfo("{}.{} = {}", m_module, m_name, i); }
  void operator()(const float f) const { logInfo("{}.{} = {:.3}", m_module, m_name, f); }

private:
  const std::string& m_module;
  const std::string& m_name;
};

void GoomStats::LogStatsValue(const std::string& module,
                              const std::string& name,
                              const StatsLogValue& logValue)
{
#if __cplusplus <= 201402L
  const LogStatsVisitor s_visitor{module, name};
  switch (logValue.type)
  {
    case StatsLogValue::Type::STRING:
      s_visitor(logValue.str);
      break;
    case StatsLogValue::Type::UINT32:
      s_visitor(logValue.ui32);
      break;
    case StatsLogValue::Type::INT32:
      s_visitor(logValue.i32);
      break;
    case StatsLogValue::Type::UINT64:
      s_visitor(logValue.ui64);
      break;
    case StatsLogValue::Type::FLOAT:
      s_visitor(logValue.flt);
      break;
  }
#else
  std::visit(LogStatsVisitor{module, name}, value);
#endif
}

} // namespace GOOM
