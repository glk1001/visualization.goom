#ifndef _GOOM_PLUGIN_INFO_H
#define _GOOM_PLUGIN_INFO_H

#include "filters.h"
#include "goom_config.h"
#include "goom_graphic.h"
#include "sound_info.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace goom
{

/**
 * Allows FXs to know the current state of the plugin.
 */
struct PluginInfo
{
  struct Screen
  {
    uint32_t width;
    uint32_t height;
    uint32_t size; // == screen.height * screen.width.
  };

  PluginInfo(const uint32_t width, const uint32_t height) noexcept;

  const Screen screen;

  const SoundInfo& getSoundInfo() const;
  void processSoundSample(const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN]);

private:
  std::unique_ptr<SoundInfo> soundInfo;
};


inline PluginInfo::PluginInfo(const uint32_t width, const uint32_t height) noexcept
  : screen{width, height, width * height}, soundInfo{std::make_unique<SoundInfo>()}
{
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
