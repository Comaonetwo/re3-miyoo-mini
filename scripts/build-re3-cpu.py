#!/usr/bin/env python3
"""Build linked re3 CPU kernel for Miyoo; pinned dependencies and private data separate."""
import pathlib,argparse,subprocess,os,shlex
p=argparse.ArgumentParser();p.add_argument('--zig',required=True,type=pathlib.Path);p.add_argument('--cmake',default='cmake');p.add_argument('--re3-cpu',required=True,type=pathlib.Path);p.add_argument('--librw',required=True,type=pathlib.Path);p.add_argument('--output',required=True,type=pathlib.Path);p.add_argument('--pixel-stats',choices=['on','off'],default='off',help='Detailed pixel diagnostics (default off for console; core counters/checks always on)');a=p.parse_args()
project=pathlib.Path(__file__).resolve().parents[1];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);zig=a.zig.resolve()
for name,action in [('zig-cc','cc'),('zig-cxx','c++'),('zig-ar','ar'),('zig-ranlib','ranlib')]:
 f=out/name;f.write_text('#!/bin/sh\nexec '+shlex.quote(str(zig))+' '+action+' "$@"\n');f.chmod(0o755)
toolchain=out/'toolchain.cmake';toolchain.write_text('set(CMAKE_SYSTEM_NAME Linux)\nset(CMAKE_SYSTEM_PROCESSOR arm)\nset(CMAKE_C_COMPILER "'+str(out/'zig-cc')+'")\nset(CMAKE_C_FLAGS_INIT "-target arm-linux-musleabihf -mcpu=cortex_a7")\nset(CMAKE_CXX_COMPILER "'+str(out/'zig-cxx')+'")\nset(CMAKE_AR "'+str(out/'zig-ar')+'")\nset(CMAKE_RANLIB "'+str(out/'zig-ranlib')+'")\nset(CMAKE_CXX_FLAGS_INIT "-target arm-linux-musleabihf -mcpu=cortex_a7 -ffunction-sections -fdata-sections")\nset(CMAKE_EXE_LINKER_FLAGS_INIT "-static -Wl,--gc-sections -Wl,--strip-debug")\n')
env=os.environ.copy();env.setdefault('ZIG_GLOBAL_CACHE_DIR',str(out/'zig-cache'))
subprocess.run([a.cmake,'-S',str(project/'cmake/re3-cpu'),'-B',str(out/'build'),'-DCMAKE_TOOLCHAIN_FILE='+str(toolchain),'-DCMAKE_BUILD_TYPE=Release','-DSOFT_PIXEL_STATS='+a.pixel_stats.upper(),'-DRE3_CPU_SOURCE='+str(a.re3_cpu.resolve()),'-DLIBRW_SOURCE='+str(a.librw.resolve())],env=env,check=True)
subprocess.run([a.cmake,'--build',str(out/'build'),'--target','re3_cpu','-j','4'],env=env,check=True)
print('Linked ARM kernel:',out/'build/re3_cpu')

subprocess.run([str(zig),'cc','-target','arm-linux-gnueabihf.2.4','-mcpu=cortex_a7','-O2','-s',str(project/'src/re3_audio_helper.c'),'-o',str(out/'build/re3_audio'),'-ldl'],env=env,check=True)
print('Dynamic ARM audio helper (console SDL, glibc 2.4):',out/'build/re3_audio')
