#include "tentacles_fx.h"

#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormaps.h"
#include "goomutils/colorutils.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
#include "goomutils/mathutils.h"
//#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/random_colormaps.h"
#include "tentacle_driver.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

CEREAL_REGISTER_TYPE(GOOM::TentaclesFx)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GOOM::IVisualFx, GOOM::TentaclesFx)

namespace GOOM
{

using namespace GOOM::UTILS;

inline auto StartPrettyMoveEvent() -> bool
{
  return ProbabilityOfMInN(1, 400);
}

inline auto ChangeRotationEvent() -> bool
{
  return ProbabilityOfMInN(1, 100);
}

inline auto TurnRotationOnEvent() -> bool
{
  return ProbabilityOfMInN(1, 2);
}

inline auto ChangeDominantColorMapEvent() -> bool
{
  return ProbabilityOfMInN(1, 50);
}

// IMPORTANT. Very delicate here - 1 in 30 seems just right
inline auto ChangeDominantColorEvent() -> bool
{
  return ProbabilityOfMInN(1, 30);
}

class TentacleStats
{
public:
  TentacleStats() noexcept = default;

  void Reset();
  void Log(const StatsLogValueFunc& l) const;
  void UpdateStart();
  void UpdateEnd();
  void ChangeDominantColorMap();
  void ChangeDominantColor();
  void UpdateWithDraw();
  void UpdateWithNoDraw();
  void UpdateWithPrettyMoveNoDraw();
  void UpdateWithPrettyMoveDraw();
  void LowToMediumAcceleration();
  void HighAcceleration();
  void CycleReset();
  void PrettyMoveHappens();
  void ChangePrettyLerpMixLower();
  void ChangePrettyLerpMixHigher();
  void SetNumTentacleDrivers(const std::vector<std::unique_ptr<TentacleDriver>>& d);
  void ChangeTentacleDriver(uint32_t driverIndex);

  void SetLastNumTentacles(size_t val);
  void SetLastUpdatingWithDraw(bool val);
  void SetLastCycle(float val);
  void SetLastCycleInc(float val);
  void SetLastLig(float val);
  void SetLastLigs(float val);
  void SetLastDistt(float val);
  void SetLastDistt2(float val);
  void SetLastDistt2Offset(float val);
  void SetLastRot(float val);
  void SetLastRotAtStartOfPrettyMove(float val);
  void SetLastDoRotation(bool val);
  void SetLastIsPrettyMoveHappening(bool val);
  void SetLastPrettyMoveHappeningTimer(int32_t val);
  void SetLastPrettyMoveCheckStopMark(int32_t val);
  void SetLastPrePrettyMoveLock(int32_t val);
  void SetLastDistt2OffsetPreStep(float val);
  void SetLastPrettyMoveReadyToStart(bool val);
  void SetLastPostPrettyMoveLock(int32_t val);
  void SetLastPrettyLerpMixValue(float val);

private:
  uint64_t m_totalTimeInUpdatesMs = 0;
  uint32_t m_minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  uint32_t m_maxTimeInUpdatesMs = 0;
  std::chrono::high_resolution_clock::time_point m_timeNowHiRes{};

  uint32_t m_numDominantColorMapChanges = 0;
  uint32_t m_numDominantColorChanges = 0;
  uint32_t m_numUpdatesWithDraw = 0;
  uint32_t m_numUpdatesWithNoDraw = 0;
  uint32_t m_numUpdatesWithPrettyMoveNoDraw = 0;
  uint32_t m_numUpdatesWithPrettyMoveDraw = 0;
  uint32_t m_numLowToMediumAcceleration = 0;
  uint32_t m_numHighAcceleration = 0;
  uint32_t m_numCycleResets = 0;
  uint32_t m_numPrettyMoveHappens = 0;
  uint32_t m_numTentacleDrivers = 0;
  std::vector<uint32_t> m_numDriverTentacles{};
  std::vector<uint32_t> m_numDriverChanges{};

