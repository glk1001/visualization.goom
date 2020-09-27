#include "colordata/all_maps.h"

// TODO -  temporary - fix generator to add namespace.
namespace goom::utils
{

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
} // namespace goom::utils

#include "colormap_enums.h"
#include "vivid/types.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace goom::utils::colordata
{

const std::array<std::pair<std::string, std::vector<vivid::srgb_t>>, 89> allMaps = {
    std::make_pair("Accent", Accent),
    std::make_pair("afmhot", afmhot),
    std::make_pair("autumn", autumn),
    std::make_pair("binary", binary),
    std::make_pair("Blues", Blues),
    std::make_pair("bone", bone),
    std::make_pair("BrBG", BrBG),
    std::make_pair("brg", brg),
    std::make_pair("BuGn", BuGn),
    std::make_pair("BuPu", BuPu),
    std::make_pair("bwr", bwr),
    std::make_pair("cividis", cividis),
    std::make_pair("CMRmap", CMRmap),
    std::make_pair("cool", cool),
    std::make_pair("coolwarm", coolwarm),
    std::make_pair("copper", copper),
    std::make_pair("cubehelix", cubehelix),
    std::make_pair("Dark2", Dark2),
    std::make_pair("flag", flag),
    std::make_pair("gist_earth", gist_earth),
    std::make_pair("gist_gray", gist_gray),
    std::make_pair("gist_heat", gist_heat),
    std::make_pair("gist_ncar", gist_ncar),
    std::make_pair("gist_rainbow", gist_rainbow),
    std::make_pair("gist_stern", gist_stern),
    std::make_pair("gist_yarg", gist_yarg),
    std::make_pair("GnBu", GnBu),
    std::make_pair("gnuplot", gnuplot),
    std::make_pair("gnuplot2", gnuplot2),
    std::make_pair("gray", gray),
    std::make_pair("Greens", Greens),
    std::make_pair("Greys", Greys),
    std::make_pair("hot", hot),
    std::make_pair("hsv", hsv),
    std::make_pair("inferno", inferno),
    std::make_pair("jet", jet),
    std::make_pair("magma", magma),
    std::make_pair("nipy_spectral", nipy_spectral),
    std::make_pair("ocean", ocean),
    std::make_pair("Oranges", Oranges),
    std::make_pair("OrRd", OrRd),
    std::make_pair("Paired", Paired),
    std::make_pair("Pastel1", Pastel1),
    std::make_pair("Pastel2", Pastel2),
    std::make_pair("pink", pink),
    std::make_pair("pink_black_green_w3c_", pink_black_green_w3c_),
    std::make_pair("PiYG", PiYG),
    std::make_pair("plasma", plasma),
    std::make_pair("PRGn", PRGn),
    std::make_pair("prism", prism),
    std::make_pair("PuBu", PuBu),
    std::make_pair("PuBuGn", PuBuGn),
    std::make_pair("PuOr", PuOr),
    std::make_pair("PuRd", PuRd),
    std::make_pair("Purples", Purples),
    std::make_pair("rainbow", rainbow),
    std::make_pair("RdBu", RdBu),
    std::make_pair("RdGy", RdGy),
    std::make_pair("RdPu", RdPu),
    std::make_pair("RdYlBu", RdYlBu),
    std::make_pair("RdYlGn", RdYlGn),
    std::make_pair("red_black_blue", red_black_blue),
    std::make_pair("red_black_green", red_black_green),
    std::make_pair("red_black_orange", red_black_orange),
    std::make_pair("red_black_sky", red_black_sky),
    std::make_pair("Reds", Reds),
    std::make_pair("seismic", seismic),
    std::make_pair("Set1", Set1),
    std::make_pair("Set2", Set2),
    std::make_pair("Set3", Set3),
    std::make_pair("Spectral", Spectral),
    std::make_pair("spring", spring),
    std::make_pair("summer", summer),
    std::make_pair("tab10", tab10),
    std::make_pair("tab20", tab20),
    std::make_pair("tab20b", tab20b),
    std::make_pair("tab20c", tab20c),
    std::make_pair("terrain", terrain),
    std::make_pair("twilight", twilight),
    std::make_pair("twilight_shifted", twilight_shifted),
    std::make_pair("viridis", viridis),
    std::make_pair("winter", winter),
    std::make_pair("Wistia", Wistia),
    std::make_pair("yellow_black_blue", yellow_black_blue),
    std::make_pair("yellow_black_sky", yellow_black_sky),
    std::make_pair("YlGn", YlGn),
    std::make_pair("YlGnBu", YlGnBu),
    std::make_pair("YlOrBr", YlOrBr),
    std::make_pair("YlOrRd", YlOrRd),
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

} // namespace goom::utils::colordata
