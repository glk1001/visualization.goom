#include "goomutils/all_maps.h"

#include "vivid/types.h"

#include <map>
#include <string>
#include <vector>
#include "../include/goomutils/colordata/afmhot.h"
#include "../include/goomutils/colordata/autumn.h"
#include "../include/goomutils/colordata/Accent.h"
#include "../include/goomutils/colordata/binary.h"
#include "../include/goomutils/colordata/bone.h"
#include "../include/goomutils/colordata/brg.h"
#include "../include/goomutils/colordata/bwr.h"
#include "../include/goomutils/colordata/Blues.h"
#include "../include/goomutils/colordata/BrBG.h"
#include "../include/goomutils/colordata/BuGn.h"
#include "../include/goomutils/colordata/BuPu.h"
#include "../include/goomutils/colordata/cividis.h"
#include "../include/goomutils/colordata/cool.h"
#include "../include/goomutils/colordata/coolwarm.h"
#include "../include/goomutils/colordata/copper.h"
#include "../include/goomutils/colordata/cubehelix.h"
#include "../include/goomutils/colordata/CMRmap.h"
#include "../include/goomutils/colordata/Dark2.h"
#include "../include/goomutils/colordata/flag.h"
#include "../include/goomutils/colordata/gist_earth.h"
#include "../include/goomutils/colordata/gist_gray.h"
#include "../include/goomutils/colordata/gist_heat.h"
#include "../include/goomutils/colordata/gist_ncar.h"
#include "../include/goomutils/colordata/gist_rainbow.h"
#include "../include/goomutils/colordata/gist_stern.h"
#include "../include/goomutils/colordata/gist_yarg.h"
#include "../include/goomutils/colordata/gnuplot.h"
#include "../include/goomutils/colordata/gnuplot2.h"
#include "../include/goomutils/colordata/gray.h"
#include "../include/goomutils/colordata/GnBu.h"
#include "../include/goomutils/colordata/Greens.h"
#include "../include/goomutils/colordata/Greys.h"
#include "../include/goomutils/colordata/hot.h"
#include "../include/goomutils/colordata/hsv.h"
#include "../include/goomutils/colordata/inferno.h"
#include "../include/goomutils/colordata/jet.h"
#include "../include/goomutils/colordata/magma.h"
#include "../include/goomutils/colordata/nipy_spectral.h"
#include "../include/goomutils/colordata/ocean.h"
#include "../include/goomutils/colordata/Oranges.h"
#include "../include/goomutils/colordata/OrRd.h"
#include "../include/goomutils/colordata/pink.h"
#include "../include/goomutils/colordata/pink_black_green_w3c_.h"
#include "../include/goomutils/colordata/plasma.h"
#include "../include/goomutils/colordata/prism.h"
#include "../include/goomutils/colordata/Paired.h"
#include "../include/goomutils/colordata/Pastel1.h"
#include "../include/goomutils/colordata/Pastel2.h"
#include "../include/goomutils/colordata/PiYG.h"
#include "../include/goomutils/colordata/PRGn.h"
#include "../include/goomutils/colordata/PuBu.h"
#include "../include/goomutils/colordata/PuBuGn.h"
#include "../include/goomutils/colordata/PuOr.h"
#include "../include/goomutils/colordata/Purples.h"
#include "../include/goomutils/colordata/PuRd.h"
#include "../include/goomutils/colordata/rainbow.h"
#include "../include/goomutils/colordata/red_black_blue.h"
#include "../include/goomutils/colordata/red_black_green.h"
#include "../include/goomutils/colordata/red_black_orange.h"
#include "../include/goomutils/colordata/red_black_sky.h"
#include "../include/goomutils/colordata/RdBu.h"
#include "../include/goomutils/colordata/RdGy.h"
#include "../include/goomutils/colordata/RdPu.h"
#include "../include/goomutils/colordata/RdYlBu.h"
#include "../include/goomutils/colordata/RdYlGn.h"
#include "../include/goomutils/colordata/Reds.h"
#include "../include/goomutils/colordata/seismic.h"
#include "../include/goomutils/colordata/spring.h"
#include "../include/goomutils/colordata/summer.h"
#include "../include/goomutils/colordata/Set1.h"
#include "../include/goomutils/colordata/Set2.h"
#include "../include/goomutils/colordata/Set3.h"
#include "../include/goomutils/colordata/Spectral.h"
#include "../include/goomutils/colordata/tab10.h"
#include "../include/goomutils/colordata/tab20.h"
#include "../include/goomutils/colordata/tab20b.h"
#include "../include/goomutils/colordata/tab20c.h"
#include "../include/goomutils/colordata/terrain.h"
#include "../include/goomutils/colordata/twilight.h"
#include "../include/goomutils/colordata/twilight_shifted.h"
#include "../include/goomutils/colordata/viridis.h"
#include "../include/goomutils/colordata/winter.h"
#include "../include/goomutils/colordata/Wistia.h"
#include "../include/goomutils/colordata/yellow_black_blue.h"
#include "../include/goomutils/colordata/yellow_black_sky.h"
#include "../include/goomutils/colordata/YlGn.h"
#include "../include/goomutils/colordata/YlGnBu.h"
#include "../include/goomutils/colordata/YlOrBr.h"
#include "../include/goomutils/colordata/YlOrRd.h"

