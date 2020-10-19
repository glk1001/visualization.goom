#include "convolve_fx.h"

#include "colorutils.h"
#include "goom_config.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/goomrand.h"
#include "goomutils/mathutils.h"

#include <cereal/archives/json.hpp>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <istream>
#include <ostream>
#include <string>

namespace goom
{

using namespace goom::utils;

inline bool allowOverexposedEvent()
{
  return probabilityOfMInN(1, 100);
}

struct ConvData
{
  FXBuffSettings buffSettings;
  bool allowOverexposed = false;
  uint32_t countSinceOverexposed = 0;
  static constexpr uint32_t maxCountSinceOverexposed = 100;
  PluginParam light;
  PluginParam factor_adj_p;
  PluginParam factor_p;
  PluginParameters params;

  template<class Archive>
  void serialize(Archive&);
};

template<class Archive>
void ConvData::serialize(Archive& ar)
{
  ar(buffSettings, allowOverexposed, countSinceOverexposed, light, factor_adj_p, factor_p);
}

static void convolve_init(VisualFX* _this, PluginInfo*)
{
  ConvData* data = new ConvData{};
  _this->fx_data = data;

  data->buffSettings = defaultFXBuffSettings;
  data->allowOverexposed = false;
  data->countSinceOverexposed = 0;

  data->light = secure_f_param("Screen Brightness");
  data->light.fval = 100.0f;

  data->factor_adj_p = secure_f_param("Flash Intensity");
  data->factor_adj_p.fval = 30.0f;

  data->factor_p = secure_f_feedback("Factor");

  data->params.name = "Bright Flash";
  data->params.params.push_back(&data->light);
  data->params.params.push_back(&data->factor_adj_p);
  data->params.params.push_back(nullptr);
  data->params.params.push_back(&data->factor_p);
  data->params.params.push_back(nullptr);

  _this->params = &data->params;
}

static void saveState(VisualFX* _this, std::ostream& f)
{
  const ConvData* data = static_cast<ConvData*>(_this->fx_data);
  cereal::JSONOutputArchive archiveOut(f);
  archiveOut(*data);
}

static void loadState(VisualFX* _this, std::istream& f)
{
  ConvData* data = static_cast<ConvData*>(_this->fx_data);
  cereal::JSONInputArchive archive_in(f);
  archive_in(*data);
}

static void convolve_free(VisualFX* _this)
{
  std::ofstream f("/tmp/convolve.json");
  saveState(_this, f);
  f << std::endl;
  f.close();

  ConvData* data = static_cast<ConvData*>(_this->fx_data);
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
      dest[i] = getBrighterColorInt(iff, src[i], allowOverexposed);
      i++;
    }
  }
}

static void convolve_apply(VisualFX* _this, PluginInfo* goomInfo, Pixel* src, Pixel* dest)
{
  ConvData* data = static_cast<ConvData*>(_this->fx_data);

  const float ff = (data->factor_p.fval * data->factor_adj_p.fval + data->light.fval) / 100.0f;
  const uint32_t iff = static_cast<uint32_t>(std::round(ff * 256 + 0.0001f));
  constexpr float increaseRate = 1.3;
  constexpr float decayRate = 0.955;
  if (goomInfo->sound->getTimeSinceLastGoom() == 0)
  {
    data->factor_p.fval += goomInfo->sound->getGoomPower() * increaseRate;
  }
  data->factor_p.fval *= decayRate;
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

static void convolve_setBuffSettings(VisualFX* _this, const FXBuffSettings& settings)
{
  ConvData* data = static_cast<ConvData*>(_this->fx_data);
  data->buffSettings = settings;
}

static std::string getFxName(VisualFX*)
{
  return "ZoomFilter";
}

VisualFX convolve_create(void)
{
  VisualFX fx;
  fx.init = convolve_init;
  fx.free = convolve_free;
  fx.setBuffSettings = convolve_setBuffSettings;
  fx.apply = convolve_apply;
  fx.getFxName = getFxName;
  fx.saveState = saveState;
  fx.loadState = loadState;
  fx.save = nullptr;
  fx.restore = nullptr;
  fx.fx_data = 0;
  return fx;
}

} // namespace goom
