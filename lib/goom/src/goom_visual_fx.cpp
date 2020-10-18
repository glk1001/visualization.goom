#include "goom_visual_fx.h"

#include <nlohmann/json.hpp>

namespace goom
{

using nlohmann::json;

void to_json(json& j, const FXBuffSettings& settings)
{
  j = json{
      {"buffIntensity", settings.buffIntensity},
      {"allowOverexposed", settings.allowOverexposed},
  };
}

void from_json(const json& j, FXBuffSettings& settings)
{
  j.at("buffIntensity").get_to(settings.buffIntensity);
  j.at("allowOverexposed").get_to(settings.allowOverexposed);
}

} // namespace goom
