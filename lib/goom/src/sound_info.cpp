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

inline auto FloatToInt16(const float f) -> int16_t
{
  if (f >= 1.0F)
  {
    return std::numeric_limits<int16_t>::max();
  }

  if (f < -1.0F)
  {
    return -std::numeric_limits<int16_t>::max();
  }

  return static_cast<int16_t>(f * static_cast<float>(std::numeric_limits<int16_t>::max()));
}

AudioSamples::AudioSamples(const size_t numSampleChannels,
                           const float floatAudioData[NUM_AUDIO_SAMPLES * AUDIO_SAMPLE_LEN])
  : m_numDistinctChannels{numSampleChannels}, m_sampleArrays(NUM_CHANNELS)
{
  if (numSampleChannels == 0 || numSampleChannels > 2)
  {
    throw std::logic_error(
        std20::format("Invalid 'numSampleChannels == {}. Must be '1' or '2'.", numSampleChannels));
  }

  m_sampleArrays[0].resize(AUDIO_SAMPLE_LEN);
  m_sampleArrays[1].resize(AUDIO_SAMPLE_LEN);

  if (NUM_CHANNELS == 1)
  {
    for (size_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
    {
      m_sampleArrays[0][i] = FloatToInt16(floatAudioData[i]);
      m_sampleArrays[1][i] = m_sampleArrays[0][i];
    }
  }
  else
  {
    int fpos = 0;
    for (size_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
    {
      m_sampleArrays[0][i] = FloatToInt16(floatAudioData[fpos]);
      fpos++;
      m_sampleArrays[1][i] = FloatToInt16(floatAudioData[fpos]);
      fpos++;
    }
  }
}

auto AudioSamples::GetSample(const size_t channelIndex) const -> const std::vector<int16_t>&
{
  return m_sampleArrays.at(channelIndex);
}

auto AudioSamples::GetSample(const size_t channelIndex) -> std::vector<int16_t>&
{
  return m_sampleArrays.at(channelIndex);
}

SoundInfo::SoundInfo() noexcept
  : m_allTimesMaxVolume{std::numeric_limits<int16_t>::min()},
    m_allTimesMinVolume{std::numeric_limits<int16_t>::max()}
{
}

SoundInfo::SoundInfo(const SoundInfo& s) noexcept = default;

SoundInfo::~SoundInfo() noexcept = default;

auto SoundInfo::operator==(const SoundInfo& s) const -> bool
{
  return m_timeSinceLastGoom == s.m_timeSinceLastGoom &&
         m_timeSinceLastBigGoom == s.m_timeSinceLastBigGoom && m_goomLimit == s.m_goomLimit &&
         m_bigGoomLimit == s.m_bigGoomLimit && m_goomPower == s.m_goomPower &&
         m_totalGoom == s.m_totalGoom && m_cycle == s.m_cycle && m_volume == s.m_volume &&
         m_acceleration == s.m_acceleration && m_allTimesMaxVolume == s.m_allTimesMaxVolume &&
         m_allTimesMinVolume == s.m_allTimesMinVolume &&
         m_allTimesPositiveMaxVolume == s.m_allTimesPositiveMaxVolume &&
         m_maxAccelSinceLastReset == s.m_maxAccelSinceLastReset;
}

void SoundInfo::ProcessSample(const AudioSamples& samples)
{
  // Find the min/max of volumes
  int16_t maxPosVar = 0;
  int16_t maxVar = std::numeric_limits<int16_t>::min();
  int16_t minVar = std::numeric_limits<int16_t>::max();
  for (size_t n = 0; n < AudioSamples::NUM_CHANNELS; n++)
  {
    const std::vector<int16_t>& soundData = samples.GetSample(n);
    for (int16_t dataVal : soundData)
    {
      if (maxPosVar < dataVal)
      {
        maxPosVar = dataVal;
      }
      if (maxVar < dataVal)
      {
        maxVar = dataVal;
      }
      if (minVar > dataVal)
      {
        minVar = dataVal;
      }
    }
  }

  if (maxPosVar > m_allTimesPositiveMaxVolume)
  {
    m_allTimesPositiveMaxVolume = maxPosVar;
  }
  if (maxVar > m_allTimesMaxVolume)
  {
    m_allTimesMaxVolume = maxVar;
  }
  if (minVar < m_allTimesMinVolume)
  {
    m_allTimesMinVolume = minVar;
  }

  // Volume sonore - TODO: why only positive volumes?
  m_volume = static_cast<float>(maxPosVar) / static_cast<float>(m_allTimesPositiveMaxVolume);

  float difaccel = m_acceleration;
  m_acceleration = m_volume; // accel entre 0 et 1

  // Transformations sur la vitesse du son
  if (m_speed > 1.0F)
  {
    m_speed = 1.0F;
  }
  if (m_speed < 0.1F)
  {
    m_acceleration *= (1.0F - static_cast<float>(m_speed));
  }
  else if (m_speed < 0.3F)
  {
    m_acceleration *= (0.9F - static_cast<float>(m_speed - 0.1F) / 2.0F);
  }
  else
  {
    m_acceleration *= (0.8F - static_cast<float>(m_speed - 0.3F) / 4.0F);
  }

  // Adoucissement de l'acceleration
  m_acceleration *= ACCELERATION_MULTIPLIER;
  if (m_acceleration < 0.0F)
  {
    m_acceleration = 0.0F;
  }

  // Mise a jour de la vitesse
  difaccel = m_acceleration - difaccel;
  if (difaccel < 0.0F)
  {
    difaccel = -difaccel;
  }
  const float prevspeed = m_speed;
  m_speed = (m_speed + difaccel * 0.5F) / 2.0F;
  m_speed *= SPEED_MULTIPLIER;
  m_speed = (m_speed + 3.0F * prevspeed) / 4.0F;
  if (m_speed < 0.0F)
  {
    m_speed = 0.0F;
  }
  if (m_speed > 1.0F)
  {
    m_speed = 1.0F;
  }

  // Temps du goom
  m_timeSinceLastGoom++;
  m_timeSinceLastBigGoom++;
  m_cycle++;

  // Detection des nouveaux gooms
  if ((m_speed > BIG_GOOM_SPEED_LIMIT / 100.0F) && (m_acceleration > m_bigGoomLimit) &&
      (m_timeSinceLastBigGoom > BIG_GOOM_DURATION))
  {
    m_timeSinceLastBigGoom = 0;
  }

  if (m_acceleration > m_goomLimit)
  {
    // TODO: tester && (info->m_timeSinceLastGoom > 20)) {
    m_totalGoom++;
    m_timeSinceLastGoom = 0;
    m_goomPower = m_acceleration - m_goomLimit;
  }

  if (m_acceleration > m_maxAccelSinceLastReset)
  {
    m_maxAccelSinceLastReset = m_acceleration;
  }

  if (m_goomLimit > 1.0F)
  {
    m_goomLimit = 1.0F;
  }

  // Toute les 2 secondes : vérifier si le taux de goom est correct et le modifier sinon..
  if (m_cycle % CYCLE_TIME == 0)
  {
    if (m_speed < 0.01F)
    {
      m_goomLimit *= 0.91;
    }
    if (m_totalGoom > 4)
    {
      m_goomLimit += 0.02;
    }
    if (m_totalGoom > 7)
    {
      m_goomLimit *= 1.03F;
      m_goomLimit += 0.03F;
    }
    if (m_totalGoom > 16)
    {
      m_goomLimit *= 1.05F;
      m_goomLimit += 0.04F;
    }
    if (m_totalGoom == 0)
    {
      m_goomLimit = m_maxAccelSinceLastReset - 0.02F;
    }
    if ((m_totalGoom == 1) && (m_goomLimit > 0.02F))
    {
      m_goomLimit -= 0.01F;
    }
    m_totalGoom = 0;
    m_bigGoomLimit = m_goomLimit * (1.0F + BIG_GOOM_FACTOR / 500.0F);
    m_maxAccelSinceLastReset = 0.0F;
  }

  // m_bigGoomLimit == m_goomLimit*9/8+7 ?
}

} // namespace goom
