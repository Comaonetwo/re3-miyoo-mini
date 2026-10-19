#include "soft_renderer.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <limits>
#include <stdexcept>
using namespace soft;
static void check(bool b){if(!b)throw std::runtime_error("texture test failed");}
static Vec3 screen(float x,float y,float z){return {(x-160)*z/180,(120-y)*z/180,z};}
int main(){try{
    Camera cam;Texture texture;texture.width=texture.height=16;
    for(int y=0;y<16;++y)for(int x=0;x<16;++x){texture.pixels.push_back(rgb(x*16,y*16,80));texture.alpha.push_back(255);}
    Renderer r;Vec3 p[3]={screen(40,30,1),screen(280,30,8),screen(40,210,4)};Vec2 uv[3]={{0,0},{1,0},{0,1}};
    r.clear(0);r.texturedTriangle(p,uv,65535,texture,3,3,cam);
    // Analytic projective interpolation reference, deliberately unequal depths.
    for(int y:{45,70,100,130})for(int x:{55,80,110}){
        double b=(x+.5-40)/240,c=(y+.5-30)/180,a=1-b-c,z=a+b/8+c/4;
        int u=int(std::floor((b/8/z)*16)),v=int(std::floor((c/4/z)*16));
        check(r.pixels()[y*320+x]==texture.pixels[v*16+u]);
    }
    auto expected=r.pixels();std::swap(p[1],p[2]);std::swap(uv[1],uv[2]);
    r.clear(0);r.texturedTriangle(p,uv,65535,texture,3,3,cam);check(expected==r.pixels());
    // Address modes with constant negative UV; repeat, mirror, clamp.
    p[0]=screen(40,30,2);p[1]=screen(280,30,2);p[2]=screen(40,210,2);
    for(auto& t:uv)t={-.125f,1.125f};
    for(int mode=1;mode<=3;++mode){r.clear(0);r.texturedTriangle(p,uv,65535,texture,mode,mode,cam);
        int u=mode==1?14:mode==2?1:0,v=mode==1?2:mode==2?13:15;
        check(r.pixels()[80*320+80]==texture.pixels[v*16+u]);}
    // Transparent texels must not write depth and must reveal a farther surface.
    std::fill(texture.alpha.begin(),texture.alpha.end(),0);r.clear(0);r.texturedTriangle(p,uv,65535,texture,1,1,cam);
    check(r.stats().pixels==0);for(auto v:r.depth())check(v==0);
    for(auto& v:p){v.x*=2;v.y*=2;v.z*=2;}r.triangle(p,rgb(0,0,255),cam);check(r.pixels()[80*320+80]==rgb(0,0,255));
    // A constant opaque texture has the same clipped coverage as flat rendering.
    std::fill(texture.alpha.begin(),texture.alpha.end(),255);std::fill(texture.pixels.begin(),texture.pixels.end(),65535);
    p[0]={-.5f,-.5f,.02f};p[1]={1,-.5f,2};p[2]={0,1,2};for(auto& t:uv)t={.5f,.5f};
    Renderer flat;flat.clear(0);flat.triangle(p,rgb(64,128,192),cam);r.clear(0);
    r.texturedTriangle(p,uv,rgb(64,128,192),texture,1,1,cam);check(flat.pixels()==r.pixels());
    // Exact modulation component values, cache collisions and white bypass.
    Texture solid{1,1,{0},{255}};Vec3 patch[3]={screen(100,80,2),screen(200,80,2),screen(100,160,2)};Vec2 one[3]={{.5f,.5f},{.5f,.5f},{.5f,.5f}};
    for(unsigned tint:{0u,65535u,12345u,54321u,31727u,43690u})for(unsigned i=0;i<128;++i){unsigned pixel=i<32?i<<11:i<96?(i-32)<<5:i-96;solid.pixels[0]=pixel;r.clear(0);r.texturedTriangle(patch,one,tint,solid,3,3,cam);unsigned rr=(((pixel>>11)&31)*((tint>>11)&31)+15)/31,gg=(((pixel>>5)&63)*((tint>>5)&63)+31)/63,bb=((pixel&31)*(tint&31)+15)/31;check(r.pixels()[100*320+120]==((rr<<11)|(gg<<5)|bb));}
    uv[0].u=std::numeric_limits<float>::quiet_NaN();r.clear(0);r.texturedTriangle(p,uv,65535,texture,1,1,cam);check(r.stats().rejected==1);
    uv[0].u=0;texture.width=15;r.clear(0);r.texturedTriangle(p,uv,65535,texture,1,1,cam);check(r.stats().rejected==1);
    std::puts("perspective/winding/addressing/alpha/clipping PASS");return 0;
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
