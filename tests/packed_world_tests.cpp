#include "soft_renderer.hpp"
#include "packed_world.hpp"
#include <cassert>
#include <random>
#include <cstdio>
using namespace soft;
int main(){const auto& tables=PackedTintTables::get();for(unsigned a=0;a<32;++a)for(unsigned b=0;b<32;++b)assert(tables.five[a*32+b]==(a*b+15)/31);for(unsigned a=0;a<64;++a)for(unsigned b=0;b<64;++b)assert(tables.six[a*64+b]==(a*b+31)/63);
 std::mt19937 rng(2202);unsigned long long pixels=0,covered=0,texelRuns=0,packets=0,lean=0;
 for(unsigned scale=1;scale<=4;++scale)for(unsigned n=0;n<300;++n){Renderer fast,slow;fast.packetSamplerEnabled((n/2)%4==0);fast.depthBlocksEnabled((n/2)%4==2);fast.renderScale(scale);slow.renderScale(scale);slow.depthRowCullingEnabled(false);slow.edgeRunsEnabled(false);slow.texelRunsEnabled(false);slow.packetSamplerEnabled(false);fast.texelRunsEnabled((n/2)%4==1);slow.batchCountersEnabled(false);slow.packedWorldEnabled(false);Texture t;t.width=1u<<(rng()%8);t.height=1u<<(rng()%8);t.pixels.resize(t.width*t.height);t.alpha.resize(t.pixels.size());for(unsigned j=0;j<t.pixels.size();++j){t.pixels[j]=rng();t.alpha[j]=n%2?255:rng()%256;}
  t.decodedOpaque=n%2;
  Camera c;c.position={0,0,0};c.nearZ=.2f;c.farZ=50;unsigned short bg=rng();fast.clear(bg);slow.clear(bg);
  for(unsigned k=0;k<5;++k){Vec3 v[3];Vec2 uv[3];for(unsigned j=0;j<3;++j){v[j]={float(int(rng()%300)-150)/10,float(int(rng()%200)-100)/10,n%7?.01f+float(rng()%700)/10:8.f};uv[j]={float(int(rng()%1200)-600)/100,float(int(rng()%1200)-600)/100};}unsigned short tint=n%11?rng():65535;unsigned modeU=n%8==0?2:n%3==0?3:1,modeV=n%9==0?2:modeU,alphaRef=n%13==0?0:n%5==0?128:1;fast.texturedTriangle(v,uv,tint,t,modeU,modeV,c,alphaRef);slow.texturedTriangle(v,uv,tint,t,modeU,modeV,c,alphaRef);}
  assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());assert(fast.stats().pixels==slow.stats().pixels&&fast.stats().spanSkippedPixels==slow.stats().spanSkippedPixels&&fast.stats().perspectiveSaved==slow.stats().perspectiveSaved);pixels+=fast.stats().packedWorldPixels;covered+=fast.stats().coverageRunPixels;texelRuns+=fast.stats().texelRunPixels;packets+=fast.stats().packetPixels;lean+=fast.stats().leanPackedTriangles;
 }assert(lean>1000);assert(pixels>10000);if(pixelDiagnosticsEnabled()){assert(covered>1000);if(packetSamplerAvailable)assert(packets>100);}printf("PASS 1200 layered clipped flat-world RGB/depth comparisons, accelerated=%llu pixels, exact 5/6-bit tint tables\n",pixels);
}
