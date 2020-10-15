#ifndef LIBS_GOOM_INCLUDE_GOOM_GOOM_DOTS_FX_H_
#define LIBS_GOOM_INCLUDE_GOOM_GOOM_DOTS_FX_H_

#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_visual_fx.h"
#include "goomutils/colormap.h"

#include <cstdint>
#include <vector>

namespace goom
{

struct PluginInfo;
class SoundInfo;

class GoomDots
{
public:
  GoomDots(const uint32_t screenWidth, const uint32_t screenHeight);
  ~GoomDots() noexcept = default;

  void setBuffSettings(const FXBuffSettings&);

  void drawDots(PluginInfo*, Pixel* prevBuff, Pixel* currentBuff);

private:
  const uint32_t screenWidth;
  const uint32_t screenHeight;
  const uint32_t pointWidth;
  const uint32_t pointHeight;

  const float pointWidthDiv2;
  const float pointHeightDiv2;
  const float pointWidthDiv3;
  const float pointHeightDiv3;

  GoomDraw draw;
  FXBuffSettings buffSettings;

  utils::WeightedColorMaps colorMaps;
  const utils::ColorMap* colorMap1 = nullptr;
  const utils::ColorMap* colorMap2 = nullptr;
  const utils::ColorMap* colorMap3 = nullptr;
  const utils::ColorMap* colorMap4 = nullptr;
  const utils::ColorMap* colorMap5 = nullptr;
  uint32_t middleColor = 0;

  void changeColors();

  static std::vector<uint32_t> getColors(const uint32_t color0,
                                         const uint32_t color1,
                                         const size_t numPts);

  float getLargeSoundFactor(const SoundInfo&) const;

  void dotFilter(Pixel* currentBuff,
                 const std::vector<uint32_t>& colors,
                 const float t1,
                 const float t2,
                 const float t3,
                 const float t4,
                 const uint32_t cycle,
                 const uint32_t radius);
};

} // namespace goom

#endif /* LIBS_GOOM_INCLUDE_GOOM_GOOM_DOTS_FX_H_ */
