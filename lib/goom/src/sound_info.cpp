#include "sound_info.h"

#include "goom_config.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace goom
{

SoundInfo::SoundInfo() noexcept
  : allTimesMaxVolume{std::numeric_limits<int16_t>::min()},
    allTimesMinVolume{std::numeric_limits<int16_t>::max()},
    params{plugin_parameters("Sound", 11)},
    volume_p{secure_f_feedback("Sound Volume")},
    speed_p{secure_f_feedback("Sound Speed")},
    accel_p{secure_f_feedback("Sound Acceleration")},
    goom_limit_p{secure_f_feedback("Goom Limit")},
    goom_power_p{secure_f_feedback("Goom Power")},
    last_goom_p{secure_f_feedback("Goom Detection")},
    last_biggoom_p{secure_f_feedback("Big Goom Detection")},
    biggoom_speed_limit_p{secure_i_param("Big Goom Speed Limit")},
    biggoom_factor_p{secure_i_param("Big Goom Factor")}
{
  IVAL(biggoom_speed_limit_p) = 10;
  IMIN(biggoom_speed_limit_p) = 0;
  IMAX(biggoom_speed_limit_p) = 100;
  ISTEP(biggoom_speed_limit_p) = 1;

  IVAL(biggoom_factor_p) = 10;
  IMIN(biggoom_factor_p) = 0;
  IMAX(biggoom_factor_p) = 100;
  ISTEP(biggoom_factor_p) = 1;

  params.params[0] = &biggoom_speed_limit_p;
  params.params[1] = &biggoom_factor_p;
  params.params[2] = 0;
  params.params[3] = &volume_p;
  params.params[4] = &accel_p;
  params.params[5] = &speed_p;
  params.params[6] = 0;
  params.params[7] = &goom_limit_p;
  params.params[8] = &goom_power_p;
  params.params[9] = &last_goom_p;
  params.params[10] = &last_biggoom_p;
}

SoundInfo::~SoundInfo() noexcept
{
  free(params.params);
}

void SoundInfo::processSample(const int16_t soundData[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN])
{
  // Find the min/max of volumes
  int16_t incVar = 0;
  int16_t maxVar = std::numeric_limits<int16_t>::min();
  int16_t minVar = std::numeric_limits<int16_t>::max();
  for (size_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
  {
    if (incVar < soundData[0][i])
    {
      incVar = soundData[0][i];
    }
    if (maxVar < soundData[0][i])
    {
      maxVar = soundData[0][i];
    }
    if (minVar > soundData[0][i])
    {
      minVar = soundData[0][i];
    }
  }

  if (incVar > allTimesPositiveMaxVolume)
  {
    allTimesPositiveMaxVolume = incVar;
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
  volume = static_cast<float>(incVar) / static_cast<float>(allTimesPositiveMaxVolume);
  memcpy(samples[0], soundData[0], AUDIO_SAMPLE_LEN * sizeof(short));
  memcpy(samples[1], soundData[1], AUDIO_SAMPLE_LEN * sizeof(short));

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
  if ((speed > static_cast<float>(IVAL(biggoom_speed_limit_p)) / 100.0f) &&
      (acceleration > bigGoomLimit) && (timeSinceLastBigGoom > bigGoomDuration))
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
      goomLimit = maxAccelSinceLastReset - 0.02;
    }
    if ((totalGoom == 1) && (goomLimit > 0.02))
    {
      goomLimit -= 0.01;
    }
    totalGoom = 0;
    bigGoomLimit = goomLimit * (1.0f + static_cast<float>(IVAL(biggoom_factor_p)) / 500.0f);
    maxAccelSinceLastReset = 0;
  }

  // Mise a jour des parametres pour la GUI
  FVAL(volume_p) = volume;
  volume_p.change_listener(&volume_p);
  FVAL(speed_p) = speed * 4;
  speed_p.change_listener(&speed_p);
  FVAL(accel_p) = acceleration;
  accel_p.change_listener(&accel_p);

  FVAL(goom_limit_p) = goomLimit;
  goom_limit_p.change_listener(&goom_limit_p);
  FVAL(goom_power_p) = goomPower;
  goom_power_p.change_listener(&goom_power_p);
  FVAL(last_goom_p) = 1.0 - static_cast<float>(timeSinceLastGoom) / 20.0f;
  last_goom_p.change_listener(&last_goom_p);
  FVAL(last_biggoom_p) = 1.0 - static_cast<float>(timeSinceLastBigGoom) / 40.0f;
  last_biggoom_p.change_listener(&last_biggoom_p);

  // bigGoomLimit == goomLimit*9/8+7 ?
}

} // namespace goom
