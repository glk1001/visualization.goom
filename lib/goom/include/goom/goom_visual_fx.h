#ifndef _VISUAL_FX_H
#define _VISUAL_FX_H

/**
 * File created on 2003-05-21 by Jeko.
 * (c)2003, JC Hoelt for iOS-software.
 *
 * LGPL Licence.
 * If you use this file on a visual program,
 *    please make my name being visible on it.
 */

#include "goom_config.h"
#include "goom_graphic.h"

#include <cereal/archives/json.hpp>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

namespace goom
{

struct FXBuffSettings
{
  float buffIntensity;
  bool allowOverexposed;
  template<class Archive>
  void serialize(Archive&);
};

template<class Archive>
void FXBuffSettings::serialize(Archive& ar)
{
  ar(buffIntensity, allowOverexposed);
}

static constexpr FXBuffSettings defaultFXBuffSettings{
    .buffIntensity = 0.5,
    .allowOverexposed = true,
};

class VisualFx
{
public:
  virtual ~VisualFx() {}

  virtual void setBuffSettings(const FXBuffSettings&) = 0;
  virtual void apply(Pixel* prevBuff, Pixel* currentBuff) = 0;
  virtual std::string getFxName() const = 0;
  virtual void saveState(std::ostream&) = 0;
  virtual void loadState(std::istream&) = 0;
};

struct PluginInfo;

struct VisualFX
{
  void (*init)(VisualFX* _this, PluginInfo* info);
  void (*free)(VisualFX* _this);
  void (*setBuffSettings)(VisualFX* _this, const FXBuffSettings&);
  void (*apply)(VisualFX* _this, PluginInfo* info, Pixel* prevBuff, Pixel* currentBuff);
  std::string (*getFxName)(VisualFX* _this);
  void (*saveState)(VisualFX* _this, std::ostream&);
  void (*loadState)(VisualFX* _this, std::istream&);
  void (*save)(VisualFX* _this, const PluginInfo* info, const char* file);
  void (*restore)(VisualFX* _this, PluginInfo* info, const char* file);
  void* fx_data;
};

} // namespace goom
#endif
