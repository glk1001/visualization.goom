#pragma once

#include "vivid/types.h"

#include <map>
#include <string>
#include <vector>

namespace colordata {

  extern const std::map<std::string, std::vector<vivid::srgb_t>> allMaps;

  extern const std::vector<std::string> sequentialsMaps;
  extern const std::vector<std::string> sequentials2Maps;
  extern const std::vector<std::string> divergingMaps;
  extern const std::vector<std::string> diverging_blackMaps;
  extern const std::vector<std::string> qualitativeMaps;
  extern const std::vector<std::string> miscMaps;
  extern const std::vector<std::string> ungroupedMaps;

} // colordata
