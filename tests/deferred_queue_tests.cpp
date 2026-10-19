#include "soft_renderer.hpp"
#include <random>
#include <cstdio>
#include <stdexcept>
using namespace soft;
struct Draw {ScreenVertex v[3];ScreenState s;unsigned kind;uint16_t tint;};
static void submit(Renderer& r,const Draw& d,const Texture& t){if(d.kind==0)r.screenTriangle(d.v,&t,d.s);else if(d.kind==1)r.texturedProjectedTriangle(d.v,d.tint,t,1,1,120);else{Vec3 p[3]={{-5,-3,10},{10,3,30},{-2,12,20}};Camera c;c.focal=150;r.triangle(p,d.tint,c);}}
static void same(const Renderer& a,const Renderer& b){if(a.pixels()!=b.pixels()||a.depth()!=b.depth())throw std::runtime_error("deferred image/depth mismatch");}
int main(int argc,char**){const bool quick=argc>1;std::mt19937 rng(550);unsigned cases=0;
for(unsigned scale:{1u,2u,3u,4u})for(unsigned scene=0;scene<(quick?2u:8u);++scene){Texture tex;tex.width=tex.height=64;tex.pixels.resize(4096);tex.alpha.resize(4096);for(unsigned i=0;i<4096;++i){tex.pixels[i]=uint16_t(rng());tex.alpha[i]=scene%2?uint8_t(rng()):255;}tex.decodedOpaque=scene%2==0;
std::vector<Draw> draws;for(unsigned t=0;t<700;++t){Draw d{};d.kind=t%3;d.tint=uint16_t(rng());for(auto& q:d.v){q.x=float(int(rng()%370)-25);q.y=float(int(rng()%290)-25);q.reciprocalZ=.005f+float(rng()%100)/1000;q.u=float(int(rng()%800)-200)/100;q.v=float(int(rng()%800)-200)/100;q.r=rng()%256;q.g=rng()%256;q.b=rng()%256;q.a=t%3?255:180;}d.s.zTest=true;d.s.zWrite=t%2;d.s.blend=t%2;d.s.cull=1;d.s.perspectiveColor=true;d.s.alphaRef=100;d.s.linear=scene%3==0;d.s.addressU=d.s.addressV=1;draws.push_back(d);}
for(unsigned mode:{1u,2u,3u})for(unsigned limit:{64u,128u,512u}){
 if(quick&&(mode!=2||limit==64))continue;
 std::fprintf(stderr,"CASE scale=%u scene=%u mode=%u limit=%u\n",scale,scene,mode,limit);
 Renderer reference,actual;for(auto r:{&reference,&actual}){r->renderScale(scale);r->clear(rgb(30,50,70));r->smallDepthEnabled(true);r->lazyDepthEnabled(scene%2==0);r->filterStages(scene%2?19:31);r->blackAddEnabled(true);r->alphaSpansEnabled(true);r->flatAlphaEnabled(scene%3==0);r->depthBlocksEnabled(scene%2!=0);r->gradientDepthRowsEnabled(true);}
 actual.beginDeferred(mode,limit);
 for(unsigned n=0;n<draws.size();++n){submit(reference,draws[n],tex);submit(actual,draws[n],tex);if(n==137&&scene%4!=0){same(reference,actual);}if(n==374&&scene%4!=0){reference.clearBuffers(rgb(40,12,80),true,true);actual.clearBuffers(rgb(40,12,80),true,true);}if(n==500&&scene%4!=0){actual.flushDeferred();tex.pixels[0]^=31;}if(n==625){actual.endDeferred();actual.beginDeferred(mode==1?2:1,limit);}}
 actual.endDeferred();same(reference,actual);if(reference.stats().pixels!=actual.stats().pixels)throw std::runtime_error("pixel count mismatch");
 // A target switch must finish world work before native UI draws.
 actual.beginDeferred(mode,limit);submit(reference,draws[0],tex);submit(actual,draws[0],tex);reference.beginUi(true);actual.beginUi(true);Draw ui=draws[1];ui.kind=0;ui.s.zTest=ui.s.zWrite=false;ui.s.screenDepth=true;for(auto& q:ui.v){q.reciprocalZ=1;q.depthZ=.5f;}submit(reference,ui,tex);submit(actual,ui,tex);actual.endDeferred();same(reference,actual);
 // Keep the same worker contexts across clears, resolution changes and depth
 // barriers. Unowned rows must never supply an occlusion certificate.

 for(unsigned next:{4u,1u,3u,2u}){for(auto r:{&reference,&actual}){r->renderScale(next);r->clear(rgb(20,10,30));}actual.beginDeferred(mode,64);for(unsigned n=0;n<160;++n){submit(reference,draws[n],tex);submit(actual,draws[n],tex);}reference.clearUiDepth();actual.clearUiDepth();for(unsigned n=160;n<240;++n){submit(reference,draws[n],tex);submit(actual,draws[n],tex);}actual.endDeferred();same(reference,actual);}
 ++cases;
}}
std::printf("PASS %u integrated queue cases: mixed materials, read/clear/texture/target barriers and mode changes\n",cases);
}
