#ifndef IFS_FX_H
#define IFS_FX_H

/*
 * File created 11 april 2002 by JeKo <jeko@free.fr>
 */

#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_visual_fx.h"

#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace goom
{

struct PluginInfo;
struct IfsData;
struct IfsPoint;
struct Fractal;

class IfsFx : public VisualFx
{
public:
  IfsFx() = delete;
  explicit IfsFx(PluginInfo*);
  ~IfsFx() noexcept;
  IfsFx(const IfsFx&) = delete;
  IfsFx& operator=(const IfsFx&) = delete;

  void renew();
  void setIfsIncrement(const int val);

  void setBuffSettings(const FXBuffSettings&) override;

  void start() override;

  void apply(Pixel* prevBuff, Pixel* currentBuff) override;

  std::string getFxName() const override;
  void saveState(std::ostream&) override;
  void loadState(std::istream&) override;

  void log(const StatsLogValueFunc& logVal) const override;
  void finish() override;

private:
  bool enabled = true;
  PluginInfo* const goomInfo;
  std::unique_ptr<IfsData> fxData;

  void updateIfs(Pixel* prevBuff, Pixel* currentBuff);
  const std::vector<IfsPoint>& drawIfs();
  void drawFractal();

  struct IfsUpdateData;
  std::unique_ptr<IfsUpdateData> updateData;

  void changeColormaps();
  void updatePixelBuffers(Pixel* prevBuff,
                          Pixel* currentBuff,
                          const size_t numPoints,
                          const std::vector<IfsPoint>& points,
                          const Pixel& color);
  void updateColors();
  void updateColorsModeMer();
  void updateColorsModeMerver();
  void updateColorsModeFeu();
};

} // namespace goom
#endif
