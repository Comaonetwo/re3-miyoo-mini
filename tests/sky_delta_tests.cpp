#include "sky_delta.hpp"
#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
static uint16_t reference(uint16_t p,uint32_t d){int r=(p>>11)+int(d&63)-31,g=((p>>5)&63)+int((d>>6)&127)-63,b=(p&31)+int((d>>13)&63)-31;return uint16_t((std::max(0,std::min(31,r))<<11)|(std::max(0,std::min(63,g))<<5)|std::max(0,std::min(31,b)));}
int main(){std::mt19937 rng(68054);uint64_t count=0;std::vector<uint16_t>a(65536);std::vector<uint32_t>d(65536);
 for(unsigned pattern=0;pattern<64;++pattern){unsigned dr=pattern==0?31:pattern==1?0:pattern==2?62:rng()%63,dg=pattern==0?63:pattern==1?0:pattern==2?126:rng()%127,db=pattern==0?31:pattern==1?0:pattern==2?62:rng()%63;uint32_t delta=dr|(dg<<6)|(db<<13);for(unsigned n=0;n<65536;++n){a[n]=n;d[n]=delta;}skyDeltaRow(a.data(),d.data(),65536);for(unsigned n=0;n<65536;++n){assert(a[n]==reference(n,delta));++count;}}
 for(unsigned offset=0;offset<8;++offset)for(unsigned n=0;n<81;++n){std::vector<uint16_t>x(n+24,0xacbd);std::vector<uint32_t>delta(n+16);for(unsigned k=0;k<n;++k){x[k+offset]=rng();delta[k+offset]=rng()%63|((rng()%127)<<6)|((rng()%63)<<13);}auto old=x;skyDeltaRow(x.data()+offset,delta.data()+offset,n);for(unsigned k=0;k<x.size();++k){assert(x[k]==(k>=offset&&k<offset+n?reference(old[k],delta[k]):old[k]));++count;}}
 unsigned cases=0;for(unsigned scale:{1u,2u,3u,4u})for(unsigned target:{120u,160u})for(unsigned blend:{0u,1u,2u,3u}){Renderer fast,slow;fast.skyCompositeEnabled(true);slow.skyCompositeEnabled(false);for(auto q:{&fast,&slow}){q->renderScale(scale);q->clear(17291);}
 bool f=fast.beginSky(target),s=slow.beginSky(target);assert(f==s);if(!f)continue;ScreenVertex v[3]={{-3.3f,7.7f,1,0,0,17,253,139,129,0},{293.7f,18.3f,1,0,0,239,7,39,211,0},{67.1f,237.9f,1,0,0,93,127,253,43,0}};ScreenState state;state.blend=blend;state.zTest=state.zWrite=false;state.screenDepth=true;
 fast.screenTriangle(v,nullptr,state);slow.screenTriangle(v,nullptr,state);fast.endSky();slow.endSky();assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());assert(fast.stats().pixels==slow.stats().pixels);assert(fast.stats().skyCompositePixels==slow.stats().skyCompositePixels);assert(fast.stats().skyVectorPixels);++cases;}
 std::printf("PASS sky delta %llu exact RGB565/offset/tail/guard comparisons and %u full compositor states RGB/depth/fragments/counts\n",(unsigned long long)count,cases);
}
