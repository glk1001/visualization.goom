#include "goom_config.h"
#include "goom_fx.h"
#include "goom_plugin_info.h"
#include "goomutils/mathutils.h"

#include <cmath>
#include <cstdint>

namespace goom
{

struct ConvData
{
  PluginParam light;
  PluginParam factor_adj_p;
  PluginParam factor_p;
  PluginParameters params;
};

static void convolve_init(VisualFX* _this, PluginInfo*)
{
  ConvData* data = (ConvData*)malloc(sizeof(ConvData));
  _this->fx_data = (void*)data;

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
  free(data);
}

static void create_output_with_brightness(Pixel* src,
                                          Pixel* dest,
                                          PluginInfo* info,
                                          const uint32_t iff)
{
  int i = 0; // info->screen.height * info->screen.width - 1;
  for (uint32_t y = 0; y < info->screen.height; y++)
  {
    for (uint32_t x = 0; x < info->screen.width; x++)
    {
      const uint32_t f0 = (src[i].cop[0] * iff) >> 8;
      const uint32_t f1 = (src[i].cop[1] * iff) >> 8;
      const uint32_t f2 = (src[i].cop[2] * iff) >> 8;
      const uint32_t f3 = (src[i].cop[3] * iff) >> 8;

      dest[i].cop[0] = (f0 & 0xffffff00) ? 0xff : (unsigned char)f0;
      dest[i].cop[1] = (f1 & 0xffffff00) ? 0xff : (unsigned char)f1;
      dest[i].cop[2] = (f2 & 0xffffff00) ? 0xff : (unsigned char)f2;
      dest[i].cop[3] = (f3 & 0xffffff00) ? 0xff : (unsigned char)f3;

      i++;
    }
  }
}

static void convolve_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* info)
{
  ConvData* data = static_cast<ConvData*>(_this->fx_data);
  const float ff = (FVAL(data->factor_p) * FVAL(data->factor_adj_p) + FVAL(data->light)) / 100.0f;
  const uint32_t iff = uint32_t(ff * 256);
  const float INCREASE_RATE = 1.5;
  const float DECAY_RATE = 0.955;

  if (FVAL(info->sound.last_goom_p) > 0.8)
  {
    FVAL(data->factor_p) += FVAL(info->sound.goom_power_p) * INCREASE_RATE;
  }
  FVAL(data->factor_p) *= DECAY_RATE;

  data->factor_p.change_listener(&data->factor_p);

  if ((ff > 0.98f) && (ff < 1.02f))
  {
    memcpy(dest, src, info->screen.size * sizeof(Pixel));
  }
  else
  {
    create_output_with_brightness(src, dest, info, iff);
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
