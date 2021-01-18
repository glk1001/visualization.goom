#ifndef VISUALIZATION_GOOM_GOOM_PLUGIN_INFO_H
#define VISUALIZATION_GOOM_GOOM_PLUGIN_INFO_H

#include "goom_config.h"
#include "sound_info.h"

#include <cereal/archives/json.hpp>
#include <cstdint>
#include <memory>

namespace GOOM
{

class PluginInfo
{
public:
  struct Screen
  {
    uint32_t width;
    uint32_t height;
    uint32_t size; // == screen.height * screen.width.
    auto operator==(const Screen&) const -> bool = default;
    template<class Archive>
    void serialize(Archive& ar)
    {
      ar(CEREAL_NVP(width), CEREAL_NVP(height), CEREAL_NVP(size));
    }
  };

  PluginInfo() noexcept;
  virtual ~PluginInfo() noexcept = default;
  PluginInfo(uint32_t width, uint32_t height) noexcept;
  PluginInfo(const PluginInfo& p) noexcept;
  PluginInfo(const PluginInfo&&) noexcept = delete;
  auto operator=(const PluginInfo&) -> PluginInfo& = delete;
  auto operator=(const PluginInfo&&) -> PluginInfo& = delete;

  [[nodiscard]] auto GetScreenInfo() const -> const Screen&;
  [[nodiscard]] auto GetSoundInfo() const -> const SoundInfo&;

  auto operator==(const PluginInfo& /*p*/) const -> bool;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(CEREAL_NVP(m_screen), CEREAL_NVP(m_soundInfo));
  }

protected:
  virtual void ProcessSoundSample(const AudioSamples& soundData);

private:
  Screen m_screen;
  std::unique_ptr<SoundInfo> m_soundInfo;
};

class WritablePluginInfo : public PluginInfo
{
public:
  WritablePluginInfo() noexcept;
  WritablePluginInfo(uint32_t width, uint32_t height) noexcept;

  void ProcessSoundSample(const AudioSamples& soundData) override;
};


inline PluginInfo::PluginInfo() noexcept : m_screen{0, 0, 0}, m_soundInfo{nullptr}
{
}

inline PluginInfo::PluginInfo(const uint32_t width, const uint32_t height) noexcept
  : m_screen{width, height, width * height}, m_soundInfo{std::make_unique<SoundInfo>()}
{
}

inline PluginInfo::PluginInfo(const PluginInfo& p) noexcept
  : m_screen{p.m_screen}, m_soundInfo{new SoundInfo{*p.m_soundInfo}}
{
}

inline auto PluginInfo::operator==(const PluginInfo& p) const -> bool
{
  return m_screen == p.m_screen &&
         ((m_soundInfo == nullptr && p.m_soundInfo == nullptr) || (*m_soundInfo == *p.m_soundInfo));
}

inline auto PluginInfo::GetScreenInfo() const -> const PluginInfo::Screen&
{
  return m_screen;
}

inline auto PluginInfo::GetSoundInfo() const -> const SoundInfo&
{
  return *m_soundInfo;
}

inline void PluginInfo::ProcessSoundSample(const AudioSamples& soundData)
{
  m_soundInfo->ProcessSample(soundData);
}

inline WritablePluginInfo::WritablePluginInfo() noexcept : PluginInfo{}
{
}

inline WritablePluginInfo::WritablePluginInfo(const uint32_t width, const uint32_t height) noexcept
  : PluginInfo{width, height}
{
}

inline void WritablePluginInfo::ProcessSoundSample(const AudioSamples& soundData)
{
  PluginInfo::ProcessSoundSample(soundData);
}

} // namespace GOOM
#endif