  size_t m_lastNumTentacles = 0;
  bool m_lastUpdatingWithDraw = false;
  float m_lastCycle = 0.0;
  float m_lastCycleInc = 0.0;
  float m_lastLig = 0.0;
  float m_lastLigs = 0.0;
  float m_lastDistt = 0.0;
  float m_lastDistt2 = 0.0;
  float m_lastDistt2Offset = 0.0;
  float m_lastRot = 0.0;
  float m_lastRotAtStartOfPrettyMove = 0.0;
  bool m_lastDoRotation = false;
  bool m_lastIsPrettyMoveHappening = false;
  int32_t m_lastPrettyMoveHappeningTimer = 0;
  int32_t m_lastPrettyMoveCheckStopMark = 0;
  int32_t m_lastPrePrettyMoveLock = 0;
  float m_lastDistt2OffsetPreStep = 0.0;
  bool m_lastPrettyMoveReadyToStart = false;
  int32_t m_lastPostPrettyMoveLock = 0;
  float m_lastPrettyLerpMixValue = 0.0;
};

void TentacleStats::Reset()
{
  m_totalTimeInUpdatesMs = 0;
  m_minTimeInUpdatesMs = std::numeric_limits<uint32_t>::max();
  m_maxTimeInUpdatesMs = 0;
  m_timeNowHiRes = std::chrono::high_resolution_clock::now();

  m_numDominantColorMapChanges = 0;
  m_numDominantColorChanges = 0;
  m_numUpdatesWithDraw = 0;
  m_numUpdatesWithNoDraw = 0;
  m_numUpdatesWithPrettyMoveNoDraw = 0;
  m_numUpdatesWithPrettyMoveDraw = 0;
  m_numLowToMediumAcceleration = 0;
  m_numHighAcceleration = 0;
  m_numCycleResets = 0;
  m_numPrettyMoveHappens = 0;
  std::fill(m_numDriverChanges.begin(), m_numDriverChanges.end(), 0);
}

void TentacleStats::Log(const StatsLogValueFunc& logVal) const
{
  const constexpr char* MODULE = "Tentacles";

  const uint32_t numUpdates = m_numUpdatesWithDraw;
  const int32_t avTimeInUpdateMs =
      std::lround(numUpdates == 0 ? -1.0
                                  : static_cast<float>(m_totalTimeInUpdatesMs) /
                                        static_cast<float>(numUpdates));
  logVal(MODULE, "avTimeInUpdateMs", avTimeInUpdateMs);
  logVal(MODULE, "minTimeInUpdatesMs", m_minTimeInUpdatesMs);
  logVal(MODULE, "maxTimeInUpdatesMs", m_maxTimeInUpdatesMs);

  logVal(MODULE, "lastNumTentacles", m_lastNumTentacles);
  logVal(MODULE, "lastUpdatingWithDraw", m_lastUpdatingWithDraw);
  logVal(MODULE, "lastCycle", m_lastCycle);
  logVal(MODULE, "lastCycleInc", m_lastCycleInc);
  logVal(MODULE, "lastLig", m_lastLig);
  logVal(MODULE, "lastLigs", m_lastLigs);
  logVal(MODULE, "lastDistt", m_lastDistt);
  logVal(MODULE, "lastDistt2", m_lastDistt2);
  logVal(MODULE, "lastDistt2Offset", m_lastDistt2Offset);
  logVal(MODULE, "lastRot", m_lastRot);
  logVal(MODULE, "lastRotAtStartOfPrettyMove", m_lastRotAtStartOfPrettyMove);
  logVal(MODULE, "lastDoRotation", m_lastDoRotation);
  logVal(MODULE, "lastIsPrettyMoveHappening", m_lastIsPrettyMoveHappening);
  logVal(MODULE, "lastPrettyMoveHappeningTimer", m_lastPrettyMoveHappeningTimer);
  logVal(MODULE, "lastPrettyMoveCheckStopMark", m_lastPrettyMoveCheckStopMark);
  logVal(MODULE, "lastPrePrettyMoveLock", m_lastPrePrettyMoveLock);
  logVal(MODULE, "lastDistt2OffsetPreStep", m_lastDistt2OffsetPreStep);
  logVal(MODULE, "lastPrettyMoveReadyToStart", m_lastPrettyMoveReadyToStart);
  logVal(MODULE, "lastPostPrettyMoveLock", m_lastPostPrettyMoveLock);
  logVal(MODULE, "lastPrettyLerpMixValue", m_lastPrettyLerpMixValue);

  logVal(MODULE, "numDominantColorMapChanges", m_numDominantColorMapChanges);
  logVal(MODULE, "numDominantColorChanges", m_numDominantColorChanges);
  logVal(MODULE, "numUpdates", numUpdates);
  logVal(MODULE, "numUpdatesWithDraw", m_numUpdatesWithDraw);
  logVal(MODULE, "numUpdatesWithNoDraw", m_numUpdatesWithNoDraw);
  logVal(MODULE, "numUpdatesWithPrettyMoveNoDraw", m_numUpdatesWithPrettyMoveNoDraw);
  logVal(MODULE, "numUpdatesWithPrettyMoveDraw", m_numUpdatesWithPrettyMoveDraw);
  logVal(MODULE, "numLowToMediumAcceleration", m_numLowToMediumAcceleration);
  logVal(MODULE, "numHighAcceleration", m_numHighAcceleration);
  logVal(MODULE, "numCycleResets", m_numCycleResets);
  logVal(MODULE, "numPrettyMoveHappens", m_numPrettyMoveHappens);
  logVal(MODULE, "numTentacleDrivers", m_numTentacleDrivers);
  // TODO Make this a string util function
  std::string numTentaclesStr;
  std::string numDriverChangesStr;
  for (size_t i = 0; i < m_numDriverTentacles.size(); i++)
  {
    if (i)
    {
      numTentaclesStr += ", ";
      numDriverChangesStr += ", ";
    }
    numTentaclesStr += std::to_string(m_numDriverTentacles[i]);
    numDriverChangesStr += std::to_string(m_numDriverChanges[i]);
  }
  logVal(MODULE, "numDriverTentacles", numTentaclesStr);
  logVal(MODULE, "numDriverChangesStr", numDriverChangesStr);
}

inline void TentacleStats::UpdateStart()
{
  m_timeNowHiRes = std::chrono::high_resolution_clock::now();
}

inline void TentacleStats::UpdateEnd()
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

inline void TentacleStats::ChangeDominantColorMap()
{
  m_numDominantColorMapChanges++;
}

inline void TentacleStats::ChangeDominantColor()
{
  m_numDominantColorChanges++;
}

inline void TentacleStats::UpdateWithDraw()
{
  m_numUpdatesWithDraw++;
}

inline void TentacleStats::UpdateWithNoDraw()
{
  m_numUpdatesWithNoDraw++;
}

inline void TentacleStats::UpdateWithPrettyMoveNoDraw()
{
  m_numUpdatesWithPrettyMoveNoDraw++;
}

inline void TentacleStats::UpdateWithPrettyMoveDraw()
{
  m_numUpdatesWithPrettyMoveDraw++;
}

inline void TentacleStats::LowToMediumAcceleration()
{
  m_numLowToMediumAcceleration++;
}

inline void TentacleStats::HighAcceleration()
{
  m_numHighAcceleration++;
}

inline void TentacleStats::CycleReset()
{
  m_numCycleResets++;
}

inline void TentacleStats::PrettyMoveHappens()
{
  m_numPrettyMoveHappens++;
}

inline void TentacleStats::SetLastNumTentacles(const size_t val)
{
  m_lastNumTentacles = val;
}

inline void TentacleStats::SetLastUpdatingWithDraw(const bool val)
{
  m_lastUpdatingWithDraw = val;
}

inline void TentacleStats::SetLastCycle(const float val)
{
  m_lastCycle = val;
}

inline void TentacleStats::SetLastCycleInc(const float val)
{
  m_lastCycleInc = val;
}

inline void TentacleStats::SetLastLig(const float val)
{
  m_lastLig = val;
}

inline void TentacleStats::SetLastLigs(const float val)
{
  m_lastLigs = val;
}

inline void TentacleStats::SetLastDistt(const float val)
{
  m_lastDistt = val;
}

inline void TentacleStats::SetLastDistt2(const float val)
{
  m_lastDistt2 = val;
}

inline void TentacleStats::SetLastDistt2Offset(const float val)
{
  m_lastDistt2Offset = val;
}

inline void TentacleStats::SetLastRot(const float val)
{
  m_lastRot = val;
}

inline void TentacleStats::SetLastRotAtStartOfPrettyMove(const float val)
{
  m_lastRotAtStartOfPrettyMove = val;
}

inline void TentacleStats::SetLastDoRotation(const bool val)
{
  m_lastDoRotation = val;
}

inline void TentacleStats::SetLastIsPrettyMoveHappening(const bool val)
{
  m_lastIsPrettyMoveHappening = val;
}

inline void TentacleStats::SetLastPrettyMoveHappeningTimer(const int32_t val)
{
  m_lastPrettyMoveHappeningTimer = val;
}

inline void TentacleStats::SetLastPrettyMoveCheckStopMark(const int32_t val)
{
  m_lastPrettyMoveCheckStopMark = val;
}

inline void TentacleStats::SetLastPrePrettyMoveLock(const int32_t val)
{
  m_lastPrePrettyMoveLock = val;
}

inline void TentacleStats::SetLastDistt2OffsetPreStep(const float val)
{
  m_lastDistt2OffsetPreStep = val;
}

inline void TentacleStats::SetLastPrettyMoveReadyToStart(const bool val)
{
  m_lastPrettyMoveReadyToStart = val;
}

inline void TentacleStats::SetLastPostPrettyMoveLock(const int32_t val)
{
  m_lastPostPrettyMoveLock = val;
}

inline void TentacleStats::SetLastPrettyLerpMixValue(const float val)
{
  m_lastPrettyLerpMixValue = val;
}

void TentacleStats::SetNumTentacleDrivers(const std::vector<std::unique_ptr<TentacleDriver>>& d)
{
  m_numTentacleDrivers = d.size();
  m_numDriverTentacles.resize(m_numTentacleDrivers);
  m_numDriverChanges.resize(m_numTentacleDrivers);
  for (size_t i = 0; i < m_numTentacleDrivers; i++)
  {
    m_numDriverTentacles[i] = d[i]->GetNumTentacles();
    m_numDriverChanges[i] = 0;
  }
}

void TentacleStats::ChangeTentacleDriver(const uint32_t driverIndex)
{
  m_numDriverChanges.at(driverIndex)++;
}

class TentaclesFx::TentaclesImpl
{
public:
  TentaclesImpl() noexcept;
  explicit TentaclesImpl(const std::shared_ptr<const PluginInfo>& goomInfo);
  ~TentaclesImpl() noexcept = default;
  TentaclesImpl(const TentaclesImpl&) noexcept = delete;
  TentaclesImpl(TentaclesImpl&&) noexcept = delete;
  auto operator=(const TentaclesImpl&) -> TentaclesImpl& = delete;
  auto operator=(TentaclesImpl&&) -> TentaclesImpl& = delete;

