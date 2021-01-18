#pragma once

#include "vivid/types.h"
#include <vector>

namespace GOOM::UTILS::COLOR_DATA
{

// clang-format off
static const std::vector<vivid::srgb_t> Paired
{
  {   0.65098f,   0.80784f,   0.89020f },
  {   0.12157f,   0.47059f,   0.70588f },
  {   0.69804f,   0.87451f,   0.54118f },
  {   0.20000f,   0.62745f,   0.17255f },
  {   0.98431f,   0.60392f,   0.60000f },
  {   0.89020f,   0.10196f,   0.10980f },
  {   0.99216f,   0.74902f,   0.43529f },
  {   1.00000f,   0.49804f,   0.00000f },
  {   0.79216f,   0.69804f,   0.83922f },
  {   0.41569f,   0.23922f,   0.60392f },
  {   1.00000f,   1.00000f,   0.60000f },
  {   0.69412f,   0.34902f,   0.15686f },
};
// clang-format on

} // namespace GOOM::UTILS::COLOR_DATA
