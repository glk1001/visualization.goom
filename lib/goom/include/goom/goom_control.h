#ifndef _GOOM_CONTROL_H
#define _GOOM_CONTROL_H

#include "goom_config.h"

#include <cereal/access.hpp>
#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>

namespace goom
{

class AudioSamples;

class GoomControl
{
public:
  static uint64_t getRandSeed();
  static void setRandSeed(const uint64_t seed);

  GoomControl() noexcept;
  GoomControl(const uint16_t resx, const uint16_t resy) noexcept;
  ~GoomControl() noexcept;

  uint16_t getScreenWidth() const;
  uint16_t getScreenHeight() const;

  void saveState(std::ostream&) const;
  void restoreState(std::istream&);

  void setScreenBuffer(uint32_t* buffer);
  void start();

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
  void update(const AudioSamples&,
              const int forceMode,
              const float fps,
              const char* songTitle,
              const char* message);

  void finish();

  bool operator==(const GoomControl&) const;

private:
  class GoomControlImpl;
  std::unique_ptr<GoomControlImpl> controller;

  friend class cereal::access;
  template<class Archive>
  void serialize(Archive&);
};

} // namespace goom
#endif
