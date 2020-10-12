#include "convolve_fx.h"

#include "colorutils.h"
#include "goom_config.h"
#include "goom_plugin_info.h"
#include "goomutils/goomrand.h"
#include "goomutils/mathutils.h"

#include <cmath>
#include <cstdint>

namespace goom
{

using namespace goom::utils;

inline bool allowOverexposedEvent()
{
  return probabilityOfMInN(1, 100);
}

struct ConvData
{
  bool allowOverexposed = false;
  uint32_t countSinceOverexposed = 0;
  static constexpr uint32_t maxCountSinceOverexposed = 100;
  PluginParam light;
  PluginParam factor_adj_p;
  PluginParam factor_p;
  PluginParameters params;
};

static void convolve_init(VisualFX* _this, PluginInfo*)
{
  ConvData* data = new ConvData{};
  _this->fx_data = data;

  data->allowOverexposed = false;
  data->countSinceOverexposed = 0;

  data->light = secure_f_param("Screen Brightness");
  data->light.param.fval.max = 300.0f;
  data->light.param.fval.step = 1.0f;
  data->light.param.fval.value = 100.0f;

  data->factor_adj_p = secure_f_param("Flash Intensity");
  data->factor_adj_p.param.fval.max = 200.0f;
  data->factor_adj_p.param.fval.step = 1.0f;
  data->factor_adj_p.param.fval.value = 30.0f;

  data->factor_p = secure_f_feedback("Factor");

  data->params = plugin_parameters("Bright Flash", 5);
  data->params.params[0] = &data->light;
  data->params.params[1] = &data->factor_adj_p;
  data->params.params[2] = 0;
  data->params.params[3] = &data->factor_p;
  data->params.params[4] = 0;

  _this->params = &data->params;
}

static void convolve_free(VisualFX* _this)
{
  ConvData* data = static_cast<ConvData*>(_this->fx_data);
  free(data->params.params);
  delete data;
}

static void createOutputWithBrightness(const Pixel* src,
                                       Pixel* dest,
                                       const PluginInfo* goomInfo,
                                       const uint32_t iff,
                                       const bool allowOverexposed)
{
  int i = 0; // info->screen.height * info->screen.width - 1;
  for (uint32_t y = 0; y < goomInfo->screen.height; y++)
  {
    for (uint32_t x = 0; x < goomInfo->screen.width; x++)
    {
      dest[i] = getBrighterColor(iff, src[i], allowOverexposed);
      i++;
    }
  }
}

static void convolve_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* goomInfo)
{
  ConvData* data = static_cast<ConvData*>(_this->fx_data);

  const float ff = (FVAL(data->factor_p) * FVAL(data->factor_adj_p) + FVAL(data->light)) / 100.0f;
  const uint32_t iff = static_cast<uint32_t>(std::round(ff * 256 + 0.0001f));
  constexpr float increaseRate = 1.3;
  constexpr float decayRate = 0.955;
  if (goomInfo->sound->getTimeSinceLastGoom() == 0)
  {
    FVAL(data->factor_p) += goomInfo->sound->getGoomPower() * increaseRate;
  }
  FVAL(data->factor_p) *= decayRate;
  data->factor_p.change_listener(&data->factor_p);

  if (data->allowOverexposed)
  {
    data->countSinceOverexposed++;
    if (data->countSinceOverexposed > ConvData::maxCountSinceOverexposed)
    {
      data->allowOverexposed = false;
    }
  }
  else if (allowOverexposedEvent())
  {
    data->countSinceOverexposed = 0;
    data->allowOverexposed = true;
  }


  data->allowOverexposed = true;


  if (std::fabs(1.0 - ff) < 0.02)
  {
    memcpy(dest, src, goomInfo->screen.size * sizeof(Pixel));
  }
  else
  {
    createOutputWithBrightness(src, dest, goomInfo, iff, data->allowOverexposed);
  }
}

VisualFX convolve_create(void)
{
  VisualFX vfx;
  vfx.init = convolve_init;
  vfx.free = convolve_free;
  vfx.apply = convolve_apply;
  vfx.save = NULL;
  vfx.restore = NULL;
  vfx.fx_data = 0;
  return vfx;
}

} // namespace goom
