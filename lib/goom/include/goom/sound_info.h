#ifndef LIBS_GOOM_INCLUDE_GOOM_SOUND_INFO_H_
#define LIBS_GOOM_INCLUDE_GOOM_SOUND_INFO_H_

#include "goom_config.h"
#include "goom_config_param.h"

#include <cstdint>

namespace goom
{

class SoundInfo
{
public:
  SoundInfo() noexcept;
  SoundInfo(const SoundInfo&) = delete;
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

  PluginParameters& getParams();

private:
  uint32_t timeSinceLastGoom = 0;
  uint32_t timeSinceLastBigGoom = 0;
  float goomLimit = 1; // auto-updated limit of goom_detection
  float bigGoomLimit = 1;
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

  int16_t samples[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN];

  PluginParameters params; // contains the previously defined parameters.
  PluginParam volume_p;
  PluginParam speed_p;
  PluginParam accel_p;
  PluginParam goom_limit_p;
  PluginParam goom_power_p;
  PluginParam last_goom_p;
  PluginParam last_biggoom_p;
  PluginParam biggoom_speed_limit_p;
  PluginParam biggoom_factor_p;
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

inline PluginParameters& SoundInfo::getParams()
{
  return params;
}

} // namespace goom
#endif /* LIBS_GOOM_INCLUDE_GOOM_SOUND_INFO_H_ */
