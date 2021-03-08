#include "sound_info.h"

#include "goom_config.h"
#include "goomutils/mathutils.h"
#include "stats/sound_stats.h"

#undef NDEBUG
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>
#include <vector>

namespace GOOM
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
  : m_stats{std::make_shared<SoundStats>()},
    m_allTimesMaxVolume{std::numeric_limits<int16_t>::min()},
    m_allTimesMinVolume{std::numeric_limits<int16_t>::max()}
{
}

SoundInfo::SoundInfo(const SoundInfo& s) noexcept = default;

SoundInfo::~SoundInfo() noexcept = default;

void SoundInfo::ProcessSample(const AudioSamples& samples)
{
  m_updateNum++;

  UpdateVolume(samples);
  assert(0.0 <= m_volume && m_volume <= 1.0);

  const float prevAcceleration = m_acceleration;
  UpdateAcceleration();
  assert(0.0 <= m_acceleration && m_acceleration <= 1.0);

  UpdateSpeed(prevAcceleration);
  assert(0.0 <= m_speed && m_speed <= 1.0);

  // Detection des nouveaux gooms
  // Detection of new gooms
  UpdateLastBigGoom();
  UpdateLastGoom();
}

void SoundInfo::UpdateVolume(const AudioSamples& samples)
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

  m_stats->DoUpdateVolume(m_volume);
}

void SoundInfo::UpdateAcceleration()
{
  // Transformations sur la vitesse du son
  // Speed of sound transformations
  m_acceleration = m_volume;

  if (m_speed < 0.1F)
  {
    m_acceleration *= (1.0F - static_cast<float>(m_speed));
  }
  else if (m_speed < 0.3F)
  {
    m_acceleration *= (0.9F - 0.5F * static_cast<float>(m_speed - 0.1F));
  }
  else
  {
    m_acceleration *= (0.8F - 0.25F * static_cast<float>(m_speed - 0.3F));
  }

  // Adoucissement de l'acceleration
  // Smooth acceleration
  if (m_acceleration < 0.0F)
  {
    m_acceleration = 0.0F;
  }
  else
  {
    m_acceleration *= ACCELERATION_MULTIPLIER;
  }

  m_stats->DoUpdateAcceleration(m_acceleration);
}

void SoundInfo::UpdateSpeed(const float prevAcceleration)
{
  // Mise a jour de la vitesse
  // Speed update
  float diffAcceleration = m_acceleration - prevAcceleration;
  if (diffAcceleration < 0.0F)
  {
    diffAcceleration = -diffAcceleration;
  }

  const float newSpeed = SPEED_MULTIPLIER * (0.5F * m_speed + 0.25F * diffAcceleration);
  m_speed = stdnew::lerp(newSpeed, m_speed, 0.75F);
  if (m_speed < 0.0F)
  {
    m_speed = 0.0F;
  }
  else if (m_speed > 1.0F)
  {
    m_speed = 1.0F;
  }

  m_stats->DoUpdateSpeed(m_speed);
}

void SoundInfo::UpdateLastGoom()
{
  // Temps du goom
  // Goom time
  m_timeSinceLastGoom++;

  if (m_acceleration > m_goomLimit)
  {
    m_timeSinceLastGoom = 0;
    m_stats->DoGoom();

    // TODO: tester && (info->m_timeSinceLastGoom > 20)) {
    m_totalGoomsInCurrentCycle++;
    m_goomPower = m_acceleration - m_goomLimit;
  }
  if (m_acceleration > m_maxAccelSinceLastReset)
  {
    m_maxAccelSinceLastReset = m_acceleration;
  }

  // Toute les 2 secondes: vérifier si le taux de goom est correct et le modifier sinon.
  // Every 2 seconds: check if the goom rate is correct and modify it otherwise.
  if (m_updateNum % CYCLE_TIME == 0)
  {
    UpdateGoomLimit();
    assert(0.0 <= m_goomLimit && m_goomLimit <= 1.0);

    m_totalGoomsInCurrentCycle = 0;
    m_maxAccelSinceLastReset = 0.0F;
    m_bigGoomLimit = m_goomLimit * BIG_GOOM_FACTOR;
  }

  // m_bigGoomLimit == m_goomLimit*9/8+7 ?
}

void SoundInfo::UpdateLastBigGoom()
{
  m_timeSinceLastBigGoom++;

  if ((m_speed > BIG_GOOM_SPEED_LIMIT) && (m_acceleration > m_bigGoomLimit) &&
      (m_timeSinceLastBigGoom > MAX_BIG_GOOM_DURATION))
  {
    m_timeSinceLastBigGoom = 0;
    m_stats->DoBigGoom();
  }
}

void SoundInfo::UpdateGoomLimit()
{
  if (m_speed < 0.01F)
  {
    m_goomLimit *= 0.91;
  }

  if (m_totalGoomsInCurrentCycle > 4)
  {
    m_goomLimit += 0.02;
  }
  else if (m_totalGoomsInCurrentCycle > 7)
  {
    m_goomLimit *= 1.03F;
    m_goomLimit += 0.03F;
  }
  else if (m_totalGoomsInCurrentCycle > 16)
  {
    m_goomLimit *= 1.05F;
    m_goomLimit += 0.04F;
  }
  else if (m_totalGoomsInCurrentCycle == 0)
  {
    m_goomLimit = m_maxAccelSinceLastReset - 0.02F;
  }

  if ((m_totalGoomsInCurrentCycle == 1) && (m_goomLimit > 0.02F))
  {
    m_goomLimit -= 0.01F;
  }

  if (m_goomLimit < 0.0F)
  {
    m_goomLimit = 0.0F;
  }
  else if (m_goomLimit > 1.0F)
  {
    m_goomLimit = 1.0F;
  }
}

void SoundInfo::Log(const StatsLogValueFunc& l) const
{
  m_stats->Log(l);
}

} // namespace GOOM
