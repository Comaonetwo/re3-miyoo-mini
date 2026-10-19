#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
static void quad(Renderer& r,float z,const Texture* t,ScreenState s){
    ScreenVertex v[4]={{0,0,z,0,0,180,210,170,255,0},{320,0,z,1,0,180,210,170,255,0},{0,240,z,0,1,180,210,170,255,0},{320,240,z,1,1,180,210,170,255,0}};
    ScreenVertex a[3]={v[0],v[1],v[2]},b[3]={v[2],v[1],v[3]};r.screenTriangle(a,t,s);r.screenTriangle(b,t,s);
}
int main(){std::mt19937 random(8319);unsigned rejected=0;
 for(unsigned scale=1;scale<=4;++scale)for(unsigned scenario=0;scenario<5;++scenario){
    Renderer on,off;on.renderScale(scale);off.renderScale(scale);off.occlusionEnabled(false);
    Texture t;t.width=t.height=16;t.pixels.resize(256);t.alpha.assign(256,255);
    for(unsigned i=0;i<256;++i){t.pixels[i]=uint16_t(random());if(scenario==1)t.alpha[i]=i%5?255:0;if(scenario==2)t.alpha[i]=i%3?128:0;}
    ScreenState s;s.zTest=s.zWrite=true;s.blend=1;s.alphaRef=1;s.perspectiveColor=true;
    quad(on,2,&t,s);quad(off,2,&t,s);
    for(unsigned n=0;n<1300;++n){
        if(n==400&&scenario==3){on.clearUiDepth();off.clearUiDepth();}
        if(n==400&&scenario==4){ScreenState overwrite=s;overwrite.zTest=false;quad(on,.001f,&t,overwrite);quad(off,.001f,&t,overwrite);}
        ScreenVertex v[3];for(auto& p:v){p={float(random()%400)-40,float(random()%320)-40,float(1+random()%100)/100.f,float(int(random()%500)-200)/100.f,float(int(random()%500)-200)/100.f,float(random()%256),float(random()%256),float(random()%256),float(random()%256),float(random()%100)/100.f};}
        ScreenState state=s;state.addressU=1+random()%4;state.addressV=1+random()%4;state.linear=n%7==0;state.blend=n%6;state.zWrite=n%3!=0;
        if(scenario==4&&n%71==0){state.screenDepth=true;state.zTest=false;state.zWrite=true;}
        on.screenTriangle(v,&t,state);off.screenTriangle(v,&t,state);
    }
    assert(on.pixels()==off.pixels());assert(on.depth()==off.depth());rejected+=on.stats().occludedTriangles;
    // A fresh frame and transitions must discard all prior occlusion data.
    on.clear(0);off.clear(0);quad(on,.01f,&t,s);quad(off,.01f,&t,s);assert(on.pixels()==off.pixels());
    on.beginHud(240);off.beginHud(240);quad(on,.01f,&t,s);quad(off,.01f,&t,s);assert(on.pixels()==off.pixels());
    on.beginUi(true);off.beginUi(true);quad(on,.01f,&t,s);quad(off,.01f,&t,s);assert(on.pixels()==off.pixels());
 }
 assert(rejected>1000);printf("PASS conservative visibility: %u rejected, holes/alpha/clear/unconditional writes, four resolutions, identical colour and depth\n",rejected);
}
