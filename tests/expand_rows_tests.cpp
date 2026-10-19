#include "expand_rows.hpp"
#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
int main(){std::mt19937 rng(69054);uint64_t pixels=0;unsigned cases=0;
 for(unsigned sw:{1u,80u,120u,160u,240u,320u,640u})for(unsigned sh:{1u,3u,60u,90u,120u,180u,240u})for(unsigned dw:{1u,80u,160u,240u,320u,640u}){unsigned dh=dw==1?7:dw*3/4,ss=sw+3,ds=dw+5;std::vector<uint16_t>src(ss*sh+24),a(ds*dh+24,0xabcd);for(auto&v:src)v=rng();auto expected=a;for(unsigned y=0;y<dh;++y)for(unsigned x=0;x<dw;++x){expected[3+y*ds+x]=src[3+(y*sh/dh)*ss+x*sw/dw];++pixels;}expandRows565(a.data()+3,ds,dw,dh,src.data()+3,ss,sw,sh);assert(a==expected);++cases;}
 unsigned integrated=0;
 for(unsigned scale:{1u,2u,3u,4u})for(unsigned mode=0;mode<3;++mode){Renderer fast,slow;fast.rowExpansionEnabled(true);slow.rowExpansionEnabled(false);for(auto q:{&fast,&slow}){q->renderScale(scale);q->clear(13291);ScreenVertex v[3]={{-5.3f,13.7f,1,0,0,19,237,91,255,0},{317.1f,31.3f,1,0,0,213,13,253,255,0},{73.9f,247.1f,1,0,0,93,171,7,255,0}};ScreenState s;s.zTest=false;s.zWrite=true;s.screenDepth=true;q->screenTriangle(v,nullptr,s);if(mode==1)q->beginUi(true);if(mode==2)q->resolveNativeUi();}
 assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());assert(fast.stats().pixels==slow.stats().pixels);if(scale!=1||mode)assert(fast.stats().rowExpandPixels);++integrated;}
 std::printf("PASS row expansion %u dimension/stride/guard cases, %llu reference pixels, %u renderer states RGB/depth/fragments\n",cases,(unsigned long long)pixels,integrated);
}