  [[nodiscard]] auto GetBuffSettings() const -> const FXBuffSettings&;
  void SetBuffSettings(const FXBuffSettings& settings);
  void FreshStart();

  void Update(PixelBuffer& currentBuff, PixelBuffer& nextBuff);
  void UpdateWithNoDraw();

  void LogStats(const StatsLogValueFunc& logVal);

  auto operator==(const TentaclesImpl& t) const -> bool;

private:
  std::shared_ptr<const PluginInfo> m_goomInfo{};
  WeightedColorMaps m_colorMaps{Weights<ColorMapGroup>{{
      {ColorMapGroup::PERCEPTUALLY_UNIFORM_SEQUENTIAL, 10},
      {ColorMapGroup::SEQUENTIAL, 10},
      {ColorMapGroup::SEQUENTIAL2, 10},
      {ColorMapGroup::CYCLIC, 10},
      {ColorMapGroup::DIVERGING, 20},
      {ColorMapGroup::DIVERGING_BLACK, 20},
      {ColorMapGroup::QUALITATIVE, 10},
      {ColorMapGroup::MISC, 20},
  }}};
  std::shared_ptr<const IColorMap> m_dominantColorMap{};
  Pixel m_dominantColor{};
  void ChangeDominantColor();

  bool m_updatingWithDraw = false;
  float m_cycle = 0.0;
  static constexpr float CYCLE_INC_MIN = 0.01;
  static constexpr float CYCLE_INC_MAX = 0.05;
  float m_cycleInc = CYCLE_INC_MIN;
  float m_lig = 1.15;
  float m_ligs = 0.1;
  float m_distt = 10.0;
  static constexpr double DISTT_MIN = 106.0;
  static constexpr double DISTT_MAX = 286.0;
  float m_distt2 = 0.0;
  static constexpr float DISTT2_MIN = 8.0;
  static constexpr float DISTT2_MAX = 1000.0;
  float m_distt2Offset = 0.0;
  float m_rot = 0.0; // entre 0 et m_two_pi
  float m_rotAtStartOfPrettyMove = 0.0;
  bool m_doRotation = false;
  static auto GetStableRotationOffset(float cycleVal) -> float;
  bool m_isPrettyMoveHappening = false;
  int32_t m_prettyMoveHappeningTimer = 0;
  int32_t m_prettyMoveCheckStopMark = 0;
  static constexpr uint32_t PRETTY_MOVE_HAPPENING_MIN = 100;
  static constexpr uint32_t PRETTY_MOVE_HAPPENING_MAX = 200;
  float m_distt2OffsetPreStep = 0.0;

