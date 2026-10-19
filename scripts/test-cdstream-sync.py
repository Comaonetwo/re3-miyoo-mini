#!/usr/bin/env python3
"""Compile the actual prepared POSIX streamer against synthetic files and minimal game stubs."""
from pathlib import Path
import argparse,subprocess,sys,shutil
from cdstream_patch import patch_cdstream
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);p.add_argument('compiler');p.add_argument('--sanitize',action='store_true');p.add_argument('--tsan',action='store_true');a=p.parse_args();root=Path(__file__).resolve().parents[1];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
def replace(s,old,new):assert s.count(old)==1;return s.replace(old,new)
stream=patch_cdstream((a.source/'src/core/CdStreamPosix.cpp').read_text(),replace);(out/'stream.cpp').write_text(stream);shutil.copy2(a.source/'src/core/CdStream.h',out/'CdStream.h')
(out/'common.h').write_text('''#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cassert>
#include <algorithm>
using uint32=uint32_t;using int32=int32_t;using uint8=uint8_t;using int8=int8_t;using RwUInt32=uint32_t;
#define nil nullptr
#define ASSERT(x) assert(x)
#define VALIDATE_SIZE(a,b)
#define debug(...) ((void)0)
#define MAX_CDCHANNELS 5
#define MAX_CDIMAGES 50
#define MIYOO_CPU
#define FLUSHABLE_STREAMING
#ifdef __linux__
#define USE_UNNAMED_SEM
#endif
''')
(out/'crossplatform.h').write_text('#pragma once\ninline char* casepath(const char* p,bool){return strdup(p);}\n')
(out/'rwcore.h').write_text('#pragma once\ninline void* RwMallocAlign(size_t n,unsigned a){void* p=nullptr;return posix_memalign(&p,a,n)?nullptr:p;}\ninline void RwFreeAlign(void* p){free(p);}\n')
(out/'MemoryMgr.h').write_text('#pragma once\n')
(out/'main.cpp').write_text('''#include "common.h"
#include "CdStream.h"
#include <vector>
#include <thread>
#include <chrono>
#include <sys/stat.h>
static std::vector<uint8> data[2];
int main(){mkdir("models",0755);
 for(unsigned image=0;image<2;++image){data[image].resize(512*CDSTREAM_SECTOR_SIZE);for(size_t i=0;i<data[image].size();++i)data[image][i]=uint8((i*37)^(i>>9)^(image*91));FILE* f=fopen(image?"models/other.img":"models/gta3.img","wb");assert(f);assert(fwrite(data[image].data(),1,data[image].size(),f)==data[image].size());fclose(f);}
 unsigned reads=0;
 for(unsigned cycle=0;cycle<8;++cycle){CdStreamInit(2);assert(CdStreamAddImage("models/gta3.img"));assert(CdStreamAddImage("models/other.img"));assert(GetGTA3ImgSize()==data[0].size());
  std::vector<uint8> buffers[2];for(auto& b:buffers)b.resize(4*CDSTREAM_SECTOR_SIZE);
  for(unsigned i=0;i<1200;++i){unsigned offset[2],count[2],image[2];
   for(unsigned ch=0;ch<2;++ch){offset[ch]=(i*17+ch*89)%508;count[ch]=1+i%4;image[ch]=(i+ch)%2;assert(CdStreamRead(ch,buffers[ch].data(),(image[ch]<<24)|offset[ch],count[ch])==STREAM_SUCCESS);++reads;if(i%11==0)assert(CdStreamRead(ch,buffers[ch].data(),(image[ch]<<24)|offset[ch],count[ch])==STREAM_SUCCESS);}
   if(i%3==0)std::this_thread::yield();if(i%7==0)flushStream[i%2]=true;
   if(i%31==0){int status[2];std::thread one([&]{status[0]=CdStreamSync(0);}),two([&]{status[1]=CdStreamSync(1);});one.join();two.join();assert(!status[0]&&!status[1]);}
   else{for(unsigned n=0;n<2;++n){unsigned ch=(i+n)%2;int status=CdStreamGetStatus(ch);assert(status==STREAM_WAITING||status==STREAM_READING||status==STREAM_NONE);assert(CdStreamSync(ch)==STREAM_NONE);}}
   for(unsigned ch=0;ch<2;++ch)assert(memcmp(buffers[ch].data(),data[image[ch]].data()+offset[ch]*CDSTREAM_SECTOR_SIZE,count[ch]*CDSTREAM_SECTOR_SIZE)==0);
  }
  // Replacing a busy request waits before buffer reuse; exact new bytes win.
  for(unsigned i=0;i<100;++i){assert(CdStreamRead(0,buffers[0].data(),10,4)==STREAM_SUCCESS);assert(CdStreamRead(0,buffers[0].data(),20,4)==STREAM_SUCCESS);assert(CdStreamSync(0)==STREAM_NONE);assert(memcmp(buffers[0].data(),data[0].data()+20*CDSTREAM_SECTOR_SIZE,4*CDSTREAM_SECTOR_SIZE)==0);reads+=2;}
  assert(CdStreamRead(0,buffers[0].data(),510,4)==STREAM_SUCCESS);assert(CdStreamSync(0)==STREAM_ERROR);assert(CdStreamGetStatus(0)==STREAM_ERROR);assert(CdStreamGetStatus(0)==STREAM_NONE);
  assert(CdStreamRead(0,buffers[0].data(),1,0)==STREAM_SUCCESS);assert(CdStreamSync(0)==STREAM_NONE);CdStreamRemoveImages();CdStreamShutdown();
 }
 printf("PASS actual CPU CdStream: %u checked reads, dual waiters, pending replacement, flush/buffer ownership, EOF status and 8 shutdown/reinitialization cycles\\n",reads);
}
''')
cmd=[a.compiler,'-std=c++11','-O1','-g','-pthread','-I'+str(out),'-I'+str(root/'src'),str(out/'stream.cpp'),str(out/'main.cpp'),'-o',str(out/'test')]
if a.sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
if a.tsan:cmd+=['-fsanitize=thread','-fno-omit-frame-pointer']
c=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
if c.returncode:sys.stdout.write(c.stdout.decode());sys.exit(c.returncode)
r=subprocess.run([str(out/'test')],cwd=out,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=45);sys.stdout.write(r.stdout.decode());sys.exit(r.returncode)
