#include "software_device.hpp"
#include "world_simplifier.hpp"
#include <cassert>
#include <cstdio>
using namespace soft;
static rw::Geometry* grid(bool mixed){unsigned vertices=mixed?1250:625,triangles=mixed?2304:1152;
 auto g=rw::Geometry::create(vertices,triangles,rw::Geometry::POSITIONS|rw::Geometry::PRELIT|rw::Geometry::MODULATE|rw::Geometry::TEXTURED);
 for(unsigned m=0;m<(mixed?2u:1u);++m){auto mat=rw::Material::create();g->matList.appendMaterial(mat);mat->destroy();}
 for(unsigned layer=0;layer<(mixed?2u:1u);++layer){unsigned offset=625*layer;
  for(unsigned y=0;y<=24;++y)for(unsigned x=0;x<=24;++x){unsigned i=offset+y*25+x;g->morphTargets[0].vertices[i]=layer?rw::V3d{float(x)*.2f-2.4f,5,float(y)*.2f}:rw::V3d{float(x)*.2f-2.4f,float(y)*.2f-2.4f,10};g->texCoords[0][i]={x/24.f,y/24.f};g->colors[i]={120,170,210,255};}
  for(unsigned y=0;y<24;++y)for(unsigned x=0;x<24;++x){unsigned n=layer*1152+(y*24+x)*2,v=offset+y*25+x;g->triangles[n]={{uint16_t(v),uint16_t(v+1),uint16_t(v+25)},uint16_t(layer)};g->triangles[n+1]={{uint16_t(v+25),uint16_t(v+1),uint16_t(v+26)},uint16_t(layer)};}
 }return g;
}
int main(){Renderer r;SoftwareDevice d(r);auto floor=grid(false);auto locked=simplifyWorldGeometry(floor,1024*1024,nullptr,0,true);assert(!locked.geometry);floor->destroy();
 auto high=grid(true);auto mid=simplifyWorldGeometry(high,1024*1024,nullptr,0,true);assert(mid.geometry);auto low=mid.geometry;assert(low->numTriangles<high->numTriangles*.9f&&low->numTriangles>=1152);
 unsigned retained=0;for(int i=0;i<low->numTriangles;++i){auto t=low->triangles[i];if(t.matId!=0)continue;assert(retained<1152);auto original=high->triangles[retained++];for(unsigned k=0;k<3;++k){auto a=low->morphTargets[0].vertices[t.v[k]],b=high->morphTargets[0].vertices[original.v[k]];assert(a.x==b.x&&a.y==b.y&&a.z==b.z);auto u=low->texCoords[0][t.v[k]],v=high->texCoords[0][original.v[k]];assert(u.u==v.u&&u.v==v.v);}}
 assert(retained==1152);assert(low->matList.materials[0]==high->matList.materials[0]);
 auto a=rw::Atomic::create();auto frame=rw::Frame::create();a->setFrame(frame);Camera c;c.position={0,0,0};d.begin(c);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);a->setGeometry(high,0);r.clear(0);a->render();auto image=r.pixels();a->setGeometry(low,0);r.clear(0);a->render();assert(image==r.pixels());d.end();a->destroy();frame->destroy();low->destroy();
 d.simplificationBudget(1);auto cached=SoftwareDevice::simplifiedWorld(high,nullptr,0,true);assert(cached!=high);assert(SoftwareDevice::simplifiedSafeAt(high,{0,0,0},1));assert(!SoftwareDevice::simplifiedSafeAt(high,{0,5,2},1));
 d.simplificationBudget(0);assert(SoftwareDevice::simplifiedWorld(high,nullptr,0,false)==high);d.simplificationBudget(1);assert(SoftwareDevice::simplifiedWorld(high,nullptr,0,true)!=high);
 high->destroy();assert(d.simplifiedWorldBytes()==0&&d.clusterCacheBytes()==0&&!d.unsupported());
 puts("PASS intermediate: all-floor fallback; mixed floor/wall reduction; exact ground positions/UV/material/order and RGB; near-region fallback; recipe isolation and ownership");
}
