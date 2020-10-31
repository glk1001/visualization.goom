#ifndef _GOOM_PLUGIN_INFO_H
#define _GOOM_PLUGIN_INFO_H

#include "goom_config.h"
#include "sound_info.h"

#include <cereal/archives/json.hpp>
#include <cstdint>
#include <memory>

namespace goom
{

class PluginInfo
{
public:
  struct Screen
  {
    uint16_t width;
    uint16_t height;
    uint32_t size; // == screen.height * screen.width.
    template<class Archive>
    void serialize(Archive& ar)
    {
      ar(CEREAL_NVP(width), CEREAL_NVP(height), CEREAL_NVP(size));
    };
  };

  PluginInfo(const uint16_t width, const uint16_t height) noexcept;

  const Screen getScreenInfo() const;
  const SoundInfo& getSoundInfo() const;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(CEREAL_NVP(screen));
  };

protected:
  void processSoundSample(const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN]);

private:
  const Screen screen;
  std::unique_ptr<SoundInfo> soundInfo;
};


inline PluginInfo::PluginInfo(const uint16_t width, const uint16_t height) noexcept
  : screen{width, height, static_cast<uint32_t>(width) * static_cast<uint32_t>(height)},
    soundInfo{std::make_unique<SoundInfo>()}
{
}

inline const PluginInfo::Screen PluginInfo::getScreenInfo() const
{
  return screen;
}

inline const SoundInfo& PluginInfo::getSoundInfo() const
{
  return *soundInfo;
}

inline void PluginInfo::processSoundSample(const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN])
{
  soundInfo->processSample(data);
}

} // namespace goom
#endif
