from colormap import Colormap
from numpy.core import ushort

INCLUDE_RELDIR = 'colordata'
CPP_LIBNAME = 'goomutils'
FINAL_INCLUDE_RELDIR = f'../include/{CPP_LIBNAME}/{INCLUDE_RELDIR}'
INCLUDE_DIR = '/tmp/' + INCLUDE_RELDIR

COLOR_MAP_ENUM_H = 'colormap_enums.h'
ALL_MAPS_H = 'all_maps.h'
ALL_MAPS_CPP = 'all_maps.cpp'
COLORDATA_NAMESPACE = 'colordata'
MAPS_ENUM_NAME = 'ColorMapName'


def get_cpp_name(nm):
    cpp_nm = nm.replace('(', '_')
    cpp_nm = cpp_nm.replace(')', '_')
    return cpp_nm


def write_cpp_headers(maps):
    used = []
    for m in maps:
        cmap = colormap.cmap(m)
        if m.endswith('_r'):
            continue

        used.append(m)
        cmap.name = m

        write_cpp_header(cmap)

    return used


def write_cpp_header(cm):
    with open(f'{INCLUDE_DIR}/{get_cpp_name(cm.name)}.h', 'w') as f:
        f.write('#pragma once\n')
        f.write('\n')
        f.write('#include "vivid/types.h"\n')
        f.write('#include <vector>\n')
        f.write('\n')
        f.write(f'namespace {COLORDATA_NAMESPACE} {{\n')
        f.write('\n')
        f.write(f'  static const std::vector<vivid::srgb_t> {get_cpp_name(cm.name)} = {{\n')
        for i in range(cm.N):
            vals = cm(i)
            f.write(f'    {{ {vals[0]:9.5f}f, {vals[1]:9.5f}f, {vals[2]:9.5f}f }},\n')
        f.write('  };\n')
        f.write('\n')
        f.write(f'}} // {COLORDATA_NAMESPACE}\n')


def write_colormaps_enums_header(maps):
    with open(f'{INCLUDE_DIR}/{COLOR_MAP_ENUM_H}', 'w') as f:
        f.write('#pragma once\n')
        f.write('\n')
        f.write(f'enum class {MAPS_ENUM_NAME} {{\n')
        for m in maps:
            f.write(f'  {get_cpp_name(m)},\n')
        f.write(f'}};\n')


def write_all_maps_header(color_map_grps, num_maps):
    with open(f'{INCLUDE_DIR}/{ALL_MAPS_H}', 'w') as f:
        f.write('#pragma once\n')
        f.write('\n')
        f.write(f'#include "{CPP_LIBNAME}/{COLOR_MAP_ENUM_H}"\n')
        f.write('\n')
        f.write('#include "vivid/types.h"\n')
        f.write('\n')
        f.write('#include <array>\n')
        f.write('#include <string>\n')
        f.write('#include <utility>\n')
        f.write('#include <vector>\n')
        f.write('\n')
        f.write(f'namespace {COLORDATA_NAMESPACE} {{\n')
        f.write('\n')
        f.write(f'  // array of raw maps matching elements of enum \'{MAPS_ENUM_NAME}\'\n')
        f.write(f'  extern const std::array<std::pair<std::string, std::vector<vivid::srgb_t>>, {num_maps}> allMaps;\n')
        f.write('\n')
        for grp in color_map_grps:
            f.write(f'  extern const std::vector<{MAPS_ENUM_NAME}> {grp}Maps;\n')
        f.write('\n')
        f.write(f'}} // {COLORDATA_NAMESPACE}\n')


def write_all_maps_cpp(color_map_grps, used_maps):
    with open(f'{INCLUDE_DIR}/{ALL_MAPS_CPP}', 'w') as f:
        f.write(f'#include "{FINAL_INCLUDE_RELDIR}/{ALL_MAPS_H}"\n')
        f.write('\n')
        f.write(f'#include "{CPP_LIBNAME}/{COLOR_MAP_ENUM_H}"\n')
        f.write('\n')

        for m in used_maps:
            f.write(f'#include "{FINAL_INCLUDE_RELDIR}/{get_cpp_name(m)}.h"\n')
        f.write('\n')
        f.write('#include "vivid/types.h"\n')
        f.write('\n')
        f.write('#include <array>\n')
        f.write('#include <string>\n')
        f.write('#include <utility>\n')
        f.write('#include <vector>\n')
        f.write('\n')
        f.write(f'namespace {COLORDATA_NAMESPACE} {{\n')
        f.write('\n')
        f.write(f'  const std::array<std::pair<std::string, std::vector<vivid::srgb_t>>, {len(used_maps)}> allMaps = {{\n')
        for m in used_maps:
            f.write(f'    std::make_pair("{get_cpp_name(m)}", {COLORDATA_NAMESPACE}::{get_cpp_name(m)}),\n')
        f.write(f'  }};\n')
        f.write('\n')

        # Do groups
        for map_nm in color_map_grps:
            f.write(f'  const std::vector<{MAPS_ENUM_NAME}> {get_cpp_name(map_nm)}Maps = {{\n')
            for m in color_map_grps[map_nm]:
                f.write(f'    {MAPS_ENUM_NAME}::{get_cpp_name(m)},\n')
                if m in used_maps:
                    used_maps.remove(m)
            f.write(f'  }};\n')

        if len(used_maps) > 0:
            print('WARNING: There were uncategorised maps: "{}".'.format(','.join(used_maps)))

        f.write('\n')
        f.write(f'}} // {COLORDATA_NAMESPACE}\n')


if __name__ == '__main__':
    colormap = Colormap()

    color_map_groups = {
            'perc_unif_sequential': [],
            'sequential': [c for c in colormap.sequentials],
            'sequential2': [c for c in colormap.sequentials2],
            'diverging': [c for c in colormap.diverging],
            'diverging_black': [c for c in colormap.diverging_black],
            'qualitative': [c for c in colormap.qualitative],
            'misc': [c for c in colormap.misc],
            'cyclic': [],
    }

    color_map_groups['perc_unif_sequential'].extend(['viridis', 'plasma', 'inferno', 'magma', 'cividis'])
    color_map_groups['sequential2'].extend(['binary', 'gist_gray', 'gist_yarg', 'Wistia'])
    color_map_groups['qualitative'].extend(['tab10', 'tab20', 'tab20b', 'tab20c'])
    color_map_groups['cyclic'].extend(['twilight', 'twilight_shifted', 'hsv'])

    for key in color_map_groups:
        color_map_groups[key] = sorted(color_map_groups[key], key=str.casefold)

    all_maps = [c for c in colormap.colormaps]
    for key in color_map_groups:
        all_maps.extend(color_map_groups[key])

    all_maps = sorted(list(set(all_maps)), key=str.casefold)

    the_used_maps = write_cpp_headers(all_maps)
    write_colormaps_enums_header(the_used_maps)

    write_all_maps_header(color_map_groups, len(the_used_maps))
    write_all_maps_cpp(color_map_groups, the_used_maps)