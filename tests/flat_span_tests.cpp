#include "flat_span.hpp"
#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
int main(){std::mt19937 r(65053);uint64_t tested=0;
 for(unsigned alpha:{0u,1u,2u,31u,63u,127u,128u,192u,254u,255u})for(unsigned pattern=0;pattern<8;++pattern){unsigned red=pattern==0?0:pattern==1?255:r()%256,green=pattern==0?0:pattern==1?255:r()%256,blue=pattern==0?0:pattern==1?255:r()%256;FlatBlend table(red,green,blue,alpha,1);std::vector<uint16_t> image(65536);for(unsigned n=0;n<65536;++n)image[n]=uint16_t(n);flatSourceOver(image.data(),unsigned(image.size()),red,green,blue,alpha,table);for(unsigned n=0;n<65536;++n){assert(image[n]==table(uint16_t(n)));++tested;}}
 for(unsigned offset=0;offset<8;++offset)for(unsigned count=0;count<81;++count){unsigned red=r()%256,green=r()%256,blue=r()%256,alpha=r()%256;FlatBlend table(red,green,blue,alpha,1);std::vector<uint16_t>a(count+24,0xabcd);for(unsigned n=0;n<count;++n)a[offset+n]=uint16_t(r());auto b=a;for(unsigned n=0;n<count;++n)b[offset+n]=table(b[offset+n]);flatSourceOver(a.data()+offset,count,red,green,blue,alpha,table);assert(a==b);}
 uint64_t bulk=0;unsigned cases=0;
 for(unsigned scale:{1u,2u,3u,4u}){Renderer fast,slow;fast.flatSpansEnabled(true);slow.flatSpansEnabled(false);for(auto q:{&fast,&slow}){q->renderScale(scale);q->clear(9321);}
  for(unsigned n=0;n<80;++n){ScreenVertex v[3]={{-13.37f,-5.19f,1,0,0,0,0,0,0,.5f},{339.73f,14.91f,1,0,0,0,0,0,0,.5f},{23.41f,263.87f,1,0,0,0,0,0,0,.5f}};for(auto& q:v){q.r=r()%256;q.g=r()%256;q.b=r()%256;q.a=r()%256;}for(unsigned j=1;j<3;++j){v[j].r=v[0].r;v[j].g=v[0].g;v[j].b=v[0].b;v[j].a=v[0].a;}
   if(n%5==0)for(auto& q:v)q.a=n%2?255:0;
   ScreenState s;s.blend=n%6;s.screenDepth=true;s.alphaRef=n%3?0:120;s.zTest=n%11==0;s.zWrite=n%13==0;
   fast.screenTriangle(v,nullptr,s);slow.screenTriangle(v,nullptr,s);assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());assert(fast.stats().pixels==slow.stats().pixels);++cases;
  }bulk+=fast.stats().flatSpanPixels;
 }
 assert(bulk>0);std::printf("PASS flat spans: %llu exact component-table pixels, 648 alignment/tail/guard cases, %u shader cases RGB/depth/fragments; bulk=%llu\n",(unsigned long long)tested,cases,(unsigned long long)bulk);
}
