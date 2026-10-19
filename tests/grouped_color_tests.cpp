#include "software_device.hpp"
#include <cassert>
#include <cstdio>
using namespace soft;
int main(){Renderer r;SoftwareDevice d(r);const unsigned side=6;auto g=rw::Geometry::create(49,72,rw::Geometry::POSITIONS|rw::Geometry::NORMALS|rw::Geometry::LIGHT|rw::Geometry::PRELIT|rw::Geometry::MODULATE);
 for(unsigned i=0;i<3;++i){auto m=rw::Material::create();g->matList.appendMaterial(m);m->destroy();}
 for(unsigned y=0;y<=side;++y)for(unsigned x=0;x<=side;++x){unsigned v=y*7+x;g->morphTargets[0].vertices[v]={float(x)*.5f-1.5f,float(y)*.5f-1.5f,6};g->morphTargets[0].normals[v]={.1f,0,-1};g->colors[v]={uint8_t(20+x*8),uint8_t(30+y*8),40,255};}
 for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x){unsigned t=(y*side+x)*2,v=y*7+x;g->triangles[t]={{uint16_t(v),uint16_t(v+1),uint16_t(v+7)},uint16_t((x/2+y)%3)};g->triangles[t+1]={{uint16_t(v+7),uint16_t(v+1),uint16_t(v+8)},uint16_t((x/2+y)%3)};}
 g->calculateBoundingSphere();auto a=rw::Atomic::create();a->setGeometry(g,0);d.world()->addAtomic(a);Camera c;c.position={0,0,0};uint64_t extra=0;
 for(unsigned n=0;n<120;++n){d.highQuality(n%2);d.bakedLighting(n%5==0);r.renderScale(1+n%4);
  for(unsigned i=0;i<3;++i){auto m=g->matList.materials[i];m->color={uint8_t(i==2?120:230),uint8_t(210+n%10),180,uint8_t(n%3?255:180)};m->surfaceProps.ambient=float(i==2?.4f:.7f);m->surfaceProps.diffuse=float(n%7)/10;}
  if(n%9==0)g->matList.materials[1]->color.red=140;
  if(n%11==0)g->matList.materials[1]->surfaceProps.diffuse=.25f;
  g->flags=(g->flags&~(rw::Geometry::LIGHT|rw::Geometry::MODULATE))|(n%4?rw::Geometry::LIGHT:0)|(n%3?rw::Geometry::MODULATE:0);
  d.lighting({.1f,.2f,.15f},{.4f,.3f,.2f},{float(n%3)*.1f,0,-1});d.begin(c);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::VERTEXALPHA,n%3==0);rw::SetRenderState(rw::ZWRITEENABLE,n%5!=0);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
  d.groupedColors(false);r.clear(rgb(30,40,50));a->render();auto image=r.pixels();auto depth=r.depth();auto pixels=r.stats().pixels;auto before=d.reusedColors();d.groupedColors(true);r.clear(rgb(30,40,50));a->render();assert(image==r.pixels()&&depth==r.depth()&&pixels==r.stats().pixels);extra+=d.reusedColors()-before;d.end();
 }
 assert(extra>1000&&!d.unsupported());d.world()->removeAtomic(a);a->destroy();g->destroy();printf("PASS 120 mixed-material light/alpha/quality frames: exact RGB/depth/pixels; reused=%llu\n",(unsigned long long)extra);
}
