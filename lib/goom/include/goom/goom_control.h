#ifndef VISUALIZATION_GOOM_GOOM_CONTROL_H
#define VISUALIZATION_GOOM_GOOM_CONTROL_H

#include "goom_config.h"

#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>
#include <string>

namespace GOOM
{

class AudioSamples;
class PixelBuffer;

class GoomControl
{
public:
  static auto GetRandSeed() -> uint64_t;
  static void SetRandSeed(uint64_t seed);

  GoomControl() noexcept = delete;
  GoomControl(uint32_t resx, uint32_t resy) noexcept;
  ~GoomControl() noexcept;
  GoomControl(const GoomControl&) noexcept = delete;
  GoomControl(GoomControl&&) noexcept = delete;
  auto operator=(const GoomControl&) -> GoomControl& = delete;
  auto operator=(GoomControl&&) -> GoomControl& = delete;

  void SaveState(std::ostream& s) const;
  void RestoreState(std::istream& s);

  [[nodiscard]] auto GetResourcesDirectory() const -> const std::string&;
  void SetResourcesDirectory(const std::string& dirName);

  void SetScreenBuffer(PixelBuffer& buff);
  void Start();

  /*
   * Update the next goom frame
   *
   * forceMode == 0 : do nothing
   * forceMode == -1 : lock the FX
   * forceMode == 1..NB_FX : force a switch to FX n# forceMode
   *
   * songTitle = pointer to the title of the song...
   *      - NULL if it is not the start of the song
   *      - only have a value at the start of the song
   */
  void Update(
      const AudioSamples& s, int forceMode, float fps, const char* songTitle, const char* message);

  void Finish();

private:
  class GoomControlImpl;
  const std::unique_ptr<GoomControlImpl> m_controller;
};

} // namespace GOOM
#endif