  bool m_prettyMoveReadyToStart = false;
  static constexpr int32_t MIN_PRE_PRETTY_MOVE_LOCK = 200;
  static constexpr int32_t MAX_PRE_PRETTY_MOVE_LOCK = 500;
  int32_t m_prePrettyMoveLock = 0;
  int32_t m_postPrettyMoveLock = 0;
  float m_prettyMoveLerpMix = 1.0 / 16.0; // original goom value
  void IsPrettyMoveHappeningUpdate(float acceleration);
  void PrettyMovePreStart();
  void PrettyMoveStart(float acceleration, int32_t timerVal = -1);
  void PrettyMoveFinish();
  void PrettyMove(float acceleration);
  void PrettyMoveWithNoDraw();
  auto GetModColors() -> std::tuple<Pixel, Pixel>;

  size_t m_countSinceHighAccelLastMarked = 0;
  size_t m_countSinceColorChangeLastMarked = 0;
  void IncCounters();

  static constexpr size_t NUM_DRIVERS = 4;
  std::vector<std::unique_ptr<TentacleDriver>> m_drivers{};
  TentacleDriver* m_currentDriver = nullptr;
  auto GetNextDriver() const -> TentacleDriver*;
  // clang-format off
  const Weights<size_t> m_driverWeights{{
      {0,  5},
      {1, 15},
      {2, 15},
      {3,  5},
  }};
  const std::vector<CirclesTentacleLayout> m_layouts{
      {10,  80, {16, 12,  8,  6, 4}, 0},
      {10,  80, {20, 16, 12,  6, 4}, 0},
      {10, 100, {30, 20, 14,  6, 4}, 0},
      {10, 110, {36, 26, 20, 12, 6}, 0},
  };
  // clang-format on

  void SetupDrivers();
  void Init();
  mutable TentacleStats m_stats{};

  friend class cereal::access;
  template<class Archive>
  void save(Archive& ar) const;
  template<class Archive>
  void load(Archive& ar);
};

TentaclesFx::TentaclesFx() noexcept : m_fxImpl{new TentaclesImpl{}}
{
}

TentaclesFx::TentaclesFx(const std::shared_ptr<const PluginInfo>& info) noexcept
  : m_fxImpl{new TentaclesImpl{info}}
{
}

TentaclesFx::~TentaclesFx() noexcept = default;

auto TentaclesFx::operator==(const TentaclesFx& t) const -> bool
{
  return m_fxImpl->operator==(*t.m_fxImpl);
}

void TentaclesFx::SetBuffSettings(const FXBuffSettings& settings)
{
  m_fxImpl->SetBuffSettings(settings);
}

void TentaclesFx::FreshStart()
{
  m_fxImpl->FreshStart();
}

void TentaclesFx::Start()
{
}

void TentaclesFx::Finish()
{
}

void TentaclesFx::Log(const StatsLogValueFunc& logVal) const
{
  m_fxImpl->LogStats(logVal);
}

void TentaclesFx::Apply([[maybe_unused]] PixelBuffer& currentBuff)
{
  throw std::logic_error("TentaclesFx::Apply should never be called.");
}

void TentaclesFx::Apply(PixelBuffer& currentBuff, PixelBuffer& nextBuff)
{
  if (!m_enabled)
  {
    return;
  }

  m_fxImpl->Update(currentBuff, nextBuff);
}

void TentaclesFx::ApplyNoDraw()
{
  if (!m_enabled)
  {
    return;
  }

  m_fxImpl->UpdateWithNoDraw();
}

auto TentaclesFx::GetFxName() const -> std::string
{
  return "Tentacles FX";
}

template<class Archive>
void TentaclesFx::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_enabled), CEREAL_NVP(m_fxImpl));
}

// Need to explicitly instantiate template functions for serialization.
template void TentaclesFx::serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);
template void TentaclesFx::serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template void TentaclesFx::TentaclesImpl::save<cereal::JSONOutputArchive>(
    cereal::JSONOutputArchive&) const;
template void TentaclesFx::TentaclesImpl::load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

