#include "all_maps.h"

#include "colordata/Accent.h"
#include "colordata/Blues.h"
#include "colordata/BrBG.h"
#include "colordata/BuGn.h"
#include "colordata/BuPu.h"
#include "colordata/CMRmap.h"
#include "colordata/Dark2.h"
#include "colordata/GnBu.h"
#include "colordata/Greens.h"
#include "colordata/Greys.h"
#include "colordata/OrRd.h"
#include "colordata/Oranges.h"
#include "colordata/PRGn.h"
#include "colordata/Paired.h"
#include "colordata/Pastel1.h"
#include "colordata/Pastel2.h"
#include "colordata/PiYG.h"
#include "colordata/PuBu.h"
#include "colordata/PuBuGn.h"
#include "colordata/PuOr.h"
#include "colordata/PuRd.h"
#include "colordata/Purples.h"
#include "colordata/RdBu.h"
#include "colordata/RdGy.h"
#include "colordata/RdPu.h"
#include "colordata/RdYlBu.h"
#include "colordata/RdYlGn.h"
#include "colordata/Reds.h"
#include "colordata/Set1.h"
#include "colordata/Set2.h"
#include "colordata/Set3.h"
#include "colordata/Spectral.h"
#include "colordata/Wistia.h"
#include "colordata/YlGn.h"
#include "colordata/YlGnBu.h"
#include "colordata/YlOrBr.h"
#include "colordata/YlOrRd.h"
#include "colordata/afmhot.h"
#include "colordata/autumn.h"
#include "colordata/binary.h"
#include "colordata/bone.h"
#include "colordata/brg.h"
#include "colordata/bwr.h"
#include "colordata/cividis.h"
#include "colordata/cool.h"
#include "colordata/coolwarm.h"
#include "colordata/copper.h"
#include "colordata/cubehelix.h"
#include "colordata/flag.h"
#include "colordata/gist_earth.h"
#include "colordata/gist_gray.h"
#include "colordata/gist_heat.h"
#include "colordata/gist_ncar.h"
#include "colordata/gist_rainbow.h"
#include "colordata/gist_stern.h"
#include "colordata/gist_yarg.h"
#include "colordata/gnuplot.h"
#include "colordata/gnuplot2.h"
#include "colordata/gray.h"
#include "colordata/hot.h"
#include "colordata/hsv.h"
#include "colordata/inferno.h"
#include "colordata/jet.h"
#include "colordata/magma.h"
#include "colordata/nipy_spectral.h"
#include "colordata/ocean.h"
#include "colordata/pink.h"
#include "colordata/pink_black_green_w3c_.h"
#include "colordata/plasma.h"
#include "colordata/prism.h"
#include "colordata/rainbow.h"
#include "colordata/red_black_blue.h"
#include "colordata/red_black_green.h"
#include "colordata/red_black_orange.h"
#include "colordata/red_black_sky.h"
#include "colordata/seismic.h"
#include "colordata/spring.h"
#include "colordata/summer.h"
#include "colordata/tab10.h"
#include "colordata/tab20.h"
#include "colordata/tab20b.h"
#include "colordata/tab20c.h"
#include "colordata/terrain.h"
#include "colordata/twilight.h"
#include "colordata/twilight_shifted.h"
#include "colordata/viridis.h"
#include "colordata/winter.h"
#include "colordata/yellow_black_blue.h"
#include "colordata/yellow_black_sky.h"

#include "vivid/types.h"

#include <map>
#include <string>
#include <vector>

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
