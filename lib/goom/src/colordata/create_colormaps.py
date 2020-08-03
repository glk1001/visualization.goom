from colormap import Colormap, cmap_builder

cm = Colormap()

mycmap = cm.cmap('red_black_sky')
mycmap.name = 'red_black_sky'
#print(f'name: {mycmap.name}')
#for i in range(2):
#  print(mycmap(i)) 

include_reldir = 'colordata'
include_dir = '/tmp/' + include_reldir
namespace = 'colordata'

all_maps = [c for c in cm.colormaps]
all_maps.extend([c for c in cm.sequentials])
all_maps.extend([c for c in cm.sequentials2])
all_maps.extend([c for c in cm.diverging])
all_maps.extend([c for c in cm.diverging_black])
all_maps.extend([c for c in cm.qualitative])
all_maps.extend([c for c in cm.misc])

all_maps = sorted(list(set(all_maps)))

used_maps = []

for c in all_maps:
  mcmap = cm.cmap(c)
  if c.endswith('_r'):
    continue
  used_maps.append(c)  
  mcmap.name = c
  with open(f'{include_dir}/{c}.h', 'w') as f:
    f.write('#pragma once\n') 
    f.write('\n')
    f.write('#include "vivid/types.h"\n') 
    f.write('#include <vector>\n') 
    f.write('\n')
    f.write(f'namespace {namespace} {{\n') 
    f.write('\n')
    f.write(f'  static const std::vector<vivid::srgb_t> {mcmap.name} = {{\n')
    for i in range(256):
        vals = mcmap(i)
        f.write(f'    {{ {vals[0]:9.5f}f, {vals[1]:9.5f}f, {vals[2]:9.5f}f }},\n')
    f.write('  };\n')
    f.write('\n')
    f.write(f'}} // {namespace}\n')

with open(f'{include_dir}/all_maps.h', 'w') as f:
    f.write('#pragma once\n') 
    f.write('\n')
    f.write('#include "vivid/types.h"\n') 
    f.write('\n')
    f.write('#include <map>\n') 
    f.write('#include <string>\n') 
    f.write('#include <vector>\n') 
    f.write('\n')
    f.write(f'namespace {namespace} {{\n') 
    f.write('\n')
    f.write(f'  extern const std::map<std::string, std::vector<vivid::srgb_t>> allMaps;\n') 
    f.write('\n')
    f.write(f'  extern const std::vector<std::string> sequentialsMaps;\n') 
    f.write(f'  extern const std::vector<std::string> sequentials2Maps;\n') 
    f.write(f'  extern const std::vector<std::string> divergingMaps;\n') 
    f.write(f'  extern const std::vector<std::string> diverging_blackMaps;\n') 
    f.write(f'  extern const std::vector<std::string> qualitativeMaps;\n') 
    f.write(f'  extern const std::vector<std::string> miscMaps;\n') 
    f.write(f'  extern const std::vector<std::string> ungroupedMaps;\n') 
    f.write('\n')
    f.write(f'}} // {namespace}\n')

with open(f'{include_dir}/all_maps.cpp', 'w') as f:
    f.write('#include "all_maps.h"\n') 
    f.write('\n')
    
    for c in used_maps:
        f.write(f'#include "{include_reldir}/{c}.h"\n') 
    f.write('\n')
    f.write('#include "vivid/types.h"\n') 
    f.write('\n')
    f.write('#include <map>\n') 
    f.write('#include <string>\n') 
    f.write('#include <vector>\n') 
    f.write('\n')
    f.write(f'namespace {namespace} {{\n') 
    f.write('\n')
    f.write(f'  const std::map<std::string, std::vector<vivid::srgb_t>> allMaps = {{\n') 
    for c in used_maps:
        f.write(f'    {{ "{c}", {namespace}::{c} }},\n') 
    f.write(f'  }};\n') 
    f.write('\n')

     # Do groups
    f.write(f'  const std::vector<std::string> sequentialsMaps = {{\n') 
    for c in cm.sequentials:
        f.write(f'    "{c}",\n') 
        if c in used_maps:
            used_maps.remove(c)
    f.write(f'  }};\n') 
    f.write(f'  const std::vector<std::string> sequentials2Maps = {{\n') 
    for c in cm.sequentials2:
        f.write(f'    "{c}",\n') 
        if c in used_maps:
            used_maps.remove(c)
    f.write(f'  }};\n') 
    f.write(f'  const std::vector<std::string> divergingMaps = {{\n') 
    for c in cm.diverging:
        f.write(f'    "{c}",\n') 
        if c in used_maps:
            used_maps.remove(c)
    f.write(f'  }};\n') 
    f.write(f'  const std::vector<std::string> diverging_blackMaps = {{\n') 
    for c in cm.diverging_black:
        f.write(f'    "{c}",\n') 
        if c in used_maps:
            used_maps.remove(c)
    f.write(f'  }};\n') 
    f.write(f'  const std::vector<std::string> qualitativeMaps = {{\n') 
    for c in cm.qualitative:
        f.write(f'    "{c}",\n') 
        if c in used_maps:
            used_maps.remove(c)
    f.write(f'  }};\n') 
    f.write(f'  const std::vector<std::string> miscMaps = {{\n') 
    for c in cm.misc:
        f.write(f'    "{c}",\n') 
        if c in used_maps:
            used_maps.remove(c)
    f.write(f'  }};\n') 
    f.write(f'  const std::vector<std::string> ungroupedMaps = {{\n') 
    for c in used_maps:
        f.write(f'    "{c}",\n') 
    f.write(f'  }};\n') 
      
    f.write('\n')
    f.write(f'}} // {namespace}\n')
