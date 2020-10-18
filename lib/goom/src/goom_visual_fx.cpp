#include "goom_visual_fx.h"

#include <cereal/archives/json.hpp>

namespace goom
{

template<class Archive>
void FXBuffSettings::serialize(Archive& ar)
{
  ar(buffIntensity, allowOverexposed);
}

} // namespace goom
