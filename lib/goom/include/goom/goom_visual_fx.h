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


struct VisualFX
{
  void (*init)(VisualFX* _this, PluginInfo* info);
  void (*free)(VisualFX* _this);
  void (*setBuffSettings)(VisualFX* _this, const FXBuffSettings&);
  void (*apply)(VisualFX* _this, PluginInfo* info, Pixel* src, Pixel* dest);
  void (*save)(VisualFX* _this, const PluginInfo* info, const char* file);
  void (*restore)(VisualFX* _this, PluginInfo* info, const char* file);
  void* fx_data;

  PluginParameters* params;
};

} // namespace goom
#endif
