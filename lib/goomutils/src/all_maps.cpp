#include "../include/goomutils/colordata/all_maps.h"

#include "../include/goomutils/colordata/Accent.h"
#include "../include/goomutils/colordata/Blues.h"
#include "../include/goomutils/colordata/BrBG.h"
#include "../include/goomutils/colordata/BuGn.h"
#include "../include/goomutils/colordata/BuPu.h"
#include "../include/goomutils/colordata/CMRmap.h"
#include "../include/goomutils/colordata/Dark2.h"
#include "../include/goomutils/colordata/GnBu.h"
#include "../include/goomutils/colordata/Greens.h"
#include "../include/goomutils/colordata/Greys.h"
#include "../include/goomutils/colordata/OrRd.h"
#include "../include/goomutils/colordata/Oranges.h"
#include "../include/goomutils/colordata/PRGn.h"
#include "../include/goomutils/colordata/Paired.h"
#include "../include/goomutils/colordata/Pastel1.h"
#include "../include/goomutils/colordata/Pastel2.h"
#include "../include/goomutils/colordata/PiYG.h"
#include "../include/goomutils/colordata/PuBu.h"
#include "../include/goomutils/colordata/PuBuGn.h"
#include "../include/goomutils/colordata/PuOr.h"
#include "../include/goomutils/colordata/PuRd.h"
#include "../include/goomutils/colordata/Purples.h"
#include "../include/goomutils/colordata/RdBu.h"
#include "../include/goomutils/colordata/RdGy.h"
#include "../include/goomutils/colordata/RdPu.h"
#include "../include/goomutils/colordata/RdYlBu.h"
#include "../include/goomutils/colordata/RdYlGn.h"
#include "../include/goomutils/colordata/Reds.h"
#include "../include/goomutils/colordata/Set1.h"
#include "../include/goomutils/colordata/Set2.h"
#include "../include/goomutils/colordata/Set3.h"
#include "../include/goomutils/colordata/Spectral.h"
#include "../include/goomutils/colordata/Wistia.h"
#include "../include/goomutils/colordata/YlGn.h"
#include "../include/goomutils/colordata/YlGnBu.h"
#include "../include/goomutils/colordata/YlOrBr.h"
#include "../include/goomutils/colordata/YlOrRd.h"
#include "../include/goomutils/colordata/afmhot.h"
#include "../include/goomutils/colordata/autumn.h"
#include "../include/goomutils/colordata/binary.h"
#include "../include/goomutils/colordata/bone.h"
#include "../include/goomutils/colordata/brg.h"
#include "../include/goomutils/colordata/bwr.h"
#include "../include/goomutils/colordata/cividis.h"
#include "../include/goomutils/colordata/cool.h"
#include "../include/goomutils/colordata/coolwarm.h"
#include "../include/goomutils/colordata/copper.h"
#include "../include/goomutils/colordata/cubehelix.h"
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
#include "../include/goomutils/colordata/hot.h"
#include "../include/goomutils/colordata/hsv.h"
#include "../include/goomutils/colordata/inferno.h"
#include "../include/goomutils/colordata/jet.h"
#include "../include/goomutils/colordata/magma.h"
#include "../include/goomutils/colordata/nipy_spectral.h"
#include "../include/goomutils/colordata/ocean.h"
#include "../include/goomutils/colordata/pink.h"
#include "../include/goomutils/colordata/pink_black_green_w3c_.h"
#include "../include/goomutils/colordata/plasma.h"
#include "../include/goomutils/colordata/prism.h"
#include "../include/goomutils/colordata/rainbow.h"
#include "../include/goomutils/colordata/red_black_blue.h"
#include "../include/goomutils/colordata/red_black_green.h"
#include "../include/goomutils/colordata/red_black_orange.h"
#include "../include/goomutils/colordata/red_black_sky.h"
#include "../include/goomutils/colordata/seismic.h"
#include "../include/goomutils/colordata/spring.h"
#include "../include/goomutils/colordata/summer.h"
#include "../include/goomutils/colordata/tab10.h"
#include "../include/goomutils/colordata/tab20.h"
#include "../include/goomutils/colordata/tab20b.h"
#include "../include/goomutils/colordata/tab20c.h"
#include "../include/goomutils/colordata/terrain.h"
#include "../include/goomutils/colordata/twilight.h"
#include "../include/goomutils/colordata/twilight_shifted.h"
#include "../include/goomutils/colordata/viridis.h"
#include "../include/goomutils/colordata/winter.h"
#include "../include/goomutils/colordata/yellow_black_blue.h"
#include "../include/goomutils/colordata/yellow_black_sky.h"
#include "goomutils/colormap_enums.h"
#include "vivid/types.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace colordata
{
const std::array<std::pair<std::string, std::vector<vivid::srgb_t>>, 89> allMaps = {
    std::make_pair("Accent", colordata::Accent),
    std::make_pair("afmhot", colordata::afmhot),
    std::make_pair("autumn", colordata::autumn),
    std::make_pair("binary", colordata::binary),
    std::make_pair("Blues", colordata::Blues),
    std::make_pair("bone", colordata::bone),
    std::make_pair("BrBG", colordata::BrBG),
    std::make_pair("brg", colordata::brg),
    std::make_pair("BuGn", colordata::BuGn),
    std::make_pair("BuPu", colordata::BuPu),
    std::make_pair("bwr", colordata::bwr),
    std::make_pair("cividis", colordata::cividis),
    std::make_pair("CMRmap", colordata::CMRmap),
    std::make_pair("cool", colordata::cool),
    std::make_pair("coolwarm", colordata::coolwarm),
    std::make_pair("copper", colordata::copper),
    std::make_pair("cubehelix", colordata::cubehelix),
    std::make_pair("Dark2", colordata::Dark2),
    std::make_pair("flag", colordata::flag),
    std::make_pair("gist_earth", colordata::gist_earth),
    std::make_pair("gist_gray", colordata::gist_gray),
    std::make_pair("gist_heat", colordata::gist_heat),
    std::make_pair("gist_ncar", colordata::gist_ncar),
    std::make_pair("gist_rainbow", colordata::gist_rainbow),
    std::make_pair("gist_stern", colordata::gist_stern),
    std::make_pair("gist_yarg", colordata::gist_yarg),
    std::make_pair("GnBu", colordata::GnBu),
    std::make_pair("gnuplot", colordata::gnuplot),
    std::make_pair("gnuplot2", colordata::gnuplot2),
    std::make_pair("gray", colordata::gray),
    std::make_pair("Greens", colordata::Greens),
    std::make_pair("Greys", colordata::Greys),
    std::make_pair("hot", colordata::hot),
    std::make_pair("hsv", colordata::hsv),
    std::make_pair("inferno", colordata::inferno),
    std::make_pair("jet", colordata::jet),
    std::make_pair("magma", colordata::magma),
    std::make_pair("nipy_spectral", colordata::nipy_spectral),
    std::make_pair("ocean", colordata::ocean),
    std::make_pair("Oranges", colordata::Oranges),
    std::make_pair("OrRd", colordata::OrRd),
    std::make_pair("Paired", colordata::Paired),
    std::make_pair("Pastel1", colordata::Pastel1),
    std::make_pair("Pastel2", colordata::Pastel2),
    std::make_pair("pink", colordata::pink),
    std::make_pair("pink_black_green_w3c_", colordata::pink_black_green_w3c_),
    std::make_pair("PiYG", colordata::PiYG),
    std::make_pair("plasma", colordata::plasma),
    std::make_pair("PRGn", colordata::PRGn),
    std::make_pair("prism", colordata::prism),
    std::make_pair("PuBu", colordata::PuBu),
    std::make_pair("PuBuGn", colordata::PuBuGn),
    std::make_pair("PuOr", colordata::PuOr),
    std::make_pair("PuRd", colordata::PuRd),
    std::make_pair("Purples", colordata::Purples),
    std::make_pair("rainbow", colordata::rainbow),
    std::make_pair("RdBu", colordata::RdBu),
    std::make_pair("RdGy", colordata::RdGy),
    std::make_pair("RdPu", colordata::RdPu),
    std::make_pair("RdYlBu", colordata::RdYlBu),
    std::make_pair("RdYlGn", colordata::RdYlGn),
    std::make_pair("red_black_blue", colordata::red_black_blue),
    std::make_pair("red_black_green", colordata::red_black_green),
    std::make_pair("red_black_orange", colordata::red_black_orange),
    std::make_pair("red_black_sky", colordata::red_black_sky),
    std::make_pair("Reds", colordata::Reds),
    std::make_pair("seismic", colordata::seismic),
    std::make_pair("Set1", colordata::Set1),
    std::make_pair("Set2", colordata::Set2),
    std::make_pair("Set3", colordata::Set3),
    std::make_pair("Spectral", colordata::Spectral),
    std::make_pair("spring", colordata::spring),
    std::make_pair("summer", colordata::summer),
    std::make_pair("tab10", colordata::tab10),
    std::make_pair("tab20", colordata::tab20),
    std::make_pair("tab20b", colordata::tab20b),
    std::make_pair("tab20c", colordata::tab20c),
    std::make_pair("terrain", colordata::terrain),
    std::make_pair("twilight", colordata::twilight),
    std::make_pair("twilight_shifted", colordata::twilight_shifted),
    std::make_pair("viridis", colordata::viridis),
    std::make_pair("winter", colordata::winter),
    std::make_pair("Wistia", colordata::Wistia),
    std::make_pair("yellow_black_blue", colordata::yellow_black_blue),
    std::make_pair("yellow_black_sky", colordata::yellow_black_sky),
    std::make_pair("YlGn", colordata::YlGn),
    std::make_pair("YlGnBu", colordata::YlGnBu),
    std::make_pair("YlOrBr", colordata::YlOrBr),
    std::make_pair("YlOrRd", colordata::YlOrRd),
};

// clang-format off
const std::vector<ColorMapName> perc_unif_sequentialMaps =
{
    ColorMapName::cividis,
    ColorMapName::inferno,
    ColorMapName::magma,
    ColorMapName::plasma,
    ColorMapName::viridis,
};
const std::vector<ColorMapName> sequentialMaps =
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
const std::vector<ColorMapName> sequential2Maps =
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
const std::vector<ColorMapName> divergingMaps =
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
const std::vector<ColorMapName> diverging_blackMaps =
{
    ColorMapName::pink_black_green_w3c_,
    ColorMapName::red_black_blue,
    ColorMapName::red_black_green,
    ColorMapName::red_black_orange,
    ColorMapName::red_black_sky,
    ColorMapName::yellow_black_blue,
    ColorMapName::yellow_black_sky,
};
const std::vector<ColorMapName> qualitativeMaps =
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
const std::vector<ColorMapName> miscMaps =
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
const std::vector<ColorMapName> cyclicMaps =
{
    ColorMapName::hsv,
    ColorMapName::twilight,
    ColorMapName::twilight_shifted,
};
// clang-format on

} // namespace colordata
