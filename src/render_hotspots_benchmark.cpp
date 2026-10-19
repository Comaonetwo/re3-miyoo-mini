// Synthetic timing aid. Hashes validate these scenes; timings are host-only.
#include "soft_renderer.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
using namespace soft;
static void uiBenchmark(){Renderer r;Texture t;t.width=512;t.height=512;t.pixels.resize(512*512);t.alpha.assign(512*512,255);for(unsigned i=0;i<t.pixels.size();++i)t.pixels[i]=rgb(i%256,(i/512)%256,(i*13)%256);ScreenState s;s.linear=true;ScreenVertex v[4]={{0,0,2,0,0,255,255,255,255},{320,0,2,1,0,255,255,255,255},{0,240,2,0,1,255,255,255,255},{320,240,2,1,1,255,255,255,255}};ScreenVertex a[3]={v[0],v[1],v[2]},b[3]={v[2],v[1],v[3]};auto start=std::chrono::steady_clock::now();for(int i=0;i<200;++i){r.clear(0);r.beginUi(true);r.screenTriangle(a,&t,s);r.screenTriangle(b,&t,s);}auto end=std::chrono::steady_clock::now();unsigned long long hash=1469598103934665603ull;for(auto p:r.pixels()){hash^=p;hash*=1099511628211ull;}printf("200 native UI frames %.3f ms/frame hash=%llx\n",std::chrono::duration<double,std::milli>(end-start).count()/200,hash);}





static void worldBenchmark(bool packed,bool batch,bool rows,bool runs){Renderer r;r.edgeRunsEnabled(runs);r.depthRowCullingEnabled(rows);r.packedWorldEnabled(packed);r.batchCountersEnabled(batch);Texture t;t.width=t.height=64;t.pixels.resize(4096);t.alpha.assign(4096,255);for(unsigned i=0;i<4096;++i)t.pixels[i]=rgb(i%256,i/16%256,i*13%256);Camera c;c.position={0,0,0};Vec3 a[3]={{-4,-3,5},{4,-3,5},{-4,3,5}},b[3]={{-4,3,5},{4,-3,5},{4,3,5}};Vec2 u[3]={{0,1},{1,1},{0,0}},v[3]={{0,0},{1,1},{1,0}};auto start=std::chrono::steady_clock::now();for(int i=0;i<1000;++i){r.clear(0);r.texturedTriangle(a,u,rgb(170,190,160),t,3,3,c);r.texturedTriangle(b,v,rgb(170,190,160),t,3,3,c);}auto end=std::chrono::steady_clock::now();unsigned long long hash=1469598103934665603ull;for(auto p:r.pixels()){hash^=p;hash*=1099511628211ull;}printf("1000 world frames %.3f ms/frame hash=%llx\n",std::chrono::duration<double,std::milli>(end-start).count()/1000,hash);}





static void hudBenchmark(){Texture texture;texture.width=texture.height=16;for(int y=0;y<16;++y)for(int x=0;x<16;++x){texture.pixels.push_back(rgb(255,255,255));texture.alpha.push_back(x%4&&y%4?255:0);}for(unsigned width:{240u,320u,640u}){Renderer r;ScreenState s;s.blend=1;s.linear=true;s.alphaRef=1;s.screenDepth=true;s.zTest=s.zWrite=true;auto start=std::chrono::steady_clock::now();for(int frame=0;frame<200;++frame){r.clear(rgb(30,60,90));r.beginHud(width);for(int i=0;i<96;++i){float x=float((i%12)*24+8),y=float((i/12)*22+16);ScreenVertex v[4]={{x,y,2,0,0,136,170,255,255,0},{x+14,y,2,1,0,136,170,255,255,0},{x,y+16,2,0,1,136,170,255,255,0},{x+14,y+16,2,1,1,136,170,255,255,0}};ScreenVertex a[3]={v[0],v[1],v[2]},b[3]={v[2],v[1],v[3]};r.screenTriangle(a,&texture,s);r.screenTriangle(b,&texture,s);}r.pixels();}auto end=std::chrono::steady_clock::now();printf("HUD %u + composite %.3f ms/frame\n",width,std::chrono::duration<double,std::milli>(end-start).count()/200);}}

static void blurBenchmark(){Renderer r;ScreenState s;s.blend=1;ScreenVertex a[3]={{0,0,1,0,0,70,100,140,120},{320,0,1,0,0,70,100,140,120},{0,240,1,0,0,70,100,140,120}},b[3]={a[2],a[1],{320,240,1,0,0,70,100,140,120}};auto start=std::chrono::steady_clock::now();for(int f=0;f<1000;++f){r.clear(rgb(70,80,90));r.screenTriangle(a,nullptr,s);r.screenTriangle(b,nullptr,s);}unsigned long long hash=1469598103934665603ull;for(auto p:r.pixels()){hash^=p;hash*=1099511628211ull;}printf("1000 colour-overlay frames %.3f ms/frame hash=%llx\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/1000,hash);}

int main(int argc,char**argv){bool packed=argc<2||strcmp(argv[1],"--no-packed-world"),batch=argc<2||strcmp(argv[1],"--no-batch-counters");bool rows=argc<2||strcmp(argv[1],"--no-depth-rows");uiBenchmark();worldBenchmark(packed,batch,rows,argc<2||strcmp(argv[1],"--no-edge-runs"));hudBenchmark();blurBenchmark();}