template<class Archive>
void TentaclesFx::TentaclesImpl::save(Archive& ar) const
{
  ar(CEREAL_NVP(m_goomInfo), CEREAL_NVP(m_dominantColor), CEREAL_NVP(m_updatingWithDraw),
     CEREAL_NVP(m_cycle), CEREAL_NVP(m_cycleInc), CEREAL_NVP(m_lig), CEREAL_NVP(m_ligs),
     CEREAL_NVP(m_distt), CEREAL_NVP(m_distt2), CEREAL_NVP(m_distt2Offset), CEREAL_NVP(m_rot),
     CEREAL_NVP(m_rotAtStartOfPrettyMove), CEREAL_NVP(m_doRotation),
     CEREAL_NVP(m_isPrettyMoveHappening), CEREAL_NVP(m_prettyMoveHappeningTimer),
     CEREAL_NVP(m_prettyMoveCheckStopMark), CEREAL_NVP(m_distt2OffsetPreStep),
     CEREAL_NVP(m_prettyMoveReadyToStart), CEREAL_NVP(m_prePrettyMoveLock),
     CEREAL_NVP(m_postPrettyMoveLock), CEREAL_NVP(m_prettyMoveLerpMix),
     CEREAL_NVP(m_countSinceHighAccelLastMarked), CEREAL_NVP(m_countSinceColorChangeLastMarked),
     CEREAL_NVP(m_drivers));
}

template<class Archive>
void TentaclesFx::TentaclesImpl::load(Archive& ar)
{
  ar(CEREAL_NVP(m_goomInfo), CEREAL_NVP(m_dominantColor), CEREAL_NVP(m_updatingWithDraw),
     CEREAL_NVP(m_cycle), CEREAL_NVP(m_cycleInc), CEREAL_NVP(m_lig), CEREAL_NVP(m_ligs),
     CEREAL_NVP(m_distt), CEREAL_NVP(m_distt2), CEREAL_NVP(m_distt2Offset), CEREAL_NVP(m_rot),
     CEREAL_NVP(m_rotAtStartOfPrettyMove), CEREAL_NVP(m_doRotation),
     CEREAL_NVP(m_isPrettyMoveHappening), CEREAL_NVP(m_prettyMoveHappeningTimer),
     CEREAL_NVP(m_prettyMoveCheckStopMark), CEREAL_NVP(m_distt2OffsetPreStep),
     CEREAL_NVP(m_prettyMoveReadyToStart), CEREAL_NVP(m_prePrettyMoveLock),
     CEREAL_NVP(m_postPrettyMoveLock), CEREAL_NVP(m_prettyMoveLerpMix),
     CEREAL_NVP(m_countSinceHighAccelLastMarked), CEREAL_NVP(m_countSinceColorChangeLastMarked),
     CEREAL_NVP(m_drivers));
}

auto TentaclesFx::TentaclesImpl::operator==(const TentaclesImpl& t) const -> bool
{
  if (m_goomInfo == nullptr && t.m_goomInfo != nullptr)
  {
    return false;
  }
  if (m_goomInfo != nullptr && t.m_goomInfo == nullptr)
  {
    return false;
  }

  bool result =
      ((m_goomInfo == nullptr && t.m_goomInfo == nullptr) || (*m_goomInfo == *t.m_goomInfo)) &&
      m_dominantColor == t.m_dominantColor && m_updatingWithDraw == t.m_updatingWithDraw &&
      m_cycle == t.m_cycle && m_cycleInc == t.m_cycleInc && m_lig == t.m_lig &&
      m_ligs == t.m_ligs && m_distt == t.m_distt && m_distt2 == t.m_distt2 &&
      m_distt2Offset == t.m_distt2Offset && m_rot == t.m_rot &&
      m_rotAtStartOfPrettyMove == t.m_rotAtStartOfPrettyMove && m_doRotation == t.m_doRotation &&
      m_isPrettyMoveHappening == t.m_isPrettyMoveHappening &&
      m_prettyMoveHappeningTimer == t.m_prettyMoveHappeningTimer &&
      m_prettyMoveCheckStopMark == t.m_prettyMoveCheckStopMark &&
      m_distt2OffsetPreStep == t.m_distt2OffsetPreStep &&
      m_prettyMoveReadyToStart == t.m_prettyMoveReadyToStart &&
      m_prePrettyMoveLock == t.m_prePrettyMoveLock &&
      m_postPrettyMoveLock == t.m_postPrettyMoveLock &&
      m_prettyMoveLerpMix == t.m_prettyMoveLerpMix &&
      m_countSinceHighAccelLastMarked == t.m_countSinceHighAccelLastMarked &&
      m_countSinceColorChangeLastMarked == t.m_countSinceColorChangeLastMarked;

  if (result)
  {
    for (size_t i = 0; i < m_drivers.size(); i++)
    {
      if (*m_drivers[i] != *t.m_drivers[i])
      {
        logInfo("TentaclesFx driver differs at index {}", i);
        return false;
      }
    }
  }

  if (!result)
  {
    logInfo("TentaclesFx result == {}", result);
    logInfo("dominantColor == t.dominantColor = {}", dominantColor == t.dominantColor);
    logInfo("updatingWithDraw == t.updatingWithDraw = {}", updatingWithDraw == t.updatingWithDraw);
    logInfo("cycle == t.cycle = {}", cycle == t.cycle);
    logInfo("cycleInc == t.cycleInc = {}", cycleInc == t.cycleInc);
    logInfo("lig == t.lig = {}", lig == t.lig);
    logInfo("ligs == t.ligs = {}", ligs == t.ligs);
    logInfo("distt == t.distt = {}", distt == t.distt);
    logInfo("distt2 == t.distt2 = {}", distt2 == t.distt2);
    logInfo("distt2Offset == t.distt2Offset = {}", distt2Offset == t.distt2Offset);
    logInfo("rot == t.rot = {}", rot == t.rot);
    logInfo("drivers == t.drivers = {}", drivers == t.drivers);
  }

  return result;
}

