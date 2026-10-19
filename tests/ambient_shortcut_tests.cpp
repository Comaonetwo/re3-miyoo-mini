#include "software_device.hpp"
#include <cassert>
#include <cstdio>
using namespace soft;
int main(){Renderer r;SoftwareDevice d(r);d.highQuality(true);
 auto g=rw::Geometry::create(3,1,rw::Geometry::POSITIONS|rw::Geometry::NORMALS|rw::Geometry::PRELIT|rw::Geometry::LIGHT|rw::Geometry::MODULATE);auto m=rw::Material::create();m->surfaceProps={1,0,1};g->matList.appendMaterial(m);m->destroy();g->triangles[0]={{0,1,2},0};
 rw::V3d p[]={{-2,-2,6},{2,-2,6},{0,2,6}};
 for(unsigned i=0;i<3;++i){g->morphTargets[0].vertices[i]=p[i];g->morphTargets[0].normals[i]={float(i)-1,1,1};g->colors[i]={uint8_t(120+i*30),150,190,255};}
 auto a=rw::Atomic::create();auto f=rw::Frame::create();a->setGeometry(g,0);a->setFrame(f);Camera c;
 for(unsigned n=0;n<48;++n){d.lighting({.1f+n*.01f,.2f,.3f},{.3f,.2f,.1f},{0,0,1});
  for(auto link=d.world()->globalLights.link.next;link!=&d.world()->globalLights.link;link=link->next){auto light=rw::Light::fromWorld(link);if(light->getType()==rw::Light::DIRECTIONAL)light->setFlags(n%2?rw::Light::LIGHTATOMICS:0);}
  g->matList.materials[0]->color.alpha=n%3?128:255;d.begin(c);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::VERTEXALPHA,1);rw::SetRenderState(rw::SRCBLEND,rw::BLENDSRCALPHA);rw::SetRenderState(rw::DESTBLEND,rw::BLENDINVSRCALPHA);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);rw::SetRenderState(rw::ZWRITEENABLE,n%3);
  d.ambientShortcut(false);r.clear(12145);a->render();auto image=r.pixels();auto depth=r.depth();auto count=r.stats().pixels;
  d.ambientShortcut(true);r.clear(12145);a->render();assert(image==r.pixels()&&depth==r.depth()&&count==r.stats().pixels);d.end();
 }
 assert(d.ambientVerticesSkipped()>0&&!d.unsupported());a->destroy();f->destroy();g->destroy();puts("PASS ambient-only vs directional lighting, changing ambient/material/prelight, alpha and depth: exact frames");
}
