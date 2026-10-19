#!/usr/bin/env python3
"""Static Miyoo build with real pinned librw CPU callbacks; no proprietary resources."""
import argparse, pathlib, re, subprocess, os
p=argparse.ArgumentParser();p.add_argument('--zig',required=True,type=pathlib.Path);p.add_argument('--librw',required=True,type=pathlib.Path)
p.add_argument('--re3',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path)
a=p.parse_args();project=pathlib.Path(__file__).resolve().parents[1];rw=a.librw.resolve();zig=a.zig.resolve()
output=(a.output or project/'build-arm-device').resolve();output.mkdir(parents=True,exist_ok=True)
commit=subprocess.check_output(['git','-C',str(rw),'rev-parse','HEAD'],text=True).strip()
if commit!='5501c4fdc7425ff926be59369a13593bb6c81b54':raise ValueError('wrong librw revision')
env=os.environ.copy();env.setdefault('ZIG_GLOBAL_CACHE_DIR',str(output/'zig-cache'))
sources=re.findall(r'^\s+([a-zA-Z0-9_/.-]+\.cpp)\s*$',(rw/'src/CMakeLists.txt').read_text(),re.M)
if len(sources)<50:raise ValueError('unexpected librw source list')
core=['soft_renderer','screen_renderer','shaded_renderer','scene','asset_scene','texture_renderer','collision_world','driving','checkpoint_run','overlay_text','software_device','rw_asset_scene','fb_blit','linux_main']
flags=[str(zig),'c++','-target','arm-linux-musleabihf','-mcpu=cortex_a7','-static','-std=c++11','-O2','-w','-pthread',
 '-DRW_NULL','-DSOFT_RW_DEVICE','-DNDEBUG','-DLODEPNG_NO_COMPILE_CPP','-ffunction-sections','-fdata-sections','-I'+str(rw),'-I'+str(project/'src')]
subprocess.run(flags+['-Wl,--strip-debug','-Wl,--gc-sections']+[str(project/'src'/(s+'.cpp')) for s in core]+[str(rw/'src'/s) for s in sources]+['-o',str(output/'gta3_soft_fb')],env=env,check=True)
if a.re3:
 subprocess.run(flags+['-DLIBRW','-I'+str(a.re3.resolve()/'src/fakerw'),'-c',str(project/'tests/re3_vertex_compile.cpp'),'-o',str(output/'re3_vertex_compile.o')],env=env,check=True)
 subprocess.run(flags+['-DLIBRW','-I'+str(a.re3.resolve()/'src/fakerw'),'-I'+str(a.re3.resolve()/'src/skel'),'-include',str(project/'src/re3_soft_compat.hpp'),'-c',str(a.re3.resolve()/'src/fakerw/fake.cpp'),'-o',str(output/'re3_fake_compile.o')],env=env,check=True)
subprocess.run(['file',str(output/'gta3_soft_fb')],check=True)
print('Static ARM CPU Device and pinned re3 vertex API built; resources not included')