TentaclesFx::TentaclesImpl::TentaclesImpl() noexcept = default;

TentaclesFx::TentaclesImpl::TentaclesImpl(const std::shared_ptr<const PluginInfo>& info)
  : m_goomInfo{info},
    m_dominantColorMap{m_colorMaps.GetRandomColorMapPtr(RandomColorMaps::ALL)},
    m_dominantColor{RandomColorMaps::GetRandomColor(*m_dominantColorMap, 0.0F, 1.0F)},
    m_rot{GetStableRotationOffset(0)}
{
  SetupDrivers();
}

void TentaclesFx::TentaclesImpl::SetupDrivers()
{
  // const GridTentacleLayout layout{ -100, 100, xRowLen, -100, 100, numXRows, 0 };
  if (NUM_DRIVERS != m_layouts.size())
  {
    throw std::logic_error("numDrivers != layouts.size()");
  }

  /**
// Temp hack of weights
Weights<ColorMapGroup> colorGroupWeights = colorMaps.getWeights();
colorGroupWeights.ClearWeights(1);
colorGroupWeights.SetWeight(ColorMapGroup::misc, 30000);
colorMaps.setWeights(colorGroupWeights);
***/

  for (size_t i = 0; i < NUM_DRIVERS; i++)
  {
    (void)m_drivers.emplace_back(
        new TentacleDriver{m_goomInfo->GetScreenInfo().width, m_goomInfo->GetScreenInfo().height});
  }

  if (NUM_DRIVERS != m_driverWeights.GetNumElements())
  {
    throw std::logic_error("numDrivers != driverWeights.GetNumElements()");
  }

  for (size_t i = 0; i < NUM_DRIVERS; i++)
  {
    m_drivers[i]->Init(m_layouts[i]);
    m_drivers[i]->StartIterating();
  }
  m_stats.SetNumTentacleDrivers(m_drivers);

  m_currentDriver = GetNextDriver();
  m_currentDriver->StartIterating();
  Init();
}

inline auto TentaclesFx::TentaclesImpl::GetBuffSettings() const -> const FXBuffSettings&
{
  return m_currentDriver->GetBuffSettings();
}

inline void TentaclesFx::TentaclesImpl::SetBuffSettings(const FXBuffSettings& settings)
{
  m_currentDriver->SetBuffSettings(settings);
}

inline void TentaclesFx::TentaclesImpl::IncCounters()
{
  m_countSinceHighAccelLastMarked++;
  m_countSinceColorChangeLastMarked++;
}

void TentaclesFx::TentaclesImpl::LogStats(const StatsLogValueFunc& logVal)
{
  m_stats.SetLastNumTentacles(m_currentDriver->GetNumTentacles());
  m_stats.SetLastUpdatingWithDraw(m_updatingWithDraw);
  m_stats.SetLastCycle(m_cycle);
  m_stats.SetLastCycleInc(m_cycleInc);
  m_stats.SetLastLig(m_lig);
  m_stats.SetLastLigs(m_ligs);
  m_stats.SetLastDistt(m_distt);
  m_stats.SetLastDistt2(m_distt2);
  m_stats.SetLastDistt2Offset(m_distt2Offset);
  m_stats.SetLastRot(m_rot);
  m_stats.SetLastRotAtStartOfPrettyMove(m_rotAtStartOfPrettyMove);
  m_stats.SetLastDoRotation(m_doRotation);
  m_stats.SetLastIsPrettyMoveHappening(m_isPrettyMoveHappening);
  m_stats.SetLastPrettyMoveHappeningTimer(m_prettyMoveHappeningTimer);
  m_stats.SetLastPrettyMoveCheckStopMark(m_prettyMoveCheckStopMark);
  m_stats.SetLastPrePrettyMoveLock(m_prePrettyMoveLock);
  m_stats.SetLastDistt2OffsetPreStep(m_distt2OffsetPreStep);
  m_stats.SetLastPrettyMoveReadyToStart(m_prettyMoveReadyToStart);
  m_stats.SetLastPostPrettyMoveLock(m_postPrettyMoveLock);
  m_stats.SetLastPrettyLerpMixValue(m_prettyMoveLerpMix);

  m_stats.Log(logVal);
}

void TentaclesFx::TentaclesImpl::UpdateWithNoDraw()
{
  m_stats.UpdateWithNoDraw();

  if (!m_updatingWithDraw)
  {
    // already in tentacle no draw state
    return;
  }

  m_updatingWithDraw = false;

  if (m_ligs > 0.0F)
  {
    m_ligs = -m_ligs;
  }
  m_lig += m_ligs;

  ChangeDominantColor();
}

void TentaclesFx::TentaclesImpl::FreshStart()
{
  m_currentDriver->FreshStart();
}

