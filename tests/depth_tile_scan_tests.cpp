#include "depth_tile_scan.hpp"
#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
static float reference(const float* p,unsigned stride,unsigned w,unsigned h){float out=std::numeric_limits<float>::infinity();for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)out=std::min(out,p[y*stride+x]);return out;}
int main(){std::mt19937 rng(78055);std::vector<float> data(24*24);uint64_t samples=0;unsigned cases=0;
 for(unsigned test=0;test<65536;++test){for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x){uint32_t bits=test%2?rng():rng()%0x7f800001u;std::memcpy(&data[3+y*19+x],&bits,4);}float expected=reference(data.data()+3,19,8,8),actual=depthTileMinimum(data.data()+3,19,8,8);assert(!std::memcmp(&actual,&expected,4));samples+=64;++cases;}
 for(unsigned w=1;w<=8;++w)for(unsigned h=1;h<=8;++h)for(unsigned offset=0;offset<8;++offset){for(auto&v:data){uint32_t bits=rng();std::memcpy(&v,&bits,4);}float expected=reference(data.data()+offset,19,w,h),actual=depthTileMinimum(data.data()+offset,19,w,h);assert(!std::memcmp(&actual,&expected,4));samples+=w*h;++cases;}
 for(uint32_t bits:{0u,0x80000000u,1u,0x7f800000u,0xff800000u,0x7fc00001u,0x7f800001u,0xffc00000u})for(unsigned index=0;index<64;++index){std::fill(data.begin(),data.end(),std::numeric_limits<float>::infinity());std::memcpy(&data[(index/8)*19+index%8],&bits,4);float expected=reference(data.data(),19,8,8),actual=depthTileMinimum(data.data(),19,8,8);assert(!std::memcmp(&actual,&expected,4));samples+=64;++cases;}
 uint64_t scanned=0;for(unsigned scale:{1u,2u,3u,4u}){Renderer fast,slow;fast.tileScanEnabled(true);slow.tileScanEnabled(false);for(auto q:{&fast,&slow}){q->renderScale(scale);q->clear(17291);q->earlyDepthEnabled(false);q->smallDepthEnabled(false);q->lazyDepthEnabled(false);}
 ScreenState s;s.perspectiveColor=true;s.zTest=s.zWrite=true;s.cull=1;s.blend=0;ScreenVertex wall[4]={{-20,-20,.2f,0,0,110,150,190,255,0},{340,-20,.5f,1,0,110,150,190,255,0},{-20,260,.2f,0,1,110,150,190,255,0},{340,260,.5f,1,1,110,150,190,255,0}};ScreenVertex q[3]={wall[0],wall[1],wall[2]},r[3]={wall[2],wall[1],wall[3]};for(auto p:{&fast,&slow}){p->screenTriangle(q,nullptr,s);p->screenTriangle(r,nullptr,s);}
 for(unsigned k=0;k<350;++k){ScreenVertex v[3]={{10,10,.2f,0,0,90,150,200,160,0},{300,10,.44f,1,0,200,70,120,160,0},{10,225,.2f,0,1,170,200,80,160,0}};s.blend=1;s.zWrite=k%2;s.alphaRef=k%3?1:128;if(k%13==0)for(auto&p:v)p.reciprocalZ+=.2f;fast.screenTriangle(v,nullptr,s);slow.screenTriangle(v,nullptr,s);assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());assert(fast.stats().pixels==slow.stats().pixels);assert(fast.stats().occludedTriangles==slow.stats().occludedTriangles);if(k==280){fast.clearUiDepth();slow.clearUiDepth();}}
 scanned+=fast.stats().tileScanPixels;assert(!slow.stats().tileScanPixels);}
 assert(scanned);std::printf("PASS depth tile scan %u bit-exact IEEE tiles, %llu samples, 4 layered occlusion/clear states RGB/depth/fragments/culls; scanned=%llu\n",cases,(unsigned long long)samples,(unsigned long long)scanned);
}
