#include "colordata/all_maps.h"

// clang-format off
#include "colordata/Accent.h"
#include "colordata/afmhot.h"
#include "colordata/autumn.h"
#include "colordata/binary.h"
#include "colordata/Blues.h"
#include "colordata/bone.h"
#include "colordata/BrBG.h"
#include "colordata/brg.h"
#include "colordata/BuGn.h"
#include "colordata/BuPu.h"
#include "colordata/bwr.h"
#include "colordata/cividis.h"
#include "colordata/CMRmap.h"
#include "colordata/cool.h"
#include "colordata/coolwarm.h"
#include "colordata/copper.h"
#include "colordata/cubehelix.h"
#include "colordata/Dark2.h"
#include "colordata/flag.h"
#include "colordata/gist_earth.h"
#include "colordata/gist_gray.h"
#include "colordata/gist_heat.h"
#include "colordata/gist_ncar.h"
#include "colordata/gist_rainbow.h"
#include "colordata/gist_stern.h"
#include "colordata/gist_yarg.h"
#include "colordata/GnBu.h"
#include "colordata/gnuplot.h"
#include "colordata/gnuplot2.h"
#include "colordata/gray.h"
#include "colordata/Greens.h"
#include "colordata/Greys.h"
#include "colordata/hot.h"
#include "colordata/hsv.h"
#include "colordata/inferno.h"
#include "colordata/jet.h"
#include "colordata/magma.h"
#include "colordata/nipy_spectral.h"
#include "colordata/ocean.h"
#include "colordata/Oranges.h"
#include "colordata/OrRd.h"
#include "colordata/Paired.h"
#include "colordata/Pastel1.h"
#include "colordata/Pastel2.h"
#include "colordata/pink.h"
#include "colordata/pink_black_green_w3c_.h"
#include "colordata/PiYG.h"
#include "colordata/plasma.h"
#include "colordata/PRGn.h"
#include "colordata/prism.h"
#include "colordata/PuBu.h"
#include "colordata/PuBuGn.h"
#include "colordata/PuOr.h"
#include "colordata/PuRd.h"
#include "colordata/Purples.h"
#include "colordata/rainbow.h"
#include "colordata/RdBu.h"
#include "colordata/RdGy.h"
#include "colordata/RdPu.h"
#include "colordata/RdYlBu.h"
#include "colordata/RdYlGn.h"
#include "colordata/red_black_blue.h"
#include "colordata/red_black_green.h"
#include "colordata/red_black_orange.h"
#include "colordata/red_black_sky.h"
#include "colordata/Reds.h"
#include "colordata/seismic.h"
#include "colordata/Set1.h"
#include "colordata/Set2.h"
#include "colordata/Set3.h"
#include "colordata/Spectral.h"
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
#include "colordata/Wistia.h"
#include "colordata/yellow_black_blue.h"
#include "colordata/yellow_black_sky.h"
#include "colordata/YlGn.h"
#include "colordata/YlGnBu.h"
#include "colordata/YlOrBr.h"
#include "colordata/YlOrRd.h"
// clang-format on

#include "colordata/colormap_enums.h"
#include "vivid/types.h"

#include <array>
#include <utility>
#include <vector>