void TentaclesFx::TentaclesImpl::Init()
{
  m_currentDriver->FreshStart();

  if (ProbabilityOfMInN(1, 500))
  {
    m_currentDriver->SetColorMode(TentacleDriver::ColorModes::minimal);
  }
  else if (ProbabilityOfMInN(1, 500))
  {
    m_currentDriver->SetColorMode(TentacleDriver::ColorModes::multiGroups);
  }
  else
  {
    m_currentDriver->SetColorMode(TentacleDriver::ColorModes::oneGroupForAll);
  }
  m_currentDriver->SetReverseColorMix(ProbabilityOfMInN(3, 10));

  m_distt = std::lerp(DISTT_MIN, DISTT_MAX, 0.3);
  m_distt2 = DISTT2_MIN;
  m_distt2Offset = 0;
  m_rot = GetStableRotationOffset(0);

  m_prePrettyMoveLock = 0;
  m_postPrettyMoveLock = 0;
  m_prettyMoveReadyToStart = false;
  if (ProbabilityOfMInN(1, 5))
  {
    m_isPrettyMoveHappening = false;
    m_prettyMoveHappeningTimer = 0;
  }
  else
  {
    m_isPrettyMoveHappening = true;
    PrettyMoveStart(1.0, PRETTY_MOVE_HAPPENING_MIN / 2);
  }
}

void TentaclesFx::TentaclesImpl::Update(PixelBuffer& currentBuff, PixelBuffer& nextBuff)
{
  logDebug("Starting Update.");
  m_stats.UpdateStart();

  IncCounters();

  m_stats.UpdateWithDraw();
  m_lig += m_ligs;

  if (!m_updatingWithDraw)
  {
    m_updatingWithDraw = true;
    Init();
  }

  if (m_lig <= 1.01F)
  {
    m_lig = 1.05F;
    if (m_ligs < 0.0F)
    {
      m_ligs = -m_ligs;
    }

    logDebug("Starting pretty_move without draw.");
    m_stats.UpdateWithPrettyMoveNoDraw();
    PrettyMove(m_goomInfo->GetSoundInfo().GetAcceleration());

    m_cycle += 10.0F * m_cycleInc;
    if (m_cycle > 1000.0F)
    {
      m_stats.CycleReset();
      m_cycle = 0.0;
    }
  }
  else
  {
    if ((m_lig > 10.0F) || (m_lig < 1.1F))
    {
      m_ligs = -m_ligs;
    }

    logDebug("Starting pretty_move and draw.");
    m_stats.UpdateWithPrettyMoveDraw();
    PrettyMove(m_goomInfo->GetSoundInfo().GetAcceleration());
    m_cycle += m_cycleInc;

    if (m_isPrettyMoveHappening || ChangeDominantColorMapEvent())
    {
      // IMPORTANT. Very delicate here - seems the right time to change maps.
      m_stats.ChangeDominantColorMap();
      m_dominantColorMap = m_colorMaps.GetRandomColorMapPtr(RandomColorMaps::ALL);
    }

    if ((m_isPrettyMoveHappening || (m_lig < 6.3F)) && ChangeDominantColorEvent())
    {
      ChangeDominantColor();
      m_countSinceColorChangeLastMarked = 0;
    }

    const auto [modColor, modColorLow] = GetModColors();

    if (m_goomInfo->GetSoundInfo().GetTimeSinceLastGoom() != 0)
    {
      m_stats.LowToMediumAcceleration();
    }
    else
    {
      m_stats.HighAcceleration();
      if (m_countSinceHighAccelLastMarked > 100)
      {
        m_countSinceHighAccelLastMarked = 0;
        if (m_countSinceColorChangeLastMarked > 100)
        {
          ChangeDominantColor();
          m_countSinceColorChangeLastMarked = 0;
        }
      }
    }

    // Higher sound acceleration increases tentacle wave frequency.
    const float tentacleWaveFreq =
        m_goomInfo->GetSoundInfo().GetAcceleration() < 0.3
            ? 1.25F
            : 1.0F / (1.10F - m_goomInfo->GetSoundInfo().GetAcceleration());
    m_currentDriver->MultiplyIterZeroYValWaveFreq(tentacleWaveFreq);

    m_currentDriver->Update(m_half_pi - m_rot, m_distt, m_distt2, modColor, modColorLow,
                            currentBuff, nextBuff);
  }

  m_stats.UpdateEnd();
}

void TentaclesFx::TentaclesImpl::ChangeDominantColor()
{
  m_stats.ChangeDominantColor();
  const Pixel newColor = RandomColorMaps::GetRandomColor(*m_dominantColorMap, 0.0F, 1.0F);
  m_dominantColor = IColorMap::GetColorMix(m_dominantColor, newColor, 0.7);
}

inline auto TentaclesFx::TentaclesImpl::GetModColors() -> std::tuple<Pixel, Pixel>
{
  // IMPORTANT. getEvolvedColor works just right - not sure why
  m_dominantColor = GetEvolvedColor(m_dominantColor);

  const Pixel modColor = GetLightenedColor(m_dominantColor, m_lig * 2.0F + 2.0F);
  const Pixel modColorLow = GetLightenedColor(modColor, (m_lig / 2.0F) + 0.67F);

  return std::make_tuple(modColor, modColorLow);
}

void TentaclesFx::TentaclesImpl::PrettyMovePreStart()
{
  m_prePrettyMoveLock = GetRandInRange(MIN_PRE_PRETTY_MOVE_LOCK, MAX_PRE_PRETTY_MOVE_LOCK);
  m_distt2OffsetPreStep =
      std::lerp(DISTT2_MIN, DISTT2_MAX, 0.2F) / static_cast<float>(m_prePrettyMoveLock);
  m_distt2Offset = 0;
}

