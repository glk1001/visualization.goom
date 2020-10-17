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

#include "goom_config_param.h"
#include "goom_graphic.h"

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

namespace goom
{

struct PluginInfo;

struct FXBuffSettings
{
  float buffIntensity;
  bool allowOverexposed;
};

static constexpr FXBuffSettings defaultFXBuffSettings{
    .buffIntensity = 0.5,
    .allowOverexposed = true,
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FXBuffSettings, buffIntensity, allowOverexposed)

class VisualFx
{
public:
  VisualFx(const PluginInfo&);
  virtual ~VisualFx() {}

  virtual void setBuffSettings(const FXBuffSettings&) = 0;
  virtual void apply(PluginInfo*, Pixel* prevBuff, Pixel* currentBuff) = 0;
  virtual std::string getFxName() const = 0;
  virtual std::string getStateAsJsonStr() const = 0;
  virtual void setStateFromJsonStr(const std::string& jsonStr) const = 0;

private:
  //PluginParameters* params; // ?????????????????????????????????????????????????????????
};

struct VisualFX
{
  void (*init)(VisualFX* _this, PluginInfo* info);
  void (*free)(VisualFX* _this);
  void (*setBuffSettings)(VisualFX* _this, const FXBuffSettings&);
  void (*apply)(VisualFX* _this, PluginInfo* info, Pixel* prevBuff, Pixel* currentBuff);
  std::string (*getFxName)(VisualFX* _this);
  std::string (*getStateAsJsonStr)(VisualFX* _this, const PluginInfo*);
  void (*setStateFromJsonStr)(VisualFX* _this, PluginInfo*, const std::string& jsonStr);
  void (*save)(VisualFX* _this, const PluginInfo* info, const char* file);
  void (*restore)(VisualFX* _this, PluginInfo* info, const char* file);
  void* fx_data;

  PluginParameters* params;
};

} // namespace goom
#endif
