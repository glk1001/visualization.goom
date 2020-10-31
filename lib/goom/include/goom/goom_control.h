#ifndef _GOOM_CONTROL_H
#define _GOOM_CONTROL_H

#include "goom_config.h"
#include "goom_plugin_info.h"

#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>

namespace goom
{

class GoomControl
{
public:
  GoomControl(const uint16_t resx, const uint16_t resy, const int seed);
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
  void update(const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN],
              const int forceMode,
              const float fps,
              const char* songTitle,
              const char* message);

  void finish();

private:
  class GoomControlImp;
  std::unique_ptr<GoomControlImp> controller;
};

} // namespace goom
#endif
