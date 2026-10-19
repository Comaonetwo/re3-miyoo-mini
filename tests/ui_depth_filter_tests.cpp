#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <cmath>
using namespace soft;
static void quad(Renderer& r,ScreenState s,float depth,float reciprocal,unsigned red,unsigned green,unsigned blue,const Texture* t=nullptr,float uv=.5f){ScreenVertex a[4]={{0,0,reciprocal,uv,uv,float(red),float(green),float(blue),255,depth},{320,0,reciprocal,uv,uv,float(red),float(green),float(blue),255,depth},{0,240,reciprocal,uv,uv,float(red),float(green),float(blue),255,depth},{320,240,reciprocal,uv,uv,float(red),float(green),float(blue),255,depth}};ScreenVertex x[3]={a[0],a[1],a[2]},y[3]={a[2],a[1],a[3]};r.screenTriangle(x,t,s);r.screenTriangle(y,t,s);}
int main(){Renderer r;for(unsigned scale:{1u,2u,3u,4u})for(bool native:{false,true}){r.renderScale(scale);r.clear(rgb(0,0,255));r.beginUi(native);unsigned w=r.outputWidth(),mul=w/320;assert(r.pixels().size()==w*(w*3/4));ScreenState mask;mask.screenDepth=true;mask.zWrite=true;mask.blend=5;
 ScreenVertex m[3]={{0,0,2,0,0,255,255,255,255,0},{40,0,2,0,0,255,255,255,255,0},{0,40,2,0,0,255,255,255,255,0}};r.screenTriangle(m,nullptr,mask);
 ScreenState map;map.screenDepth=true;map.zTest=true;quad(r,map,.0001f,10,0,255,0);assert(r.pixels()[5*mul*w+5*mul]==rgb(0,0,255));assert(r.pixels()[100*mul*w+100*mul]==rgb(0,255,0));}
 r.renderScale(1);r.clear(0);ScreenState glyph;glyph.screenDepth=true;glyph.zTest=glyph.zWrite=true;quad(r,glyph,0,2,0,0,0);quad(r,glyph,0,2,255,255,255);assert(r.pixels()[120*320+160]==rgb(255,255,255));
 r.clear(0);Texture t{2,2,{rgb(255,0,0),rgb(0,255,0),rgb(0,0,255),rgb(255,255,255)},{255,255,255,255}};ScreenState filter;filter.linear=true;quad(r,filter,0,1,255,255,255,&t);assert(r.pixels()[120*320+160]==rgb(128,128,128));
 t.pixels.assign(4,rgb(255,255,255));t.alpha={0,255,255,0};filter.blend=1;filter.alphaRef=1;r.clear(rgb(0,0,255));quad(r,filter,0,1,255,255,255,&t);assert(r.pixels()[120*320+160]==rgb(128,128,255));
 r.clear(rgb(0,0,255));quad(r,filter,0,1,255,255,255,&t,0);assert(r.pixels()[120*320+160]==rgb(0,0,255));
 // Independent bilinear reference at negative/repeated UV, including a
 // non-power-of-two texture. This covers the new integer neighbour address.
 for(unsigned size:{3u,4u}){Texture n;n.width=size;n.height=size;n.alpha.assign(size*size,255);for(unsigned i=0;i<size*size;++i)n.pixels.push_back(rgb(i*17,i*11,i*7));
  for(unsigned mode=1;mode<=4;++mode)for(float uv:{-2.f,-1.5f,-.125f,0.f,.125f,.5f,.875f,1.f,1.5f,2.f}){
   float pos=uv*size-.5f;int base=int(std::floor(pos));unsigned fraction=unsigned((pos-base)*256);
   auto address=[&](int x){if(mode>=3)return std::max(0,std::min(int(size)-1,x));int period=int(size)*(mode==2?2:1);x=(x%period+period)%period;return mode==2&&x>=int(size)?period-1-x:x;};
   unsigned red=0,green=0,blue=0;for(int y=0;y<2;++y)for(int x=0;x<2;++x){unsigned weight=(x?fraction:256-fraction)*(y?fraction:256-fraction),c=n.pixels[address(base+y)*size+address(base+x)];unsigned rr=c>>11,gg=(c>>5)&63,bb=c&31;red+=((rr<<3)|(rr>>2))*weight;green+=((gg<<2)|(gg>>4))*weight;blue+=((bb<<3)|(bb>>2))*weight;}
   ScreenState state;state.linear=true;state.addressU=state.addressV=mode;r.clear(0);quad(r,state,0,2,255,255,255,&n,uv);assert(r.pixels()[120*320+160]==rgb((red+32768)>>16,(green+32768)>>16,(blue+32768)>>16));
  }
 }
 puts("PASS independent Im2D depth, radar mask, 320/640 UI, bilinear color/alpha and transparent edge clamp");}
