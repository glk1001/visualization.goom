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

namespace goom
{

struct PluginInfo;

struct VisualFX
{
  void (*init)(struct VisualFX* _this, PluginInfo* info);
  void (*free)(struct VisualFX* _this);
  void (*apply)(struct VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* info);
  void (*save)(struct VisualFX* _this, const PluginInfo* info, const char* file);
  void (*restore)(struct VisualFX* _this, PluginInfo* info, const char* file);
  void* fx_data;

  PluginParameters* params;
};

} // namespace goom
#endif
