#include "soft_renderer.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
static void require(bool p,const char* message) { if(!p) {std::cerr<<message<<"\n";std::exit(1);} }
int main() {
    soft::Renderer r;soft::Camera c;c.position={0,0,0};uint16_t red=soft::rgb(255,0,0),green=soft::rgb(0,255,0);
    soft::Vec3 v[3]={{-1,-1,2},{1,-1,2},{0,1,2}};
    r.clear(0);r.triangle(v,red,c);auto pixels=r.pixels();require(r.stats().pixels>10000,"triangle missing");
    std::swap(v[0],v[1]);r.clear(0);r.triangle(v,red,c);require(r.pixels()==pixels,"winding mismatch");
    soft::Vec3 near[3]={{-1,-1,2},{1,-1,2},{0,1,2}},far[3]={{-2,-2,4},{2,-2,4},{0,2,4}};
    r.clear(0);r.triangle(near,red,c);r.triangle(far,green,c);auto order=r.pixels();
    r.clear(0);r.triangle(far,green,c);r.triangle(near,red,c);require(order==r.pixels(),"depth depends on draw order");
    require(r.pixels()[120*320+160]==red,"closer triangle did not win");
    // Slanted triangle versus constant Z checks reciprocal (not affine Z) interpolation.
    soft::Vec3 slope[3]={{-1,-1,2},{2,-2,4},{0,1,2}};
    r.clear(0);r.triangle(slope,green,c);
    require(std::fabs(r.depth()[120*320+160]-.4364583f)<.00001f,"perspective depth incorrect");
    soft::Vec3 cross[3]={{-.1f,-.1f,.01f},{1,-1,2},{0,1,2}};
    r.clear(0);r.triangle(cross,red,c);require(r.stats().pixels>0,"near clipping lost visible polygon");
    for(int plane=0;plane<6;++plane) {
        soft::Vec3 off[3]={{-1,-1,2},{1,-1,2},{0,1,2}};
        for(auto& p:off) {if(plane==0)p.z=-1;if(plane==1)p.z=100;
            if(plane==2)p.x-=100;if(plane==3)p.x+=100;if(plane==4)p.y-=100;if(plane==5)p.y+=100;}
        r.clear(0);r.triangle(off,red,c);require(r.stats().pixels==0,"off-frustum triangle visible");
    }
    // Shared edges must have no cracks, regardless of which face draws first.
    soft::Vec3 qa[3]={{-.5f,-.5f,1.5f},{.5f,-.5f,1.5f},{.5f,.5f,1.5f}};
    soft::Vec3 qb[3]={{-.5f,-.5f,1.5f},{.5f,.5f,1.5f},{-.5f,.5f,1.5f}};
    r.clear(0);r.triangle(qa,red,c);r.triangle(qb,green,c);auto square=r.pixels();
    for(int y=60;y<180;++y)for(int x=100;x<220;++x)require(square[y*320+x]!=0,"shared-edge crack");
    require(r.stats().pixels==14400,"incorrect square coverage");
    r.clear(0);r.triangle(qb,green,c);r.triangle(qa,red,c);require(r.pixels()==square,"shared-edge ownership depends on order");
    soft::Vec3 bad[3]={{0,0,2},{0,0,2},{0,0,2}};
    r.clear(0);r.triangle(bad,red,c);require(r.stats().pixels==0,"degenerate triangle visible");
    bad[0].x=std::numeric_limits<float>::quiet_NaN();r.triangle(bad,red,c);require(r.stats().rejected==1,"NaN accepted");
    std::mt19937 gen(42);std::uniform_real_distribution<float> dist(-100,100);
    for(int i=0;i<10000;++i) {soft::Vec3 t[3];for(auto& p:t)p={dist(gen),dist(gen),dist(gen)};r.triangle(t,red,c);}
    for(float z:r.depth())require(std::isfinite(z)&&z>=0&&z<=10.01f,"depth outside clipped range");
    require(r.bufferBytes()==460800,"unexpected framebuffer memory");
    std::cout<<"PASS: winding, occlusion, reciprocal depth, six-plane clipping, near crossing, shared edges, degenerate, NaN, 10000 random triangles\n";
}