namespace colordata {

  const std::map<std::string, std::vector<vivid::srgb_t>> allMaps = {
    { "Accent", colordata::Accent },
    { "Blues", colordata::Blues },
    { "BrBG", colordata::BrBG },
    { "BuGn", colordata::BuGn },
    { "BuPu", colordata::BuPu },
    { "CMRmap", colordata::CMRmap },
    { "Dark2", colordata::Dark2 },
    { "GnBu", colordata::GnBu },
    { "Greens", colordata::Greens },
    { "Greys", colordata::Greys },
    { "OrRd", colordata::OrRd },
    { "Oranges", colordata::Oranges },
    { "PRGn", colordata::PRGn },
    { "Paired", colordata::Paired },
    { "Pastel1", colordata::Pastel1 },
    { "Pastel2", colordata::Pastel2 },
    { "PiYG", colordata::PiYG },
    { "PuBu", colordata::PuBu },
    { "PuBuGn", colordata::PuBuGn },
    { "PuOr", colordata::PuOr },
    { "PuRd", colordata::PuRd },
    { "Purples", colordata::Purples },
    { "RdBu", colordata::RdBu },
    { "RdGy", colordata::RdGy },
    { "RdPu", colordata::RdPu },
    { "RdYlBu", colordata::RdYlBu },
    { "RdYlGn", colordata::RdYlGn },
    { "Reds", colordata::Reds },
    { "Set1", colordata::Set1 },
    { "Set2", colordata::Set2 },
    { "Set3", colordata::Set3 },
    { "Spectral", colordata::Spectral },
    { "Wistia", colordata::Wistia },
    { "YlGn", colordata::YlGn },
    { "YlGnBu", colordata::YlGnBu },
    { "YlOrBr", colordata::YlOrBr },
    { "YlOrRd", colordata::YlOrRd },
    { "afmhot", colordata::afmhot },
    { "autumn", colordata::autumn },
    { "binary", colordata::binary },
    { "bone", colordata::bone },
    { "brg", colordata::brg },
    { "bwr", colordata::bwr },
    { "cividis", colordata::cividis },
    { "cool", colordata::cool },
    { "coolwarm", colordata::coolwarm },
    { "copper", colordata::copper },
    { "cubehelix", colordata::cubehelix },
    { "flag", colordata::flag },
    { "gist_earth", colordata::gist_earth },
    { "gist_gray", colordata::gist_gray },
    { "gist_heat", colordata::gist_heat },
    { "gist_ncar", colordata::gist_ncar },
    { "gist_rainbow", colordata::gist_rainbow },
    { "gist_stern", colordata::gist_stern },
    { "gist_yarg", colordata::gist_yarg },
    { "gnuplot", colordata::gnuplot },
    { "gnuplot2", colordata::gnuplot2 },
    { "gray", colordata::gray },
    { "hot", colordata::hot },
    { "hsv", colordata::hsv },
    { "inferno", colordata::inferno },
    { "jet", colordata::jet },
    { "magma", colordata::magma },
    { "nipy_spectral", colordata::nipy_spectral },
    { "ocean", colordata::ocean },
    { "pink", colordata::pink },
    { "pink_black_green_w3c_", colordata::pink_black_green_w3c_ },
    { "plasma", colordata::plasma },
    { "prism", colordata::prism },
    { "rainbow", colordata::rainbow },
    { "red_black_blue", colordata::red_black_blue },
    { "red_black_green", colordata::red_black_green },
    { "red_black_orange", colordata::red_black_orange },
    { "red_black_sky", colordata::red_black_sky },
    { "seismic", colordata::seismic },
    { "spring", colordata::spring },
    { "summer", colordata::summer },
    { "tab10", colordata::tab10 },
    { "tab20", colordata::tab20 },
    { "tab20b", colordata::tab20b },
    { "tab20c", colordata::tab20c },
    { "terrain", colordata::terrain },
    { "twilight", colordata::twilight },
    { "twilight_shifted", colordata::twilight_shifted },
    { "viridis", colordata::viridis },
    { "winter", colordata::winter },
    { "yellow_black_blue", colordata::yellow_black_blue },
    { "yellow_black_sky", colordata::yellow_black_sky },
  };

