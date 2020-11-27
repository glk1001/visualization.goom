#include "sound_info.h"

#include "goom_config.h"

#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>
#include <vector>

namespace goom
{

inline int16_t floatToInt16(const float f)
{
  if (f >= 1.0f)
  {
    return std::numeric_limits<int16_t>::max();
  }
  else if (f < -1.0f)
  {
    return -std::numeric_limits<int16_t>::max();
  }
  else
  {
    return static_cast<int16_t>(f * static_cast<float>(std::numeric_limits<int16_t>::max()));
  }
}

AudioSamples::AudioSamples(const size_t numSampleChannels,
                           const float floatAudioData[NUM_AUDIO_SAMPLES * AUDIO_SAMPLE_LEN])
  : numDistinctChannels{numSampleChannels}, sampleArrays(numChannels)
{
  if (numSampleChannels == 0 || numSampleChannels > 2)
  {
    throw std::logic_error(
        std20::format("Invalid 'numSampleChannels == {}. Must be '1' or '2'.", numSampleChannels));
  }

  sampleArrays[0].resize(AUDIO_SAMPLE_LEN);
  sampleArrays[1].resize(AUDIO_SAMPLE_LEN);

  if (numChannels == 1)
  {
    for (size_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
    {
      sampleArrays[0][i] = floatToInt16(floatAudioData[i]);
      sampleArrays[1][i] = sampleArrays[0][i];
    }
  }
  else
  {
    int fpos = 0;
    for (size_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
    {
      sampleArrays[0][i] = floatToInt16(floatAudioData[fpos]);
      fpos++;
      sampleArrays[1][i] = floatToInt16(floatAudioData[fpos]);
      fpos++;
    }
  }
}

const std::vector<int16_t>& AudioSamples::getSample(const size_t channelIndex) const
{
  return sampleArrays.at(channelIndex);
}

std::vector<int16_t>& AudioSamples::getSample(const size_t channelIndex)
{
  return sampleArrays.at(channelIndex);
}

SoundInfo::SoundInfo() noexcept
  : allTimesMaxVolume{std::numeric_limits<int16_t>::min()},
    allTimesMinVolume{std::numeric_limits<int16_t>::max()}
{
}

SoundInfo::SoundInfo(const SoundInfo& s) noexcept
  : timeSinceLastGoom{s.timeSinceLastGoom},
    timeSinceLastBigGoom{s.timeSinceLastBigGoom},
    goomLimit{s.goomLimit},
    bigGoomLimit{s.bigGoomLimit},
    goomPower{s.goomPower},
    totalGoom{s.totalGoom},
    cycle{s.cycle},
    volume{s.volume},
    acceleration{s.acceleration},
    speed{s.speed},
    allTimesMaxVolume{s.allTimesMaxVolume},
    allTimesMinVolume{s.allTimesMinVolume},
    allTimesPositiveMaxVolume{s.allTimesPositiveMaxVolume},
    maxAccelSinceLastReset{s.maxAccelSinceLastReset}
{
}

SoundInfo::~SoundInfo() noexcept
{
}

bool SoundInfo::operator==(const SoundInfo& s) const
{
  return timeSinceLastGoom == s.timeSinceLastGoom &&
         timeSinceLastBigGoom == s.timeSinceLastBigGoom && goomLimit == s.goomLimit &&
         bigGoomLimit == s.bigGoomLimit && goomPower == s.goomPower && totalGoom == s.totalGoom &&
         cycle == s.cycle && volume == s.volume && acceleration == s.acceleration &&
         allTimesMaxVolume == s.allTimesMaxVolume && allTimesMinVolume == s.allTimesMinVolume &&
         allTimesPositiveMaxVolume == s.allTimesPositiveMaxVolume &&
         maxAccelSinceLastReset == s.maxAccelSinceLastReset;
}

void SoundInfo::processSample(const AudioSamples& samples)
{
  // Find the min/max of volumes
  int16_t maxPosVar = 0;
  int16_t maxVar = std::numeric_limits<int16_t>::min();
  int16_t minVar = std::numeric_limits<int16_t>::max();
  for (size_t n = 0; n < AudioSamples::numChannels; n++)
  {
    const std::vector<int16_t>& soundData = samples.getSample(n);
    for (size_t i = 0; i < soundData.size(); i++)
    {
      if (maxPosVar < soundData[i])
      {
        maxPosVar = soundData[i];
      }
      if (maxVar < soundData[i])
      {
        maxVar = soundData[i];
      }
      if (minVar > soundData[i])
      {
        minVar = soundData[i];
      }
    }
  }

  if (maxPosVar > allTimesPositiveMaxVolume)
  {
    allTimesPositiveMaxVolume = maxPosVar;
  }
  if (maxVar > allTimesMaxVolume)
  {
    allTimesMaxVolume = maxVar;
  }
  if (minVar < allTimesMinVolume)
  {
    allTimesMinVolume = minVar;
  }

  // Volume sonore - TODO: why only positive volumes?
  volume = static_cast<float>(maxPosVar) / static_cast<float>(allTimesPositiveMaxVolume);

  float difaccel = acceleration;
  acceleration = volume; // accel entre 0 et 1

  // Transformations sur la vitesse du son
  if (speed > 1.0f)
  {
    speed = 1.0f;
  }
  if (speed < 0.1f)
  {
    acceleration *= (1.0f - static_cast<float>(speed));
  }
  else if (speed < 0.3f)
  {
    acceleration *= (0.9f - static_cast<float>(speed - 0.1f) / 2.0f);
  }
  else
  {
    acceleration *= (0.8f - static_cast<float>(speed - 0.3f) / 4.0f);
  }

  // Adoucissement de l'acceleration
  acceleration *= accelerationMultiplier;
  if (acceleration < 0)
  {
    acceleration = 0;
  }

  // Mise a jour de la vitesse
  difaccel = acceleration - difaccel;
  if (difaccel < 0)
  {
    difaccel = -difaccel;
  }
  const float prevspeed = speed;
  speed = (speed + difaccel * 0.5f) / 2;
  speed *= speedMultiplier;
  speed = (speed + 3.0f * prevspeed) / 4.0f;
  if (speed < 0)
  {
    speed = 0;
  }
  if (speed > 1)
  {
    speed = 1;
  }

  // Temps du goom
  timeSinceLastGoom++;
  timeSinceLastBigGoom++;
  cycle++;

  // Detection des nouveaux gooms
  if ((speed > bigGoomSpeedLimit / 100.0f) && (acceleration > bigGoomLimit) &&
      (timeSinceLastBigGoom > bigGoomDuration))
  {
    timeSinceLastBigGoom = 0;
  }

  if (acceleration > goomLimit)
  {
    // TODO: tester && (info->timeSinceLastGoom > 20)) {
    totalGoom++;
    timeSinceLastGoom = 0;
    goomPower = acceleration - goomLimit;
  }

  if (acceleration > maxAccelSinceLastReset)
  {
    maxAccelSinceLastReset = acceleration;
  }

  if (goomLimit > 1)
  {
    goomLimit = 1;
  }

  // Toute les 2 secondes : vérifier si le taux de goom est correct et le modifier sinon..
  if (cycle % cycleTime == 0)
  {
    if (speed < 0.01f)
    {
      goomLimit *= 0.91;
    }
    if (totalGoom > 4)
    {
      goomLimit += 0.02;
    }
    if (totalGoom > 7)
    {
      goomLimit *= 1.03f;
      goomLimit += 0.03;
    }
    if (totalGoom > 16)
    {
      goomLimit *= 1.05f;
      goomLimit += 0.04;
    }
    if (totalGoom == 0)
    {
      goomLimit = maxAccelSinceLastReset - 0.02F;
    }
    if ((totalGoom == 1) && (goomLimit > 0.02))
    {
      goomLimit -= 0.01;
    }
    totalGoom = 0;
    bigGoomLimit = goomLimit * (1.0f + bigGoomFactor / 500.0f);
    maxAccelSinceLastReset = 0;
  }

  // bigGoomLimit == goomLimit*9/8+7 ?
}

} // namespace goom
