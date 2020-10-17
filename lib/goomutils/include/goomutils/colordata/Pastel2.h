#pragma once

#include "vivid/types.h"
#include <vector>

namespace goom::utils::colordata
{

// clang-format off
static const std::vector<vivid::srgb_t> Pastel2
{
  {   0.70196f,   0.88627f,   0.80392f },
  {   0.99216f,   0.80392f,   0.67451f },
  {   0.79608f,   0.83529f,   0.90980f },
  {   0.95686f,   0.79216f,   0.89412f },
  {   0.90196f,   0.96078f,   0.78824f },
  {   1.00000f,   0.94902f,   0.68235f },
  {   0.94510f,   0.88627f,   0.80000f },
  {   0.80000f,   0.80000f,   0.80000f },
};
// clang-format on

} // namespace goom::utils::colordata