namespace goom::utils::colordata
{

// clang-format off
const std::array<std::pair<ColorMapName, std::vector<vivid::srgb_t>>, 89> allMaps
{
  std::make_pair(ColorMapName::Accent, colordata::Accent),
  std::make_pair(ColorMapName::afmhot, colordata::afmhot),
  std::make_pair(ColorMapName::autumn, colordata::autumn),
  std::make_pair(ColorMapName::binary, colordata::binary),
  std::make_pair(ColorMapName::Blues, colordata::Blues),
  std::make_pair(ColorMapName::bone, colordata::bone),
  std::make_pair(ColorMapName::BrBG, colordata::BrBG),
  std::make_pair(ColorMapName::brg, colordata::brg),
  std::make_pair(ColorMapName::BuGn, colordata::BuGn),
  std::make_pair(ColorMapName::BuPu, colordata::BuPu),
  std::make_pair(ColorMapName::bwr, colordata::bwr),
  std::make_pair(ColorMapName::cividis, colordata::cividis),
  std::make_pair(ColorMapName::CMRmap, colordata::CMRmap),
  std::make_pair(ColorMapName::cool, colordata::cool),
  std::make_pair(ColorMapName::coolwarm, colordata::coolwarm),
  std::make_pair(ColorMapName::copper, colordata::copper),
  std::make_pair(ColorMapName::cubehelix, colordata::cubehelix),
  std::make_pair(ColorMapName::Dark2, colordata::Dark2),
  std::make_pair(ColorMapName::flag, colordata::flag),
  std::make_pair(ColorMapName::gist_earth, colordata::gist_earth),
  std::make_pair(ColorMapName::gist_gray, colordata::gist_gray),
  std::make_pair(ColorMapName::gist_heat, colordata::gist_heat),
  std::make_pair(ColorMapName::gist_ncar, colordata::gist_ncar),
  std::make_pair(ColorMapName::gist_rainbow, colordata::gist_rainbow),
  std::make_pair(ColorMapName::gist_stern, colordata::gist_stern),
  std::make_pair(ColorMapName::gist_yarg, colordata::gist_yarg),
  std::make_pair(ColorMapName::GnBu, colordata::GnBu),
  std::make_pair(ColorMapName::gnuplot, colordata::gnuplot),
  std::make_pair(ColorMapName::gnuplot2, colordata::gnuplot2),
  std::make_pair(ColorMapName::gray, colordata::gray),
  std::make_pair(ColorMapName::Greens, colordata::Greens),
  std::make_pair(ColorMapName::Greys, colordata::Greys),
  std::make_pair(ColorMapName::hot, colordata::hot),
  std::make_pair(ColorMapName::hsv, colordata::hsv),
  std::make_pair(ColorMapName::inferno, colordata::inferno),
  std::make_pair(ColorMapName::jet, colordata::jet),
  std::make_pair(ColorMapName::magma, colordata::magma),
  std::make_pair(ColorMapName::nipy_spectral, colordata::nipy_spectral),
  std::make_pair(ColorMapName::ocean, colordata::ocean),
  std::make_pair(ColorMapName::Oranges, colordata::Oranges),
  std::make_pair(ColorMapName::OrRd, colordata::OrRd),
  std::make_pair(ColorMapName::Paired, colordata::Paired),
  std::make_pair(ColorMapName::Pastel1, colordata::Pastel1),
  std::make_pair(ColorMapName::Pastel2, colordata::Pastel2),
  std::make_pair(ColorMapName::pink, colordata::pink),
  std::make_pair(ColorMapName::pink_black_green_w3c_, colordata::pink_black_green_w3c_),
  std::make_pair(ColorMapName::PiYG, colordata::PiYG),
  std::make_pair(ColorMapName::plasma, colordata::plasma),
  std::make_pair(ColorMapName::PRGn, colordata::PRGn),
  std::make_pair(ColorMapName::prism, colordata::prism),
  std::make_pair(ColorMapName::PuBu, colordata::PuBu),
  std::make_pair(ColorMapName::PuBuGn, colordata::PuBuGn),
  std::make_pair(ColorMapName::PuOr, colordata::PuOr),
  std::make_pair(ColorMapName::PuRd, colordata::PuRd),
  std::make_pair(ColorMapName::Purples, colordata::Purples),
  std::make_pair(ColorMapName::rainbow, colordata::rainbow),
  std::make_pair(ColorMapName::RdBu, colordata::RdBu),
  std::make_pair(ColorMapName::RdGy, colordata::RdGy),
  std::make_pair(ColorMapName::RdPu, colordata::RdPu),
  std::make_pair(ColorMapName::RdYlBu, colordata::RdYlBu),
  std::make_pair(ColorMapName::RdYlGn, colordata::RdYlGn),
  std::make_pair(ColorMapName::red_black_blue, colordata::red_black_blue),
  std::make_pair(ColorMapName::red_black_green, colordata::red_black_green),
  std::make_pair(ColorMapName::red_black_orange, colordata::red_black_orange),
  std::make_pair(ColorMapName::red_black_sky, colordata::red_black_sky),
  std::make_pair(ColorMapName::Reds, colordata::Reds),
  std::make_pair(ColorMapName::seismic, colordata::seismic),
  std::make_pair(ColorMapName::Set1, colordata::Set1),
  std::make_pair(ColorMapName::Set2, colordata::Set2),
  std::make_pair(ColorMapName::Set3, colordata::Set3),
  std::make_pair(ColorMapName::Spectral, colordata::Spectral),
  std::make_pair(ColorMapName::spring, colordata::spring),
  std::make_pair(ColorMapName::summer, colordata::summer),
  std::make_pair(ColorMapName::tab10, colordata::tab10),
  std::make_pair(ColorMapName::tab20, colordata::tab20),
  std::make_pair(ColorMapName::tab20b, colordata::tab20b),
  std::make_pair(ColorMapName::tab20c, colordata::tab20c),
  std::make_pair(ColorMapName::terrain, colordata::terrain),
  std::make_pair(ColorMapName::twilight, colordata::twilight),
  std::make_pair(ColorMapName::twilight_shifted, colordata::twilight_shifted),
  std::make_pair(ColorMapName::viridis, colordata::viridis),
  std::make_pair(ColorMapName::winter, colordata::winter),
  std::make_pair(ColorMapName::Wistia, colordata::Wistia),
  std::make_pair(ColorMapName::yellow_black_blue, colordata::yellow_black_blue),
  std::make_pair(ColorMapName::yellow_black_sky, colordata::yellow_black_sky),
  std::make_pair(ColorMapName::YlGn, colordata::YlGn),
  std::make_pair(ColorMapName::YlGnBu, colordata::YlGnBu),
  std::make_pair(ColorMapName::YlOrBr, colordata::YlOrBr),
  std::make_pair(ColorMapName::YlOrRd, colordata::YlOrRd),
};

const std::vector<ColorMapName> perc_unif_sequentialMaps
{
    ColorMapName::cividis,
    ColorMapName::inferno,
    ColorMapName::magma,
    ColorMapName::plasma,
    ColorMapName::viridis,
};
const std::vector<ColorMapName> sequentialMaps
{
    ColorMapName::Blues,
    ColorMapName::BuGn,
    ColorMapName::BuPu,
    ColorMapName::GnBu,
    ColorMapName::Greens,
    ColorMapName::Greys,
    ColorMapName::Oranges,
    ColorMapName::OrRd,
    ColorMapName::PuBu,
    ColorMapName::PuBuGn,
    ColorMapName::PuRd,
    ColorMapName::Purples,
    ColorMapName::RdPu,
    ColorMapName::Reds,
    ColorMapName::YlGn,
    ColorMapName::YlGnBu,
    ColorMapName::YlOrBr,
    ColorMapName::YlOrRd,
};
const std::vector<ColorMapName> sequential2Maps
{
    ColorMapName::afmhot,
    ColorMapName::autumn,
    ColorMapName::binary,
    ColorMapName::bone,
    ColorMapName::cool,
    ColorMapName::copper,
    ColorMapName::gist_gray,
    ColorMapName::gist_heat,
    ColorMapName::gist_yarg,
    ColorMapName::gray,
    ColorMapName::hot,
    ColorMapName::pink,
    ColorMapName::spring,
    ColorMapName::summer,
    ColorMapName::winter,
    ColorMapName::Wistia,
};
const std::vector<ColorMapName> divergingMaps
{
    ColorMapName::BrBG,
    ColorMapName::bwr,
    ColorMapName::coolwarm,
    ColorMapName::PiYG,
    ColorMapName::PRGn,
    ColorMapName::PuOr,
    ColorMapName::RdBu,
    ColorMapName::RdGy,
    ColorMapName::RdYlBu,
    ColorMapName::RdYlGn,
    ColorMapName::seismic,
    ColorMapName::Spectral,
};
const std::vector<ColorMapName> diverging_blackMaps
{
    ColorMapName::pink_black_green_w3c_,
    ColorMapName::red_black_blue,
    ColorMapName::red_black_green,
    ColorMapName::red_black_orange,
    ColorMapName::red_black_sky,
    ColorMapName::yellow_black_blue,
    ColorMapName::yellow_black_sky,
};
const std::vector<ColorMapName> qualitativeMaps
{
    ColorMapName::Accent,
    ColorMapName::Dark2,
    ColorMapName::Paired,
    ColorMapName::Pastel1,
    ColorMapName::Pastel2,
    ColorMapName::Set1,
    ColorMapName::Set2,
    ColorMapName::Set3,
    ColorMapName::tab10,
    ColorMapName::tab20,
    ColorMapName::tab20b,
    ColorMapName::tab20c,
};
const std::vector<ColorMapName> miscMaps
{
    ColorMapName::brg,
    ColorMapName::CMRmap,
    ColorMapName::cubehelix,
    ColorMapName::flag,
    ColorMapName::gist_earth,
    ColorMapName::gist_ncar,
    ColorMapName::gist_rainbow,
    ColorMapName::gist_stern,
    ColorMapName::gnuplot,
    ColorMapName::gnuplot2,
    ColorMapName::hsv,
    ColorMapName::jet,
    ColorMapName::nipy_spectral,
    ColorMapName::ocean,
    ColorMapName::prism,
    ColorMapName::rainbow,
    ColorMapName::terrain,
};
const std::vector<ColorMapName> cyclicMaps
{
    ColorMapName::hsv,
    ColorMapName::twilight,
    ColorMapName::twilight_shifted,
};
// clang-format on

} // namespace goom::utils::colordata
