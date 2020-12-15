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
    uint32_t width;
    uint32_t height;
    uint32_t size; // == screen.height * screen.width.
    bool operator==(const Screen&) const = default;
    template<class Archive>
    void serialize(Archive& ar)
    {
      ar(CEREAL_NVP(width), CEREAL_NVP(height), CEREAL_NVP(size));
    };
  };

  PluginInfo() noexcept;
  PluginInfo(uint32_t width, uint32_t height) noexcept;
  PluginInfo(const PluginInfo&) noexcept;
  virtual ~PluginInfo() noexcept = default;

  [[nodiscard]] const Screen& getScreenInfo() const;
  [[nodiscard]] const SoundInfo& getSoundInfo() const;

  bool operator==(const PluginInfo&) const;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(CEREAL_NVP(screen), CEREAL_NVP(soundInfo));
  };

protected:
  virtual void processSoundSample(const AudioSamples&);

private:
  Screen screen;
  std::unique_ptr<SoundInfo> soundInfo;
};

class WritablePluginInfo : public PluginInfo
{
public:
  WritablePluginInfo() noexcept;
  WritablePluginInfo(uint32_t width, uint32_t height) noexcept;

  void processSoundSample(const AudioSamples&) override;
};


inline PluginInfo::PluginInfo() noexcept : screen{0, 0, 0}, soundInfo{nullptr}
{
}

inline PluginInfo::PluginInfo(const uint32_t width, const uint32_t height) noexcept
  : screen{width, height, width * height}, soundInfo{std::make_unique<SoundInfo>()}
{
}

inline PluginInfo::PluginInfo(const PluginInfo& p) noexcept
  : screen{p.screen}, soundInfo{new SoundInfo{*p.soundInfo}}
{
}

inline bool PluginInfo::operator==(const PluginInfo& p) const
{
  return screen == p.screen &&
         ((soundInfo == nullptr && p.soundInfo == nullptr) || (*soundInfo == *p.soundInfo));
}

inline const PluginInfo::Screen& PluginInfo::getScreenInfo() const
{
  return screen;
}

inline const SoundInfo& PluginInfo::getSoundInfo() const
{
  return *soundInfo;
}

inline void PluginInfo::processSoundSample(const AudioSamples& soundData)
{
  soundInfo->processSample(soundData);
}

inline WritablePluginInfo::WritablePluginInfo() noexcept : PluginInfo{}
{
}

inline WritablePluginInfo::WritablePluginInfo(const uint32_t width, const uint32_t height) noexcept
  : PluginInfo{width, height}
{
}

inline void WritablePluginInfo::processSoundSample(const AudioSamples& soundData)
{
  PluginInfo::processSoundSample(soundData);
}

} // namespace goom
#endif