  const std::vector<std::string> sequentialsMaps = {
    "Blues",
    "BuGn",
    "BuPu",
    "GnBu",
    "Greens",
    "Greys",
    "OrRd",
    "Oranges",
    "PuBu",
    "PuBuGn",
    "PuRd",
    "Purples",
    "RdPu",
    "Reds",
    "YlGn",
    "YlGnBu",
    "YlOrBr",
    "YlOrRd",
  };
  const std::vector<std::string> sequentials2Maps = {
    "afmhot",
    "autumn",
    "bone",
    "cool",
    "copper",
    "gist_heat",
    "gray",
    "hot",
    "pink",
    "spring",
    "summer",
    "winter",
  };
  const std::vector<std::string> divergingMaps = {
    "BrBG",
    "PRGn",
    "PiYG",
    "PuOr",
    "RdBu",
    "RdGy",
    "RdYlBu",
    "RdYlGn",
    "Spectral",
    "bwr",
    "coolwarm",
    "seismic",
  };
  const std::vector<std::string> diverging_blackMaps = {
    "red_black_sky",
    "red_black_blue",
    "red_black_green",
    "yellow_black_blue",
    "yellow_black_sky",
    "red_black_orange",
    "pink_black_green_w3c_",
  };
  const std::vector<std::string> qualitativeMaps = {
    "Accent",
    "Dark2",
    "Paired",
    "Pastel1",
    "Pastel2",
    "Set1",
    "Set2",
    "Set3",
  };
  const std::vector<std::string> miscMaps = {
    "gist_earth",
    "terrain",
    "ocean",
    "gist_stern",
    "brg",
    "CMRmap",
    "cubehelix",
    "gnuplot",
    "gnuplot2",
    "gist_ncar",
    "nipy_spectral",
    "jet",
    "rainbow",
    "gist_rainbow",
    "hsv",
    "flag",
    "prism",
  };
  const std::vector<std::string> ungroupedMaps = {
    "Wistia",
    "binary",
    "cividis",
    "gist_gray",
    "gist_yarg",
    "inferno",
    "magma",
    "plasma",
    "tab10",
    "tab20",
    "tab20b",
    "tab20c",
    "twilight",
    "twilight_shifted",
    "viridis",
  };

} // colordata
