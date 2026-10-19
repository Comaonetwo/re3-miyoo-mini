#include "soft_renderer.hpp"
#include <chrono>
#include <cstdio>
#include <cassert>
using namespace soft;
int main(){for(unsigned scale:{1u,3u}){Texture t;t.width=t.height=64;t.alpha.assign(4096,255);for(unsigned i=0;i<4096;++i)t.pixels.push_back(rgb(i*17%256,i*31%256,i*73%256));t.decodedOpaque=true;std::vector<uint16_t> reference;std::vector<float> depth;
 for(unsigned mode=0;mode<2;++mode){Renderer r;r.packetSamplerEnabled(false);r.renderScale(scale);r.texelRunsEnabled(mode);Camera c;c.position={0,0,0};Vec3 a[3]={{-4,-3,4},{5,-3,8},{-4,3,4}},b[3]={a[2],a[1],{5,3,8}};Vec2 uv[3]={{.203f,.217f},{.253f,.217f},{.203f,.267f}},uvb[3]={uv[2],uv[1],{.253f,.267f}};
 auto start=std::chrono::steady_clock::now();for(unsigned frame=0;frame<500;++frame){r.clear(0);r.texturedTriangle(a,uv,rgb(170,190,160),t,1,1,c);r.texturedTriangle(b,uvb,rgb(170,190,160),t,1,1,c);}
 if(!mode){reference=r.pixels();depth=r.depth();}else{assert(reference==r.pixels());assert(depth==r.depth());}
 printf("texel_run scale=%u enabled=%u %.4f ms/frame reused=%llu blocks=%llu reciprocal_calls=%llu\n",scale,mode,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/500,(unsigned long long)r.stats().texelRunPixels,(unsigned long long)r.stats().texelRunBlocks,(unsigned long long)r.stats().packedReciprocals);
 }} }
