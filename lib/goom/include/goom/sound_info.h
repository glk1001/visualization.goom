#ifndef VISUALIZATION_GOOM_SOUND_INFO_H
#define VISUALIZATION_GOOM_SOUND_INFO_H

#include "goom_config.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cstdint>
#include <vector>

namespace goom
{

class AudioSamples
{
public:
  static constexpr size_t NUM_CHANNELS = 2;

  // AudioSample object: numSampleChannels = 1 or 2.
  //   If numSampleChannels = 1, then the first  AUDIO_SAMPLE_LEN values of
  //   'floatAudioData' are used to for two channels
  //   If numSampleChannels = 2, then the 'floatAudioData' must interleave the two channels
  //   one after the other. So 'floatAudioData[0]' is channel 0, 'floatAudioData[1]'
  //   is channel 1, 'floatAudioData[2]' is channel 0, 'floatAudioData[3]' is channel 1, etc.
  AudioSamples(size_t numSampleChannels,
               const float floatAudioData[NUM_AUDIO_SAMPLES * AUDIO_SAMPLE_LEN]);

  [[nodiscard]] auto GetNumDistinctChannels() const -> size_t { return m_numDistinctChannels; }
  [[nodiscard]] auto GetSample(size_t channelIndex) const -> const std::vector<int16_t>&;
  auto GetSample(size_t channelIndex) -> std::vector<int16_t>&;

private:
  const size_t m_numDistinctChannels;
  using SampleArray = std::vector<int16_t>;
  std::vector<SampleArray> m_sampleArrays;
};

class SoundInfo
{
public:
  SoundInfo() noexcept;
  ~SoundInfo() noexcept;
  SoundInfo(const SoundInfo& s) noexcept;
  SoundInfo(const SoundInfo&&) noexcept = delete;
  auto operator=(const SoundInfo&) -> SoundInfo& = delete;
  auto operator=(const SoundInfo&&) -> SoundInfo& = delete;

  void ProcessSample(const AudioSamples& s);

  // Note: a Goom is just a sound event...
  [[nodiscard]] auto GetTimeSinceLastGoom() const -> uint32_t; // >= 0
  [[nodiscard]] auto GetTimeSinceLastBigGoom() const -> uint32_t; // >= 0
  [[nodiscard]] auto GetTotalGoom() const
      -> uint32_t; // number of goom since last reset (every 'cycleTime')
  [[nodiscard]] auto GetGoomPower() const -> float; // power of the last Goom [0..1]

  [[nodiscard]] auto GetVolume() const -> float; // [0..1]
  [[nodiscard]] auto GetSpeed() const -> float; // speed of the sound [0..100]
  [[nodiscard]] auto GetAcceleration() const -> float; // acceleration of the sound [0..1]

  [[nodiscard]] auto GetAllTimesMaxVolume() const -> int16_t;
  [[nodiscard]] auto GetAllTimesMinVolume() const -> int16_t;

  auto operator==(const SoundInfo& s) const -> bool;

  template<class Archive>
  void serialize(Archive&);

private:
  uint32_t m_timeSinceLastGoom = 0;
  uint32_t m_timeSinceLastBigGoom = 0;
  float m_goomLimit = 1.0F; // auto-updated limit of goom_detection
  float m_bigGoomLimit = 1.0F;
  static constexpr float BIG_GOOM_SPEED_LIMIT = 10.0F;
  static constexpr float BIG_GOOM_FACTOR = 10.0F;
  float m_goomPower = 0.0F;
  uint32_t m_totalGoom = 0;
  static constexpr uint32_t CYCLE_TIME = 64;
  uint32_t m_cycle = 0;

  static constexpr uint32_t BIG_GOOM_DURATION = 100;
  // static constexpr float BIG_GOOM_SPEED_LIMIT = 0.1;
  static constexpr float ACCELERATION_MULTIPLIER = 0.95;
  static constexpr float SPEED_MULTIPLIER = 0.99;

  float m_volume = 0.0F;
  float m_acceleration = 0.0F;
  float m_speed = 0.0F;

  int16_t m_allTimesMaxVolume{};
  int16_t m_allTimesMinVolume{};
  int16_t m_allTimesPositiveMaxVolume = 1;
  float m_maxAccelSinceLastReset = 0.0F;
};

template<class Archive>
void SoundInfo::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_timeSinceLastGoom), CEREAL_NVP(m_timeSinceLastBigGoom), CEREAL_NVP(m_goomLimit),
     CEREAL_NVP(m_bigGoomLimit), CEREAL_NVP(m_goomPower), CEREAL_NVP(m_totalGoom),
     CEREAL_NVP(m_cycle), CEREAL_NVP(m_volume), CEREAL_NVP(m_acceleration),
     CEREAL_NVP(m_allTimesMaxVolume), CEREAL_NVP(m_allTimesMinVolume),
     CEREAL_NVP(m_allTimesPositiveMaxVolume), CEREAL_NVP(m_maxAccelSinceLastReset));
}

inline auto SoundInfo::GetTimeSinceLastGoom() const -> uint32_t
{
  return m_timeSinceLastGoom;
}

inline auto SoundInfo::GetTimeSinceLastBigGoom() const -> uint32_t
{
  return m_timeSinceLastBigGoom;
}

inline auto SoundInfo::GetGoomPower() const -> float
{
  return m_goomPower;
}

inline auto SoundInfo::GetSpeed() const -> float
{
  return m_speed;
}

inline auto SoundInfo::GetAcceleration() const -> float
{
  return m_acceleration;
}

inline auto SoundInfo::GetTotalGoom() const -> uint32_t
{
  return m_totalGoom;
}

inline auto SoundInfo::GetVolume() const -> float
{
  return m_volume;
}

inline auto SoundInfo::GetAllTimesMaxVolume() const -> int16_t
{
  return m_allTimesMaxVolume;
}

inline auto SoundInfo::GetAllTimesMinVolume() const -> int16_t
{
  return m_allTimesMinVolume;
}

} // namespace goom
#endif