void TentaclesFx::TentaclesImpl::PrettyMoveStart(const float acceleration, const int32_t timerVal)
{
  m_stats.PrettyMoveHappens();

  if (timerVal != -1)
  {
    m_prettyMoveHappeningTimer = timerVal;
  }
  else
  {
    m_prettyMoveHappeningTimer =
        static_cast<int>(GetRandInRange(PRETTY_MOVE_HAPPENING_MIN, PRETTY_MOVE_HAPPENING_MAX));
  }
  m_prettyMoveCheckStopMark = m_prettyMoveHappeningTimer / 4;
  m_postPrettyMoveLock = 3 * m_prettyMoveHappeningTimer / 2;

  m_distt2Offset = (1.0F / (1.10F - acceleration)) * GetRandInRange(DISTT2_MIN, DISTT2_MAX);
  m_rotAtStartOfPrettyMove = m_rot;
  m_cycleInc = GetRandInRange(CYCLE_INC_MIN, CYCLE_INC_MAX);
}

/****
inline float getRapport(const float accelvar)
{
  return std::min(1.12f, 1.2f * (1.0f + 2.0f * (accelvar - 1.0f)));
}
****/

void TentaclesFx::TentaclesImpl::PrettyMoveFinish()
{
  m_prettyMoveHappeningTimer = 0;
  m_distt2Offset = 0.0;
}

void TentaclesFx::TentaclesImpl::IsPrettyMoveHappeningUpdate(const float acceleration)
{
  // Are we in a prettyMove?
  if (m_prettyMoveHappeningTimer > 0)
  {
    m_prettyMoveHappeningTimer -= 1;
    return;
  }

  // Not in a pretty move. Have we just finished?
  if (m_isPrettyMoveHappening)
  {
    m_isPrettyMoveHappening = false;
    PrettyMoveFinish();
    return;
  }

  // Are we locked after prettyMove finished?
  if (m_postPrettyMoveLock != 0)
  {
    m_postPrettyMoveLock--;
    return;
  }

  // Are we locked after prettyMove start signal?
  if (m_prePrettyMoveLock != 0)
  {
    m_prePrettyMoveLock--;
    m_distt2Offset += m_distt2OffsetPreStep;
    return;
  }

  // Are we ready to start a prettyMove?
  if (m_prettyMoveReadyToStart)
  {
    m_prettyMoveReadyToStart = false;
    m_isPrettyMoveHappening = true;
    PrettyMoveStart(acceleration);
    return;
  }

  // Are we ready to signal a prettyMove start?
  if (StartPrettyMoveEvent())
  {
    m_prettyMoveReadyToStart = true;
    PrettyMovePreStart();
    return;
  }
}

inline auto TentaclesFx::TentaclesImpl::GetNextDriver() const -> TentacleDriver*
{
  const size_t driverIndex = m_driverWeights.GetRandomWeighted();
  m_stats.ChangeTentacleDriver(driverIndex);
  return m_drivers[driverIndex].get();
}

inline auto TentaclesFx::TentaclesImpl::GetStableRotationOffset(const float cycleVal) -> float
{
  return (1.5F + std::sin(cycleVal) / 32.0F) * m_pi;
}

void TentaclesFx::TentaclesImpl::PrettyMove(const float acceleration)
{
  IsPrettyMoveHappeningUpdate(acceleration);

  if (m_isPrettyMoveHappening && (m_prettyMoveHappeningTimer == m_prettyMoveCheckStopMark))
  {
    m_currentDriver = GetNextDriver();
  }

  m_distt2 = std::lerp(m_distt2, m_distt2Offset, m_prettyMoveLerpMix);

  // Bigger offset here means tentacles start further back behind screen.
  float disttOffset = std::lerp(DISTT_MIN, DISTT_MAX, 0.5F * (1.0F - sin(m_cycle * 19.0 / 20.0)));
  if (m_isPrettyMoveHappening)
  {
    disttOffset *= 0.6F;
  }
  m_distt = std::lerp(m_distt, disttOffset, 4.0F * m_prettyMoveLerpMix);

  float rotOffset = 0.0;
  if (!m_isPrettyMoveHappening)
  {
    rotOffset = GetStableRotationOffset(m_cycle);
  }
  else
  {
    float currentCycle = m_cycle;
    if (ChangeRotationEvent())
    {
      m_doRotation = TurnRotationOnEvent();
    }
    if (m_doRotation)
    {
      currentCycle *= m_two_pi;
    }
    else
    {
      currentCycle *= -m_pi;
    }
    rotOffset = std::fmod(currentCycle, m_two_pi);
    if (rotOffset < 0.0)
    {
      rotOffset += m_two_pi;
    }

    if (m_prettyMoveHappeningTimer < m_prettyMoveCheckStopMark)
    {
      // If close enough to initialStableRotationOffset, then stop the happening.
      if (std::fabs(m_rot - m_rotAtStartOfPrettyMove) < 0.1)
      {
        m_isPrettyMoveHappening = false;
        PrettyMoveFinish();
      }
    }

    if (!(0.0 <= rotOffset && rotOffset <= m_two_pi))
    {
      throw std::logic_error(std20::format(
          "rotOffset {:.3f} not in [0, 2pi]: currentCycle = {:.3f}", rotOffset, currentCycle));
    }
  }

  m_rot = std::clamp(std::lerp(m_rot, rotOffset, m_prettyMoveLerpMix), 0.0F, m_two_pi);

  logDebug("happening = {}, lock = {}, rot = {:.03f}, rotOffset = {:.03f}, "
           "lerpMix = {:.03f}, cycle = {:.03f}, doRotation = {}",
           prettyMoveHappeningTimer, postPrettyMoveLock, rot, rotOffset, prettyMoveLerpMix, cycle,
           doRotation);
}

} // namespace GOOM
