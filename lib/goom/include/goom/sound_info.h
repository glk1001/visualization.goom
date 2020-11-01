#ifndef LIBS_GOOM_INCLUDE_GOOM_SOUND_INFO_H_
#define LIBS_GOOM_INCLUDE_GOOM_SOUND_INFO_H_

#include "goom_config.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cstdint>

namespace goom
{

class SoundInfo
{
public:
  SoundInfo() noexcept;
  SoundInfo(const SoundInfo&) noexcept;
  ~SoundInfo() noexcept;

  void processSample(const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN]);

  // Note: a Goom is just a sound event...
  uint32_t getTimeSinceLastGoom() const; // >= 0
  uint32_t getTimeSinceLastBigGoom() const; // >= 0
  uint32_t getTotalGoom() const; // number of goom since last reset (every 'cycleTime')
  float getGoomPower() const; // power of the last Goom [0..1]

  float getVolume() const; // [0..1]
  float getSpeed() const; // speed of the sound [0..100]
  float getAcceleration() const; // acceleration of the sound [0..1]

  int16_t getAllTimesMaxVolume() const;
  int16_t getAllTimesMinVolume() const;

  bool operator==(const SoundInfo&) const;

  template<class Archive>
  void serialize(Archive&);

private:
  uint32_t timeSinceLastGoom = 0;
  uint32_t timeSinceLastBigGoom = 0;
  float goomLimit = 1; // auto-updated limit of goom_detection
  float bigGoomLimit = 1;
  static constexpr float bigGoomSpeedLimit = 10;
  static constexpr float bigGoomFactor = 10;
  float goomPower = 0;
  uint32_t totalGoom = 0;
  static constexpr uint32_t cycleTime = 64;
  uint32_t cycle = 0;

  static constexpr uint32_t bigGoomDuration = 100;
  // static constexpr float BIG_GOOM_SPEED_LIMIT = 0.1;
  static constexpr float accelerationMultiplier = 0.95;
  static constexpr float speedMultiplier = 0.99;

  float volume = 0;
  float acceleration = 0;
  float speed = 0;

  int16_t allTimesMaxVolume;
  int16_t allTimesMinVolume;
  int16_t allTimesPositiveMaxVolume = 1;
  float maxAccelSinceLastReset = 0;
};

template<class Archive>
void SoundInfo::serialize(Archive& ar)
{
  ar(CEREAL_NVP(timeSinceLastGoom), CEREAL_NVP(timeSinceLastBigGoom), CEREAL_NVP(goomLimit),
     CEREAL_NVP(bigGoomLimit), CEREAL_NVP(goomPower), CEREAL_NVP(totalGoom), CEREAL_NVP(cycle),
     CEREAL_NVP(volume), CEREAL_NVP(acceleration), CEREAL_NVP(allTimesMaxVolume),
     CEREAL_NVP(allTimesMinVolume), CEREAL_NVP(allTimesPositiveMaxVolume),
     CEREAL_NVP(maxAccelSinceLastReset));
};

inline uint32_t SoundInfo::getTimeSinceLastGoom() const
{
  return timeSinceLastGoom;
}

inline uint32_t SoundInfo::getTimeSinceLastBigGoom() const
{
  return timeSinceLastBigGoom;
}

inline float SoundInfo::getGoomPower() const
{
  return goomPower;
}

inline float SoundInfo::getSpeed() const
{
  return speed;
}

inline float SoundInfo::getAcceleration() const
{
  return acceleration;
}

inline uint32_t SoundInfo::getTotalGoom() const
{
  return totalGoom;
}

inline float SoundInfo::getVolume() const
{
  return volume;
}

inline int16_t SoundInfo::getAllTimesMaxVolume() const
{
  return allTimesMaxVolume;
}

inline int16_t SoundInfo::getAllTimesMinVolume() const
{
  return allTimesMinVolume;
}

} // namespace goom
#endif /* LIBS_GOOM_INCLUDE_GOOM_SOUND_INFO_H_ */
