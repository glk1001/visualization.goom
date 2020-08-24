#include "goom_config.h"
#include "goom_fx.h"
#include "goom_plugin_info.h"

#include "goomutils/mathtools.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _CONV_DATA {
  PluginParam light;
  PluginParam factor_adj_p;
  PluginParam factor_p;
  PluginParameters params;
} ConvData;

static void convolve_init(VisualFX* _this, PluginInfo* info)
{
  ConvData* data;
  data = (ConvData*)malloc(sizeof(ConvData));
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
  ConvData* data = (ConvData* )_this->fx_data;
  free(data->params.params);
  free(data);
}

static void create_output_with_brightness(Pixel* src, Pixel* dest, PluginInfo* info, unsigned int iff)
{
  int i = 0; //info->screen.height * info->screen.width - 1;
  for (int y = 0; y < info->screen.height; y++) {
    for (int x = 0; x < info->screen.width; x++) {
      const unsigned int f0 = (src[i].cop[0] * iff) >> 8;
      const unsigned int f1 = (src[i].cop[1] * iff) >> 8;
      const unsigned int f2 = (src[i].cop[2] * iff) >> 8;
      const unsigned int f3 = (src[i].cop[3] * iff) >> 8;

      dest[i].cop[0] = (f0 & 0xffffff00) ? 0xff : (unsigned char)f0;
      dest[i].cop[1] = (f1 & 0xffffff00) ? 0xff : (unsigned char)f1;
      dest[i].cop[2] = (f2 & 0xffffff00) ? 0xff : (unsigned char)f2;
      dest[i].cop[3] = (f3 & 0xffffff00) ? 0xff : (unsigned char)f3;

      i++;
    }
  }
}

/*#include <stdint.h>

 static uint64_t GetTick()
 {
 uint64_t x;
 asm volatile ("RDTSC" : "=A" (x));
 return x;
 }*/

static void convolve_apply(VisualFX* _this, Pixel* src, Pixel* dest, PluginInfo* info)
{
  ConvData* data = (ConvData*)_this->fx_data;
  const float ff = (FVAL(data->factor_p) * FVAL(data->factor_adj_p) + FVAL(data->light)) / 100.0f;
  const unsigned int iff = (unsigned int)(ff * 256);
  {
    const float INCREASE_RATE = 1.5;
    const float DECAY_RATE = 0.955;
    if (FVAL(info->sound.last_goom_p) > 0.8) {
      FVAL(data->factor_p) += FVAL(info->sound.goom_power_p) * INCREASE_RATE;
    }
    FVAL(data->factor_p) *= DECAY_RATE;

    data->factor_p.change_listener(&data->factor_p);
  }

  if ((ff > 0.98f) && (ff < 1.02f)) {
    memcpy(dest, src, (size_t)info->screen.size * sizeof(Pixel));
  } else {
    create_output_with_brightness(src, dest, info, iff);
  }
  /*
   //   Benching suite...
   {
   uint64_t before, after;
   double   timed;
   static double stimed = 10000.0;
   before = GetTick();
   data->visibility = 1.0;
   create_output_with_brightness(_this,src,dest,info,iff);
   after  = GetTick();
   timed = (double)((after-before) / info->screen.size);
   if (timed < stimed) {
   stimed = timed;
   printf ("CLK = %3.0f CPP\n", stimed);
   }
   }
   */
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
