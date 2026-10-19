#include "software_device.hpp"
#include <cassert>
#include <random>
#include <cstdio>
using namespace soft;
static bool attach(){return true;}
int main(){Renderer r;SoftwareDevice d(r,attach);std::mt19937 rng(4211);
 auto g=rw::Geometry::create(4,2,rw::Geometry::POSITIONS|rw::Geometry::PRELIT|rw::Geometry::MODULATE|rw::Geometry::LIGHT);auto m=rw::Material::create();m->surfaceProps={.2f,0,.7f};g->matList.appendMaterial(m);m->destroy();
 rw::V3d p[]={{-2,-2,6},{2,-2,6},{-2,2,6},{2,2,6}};for(unsigned i=0;i<4;++i){g->morphTargets[0].vertices[i]=p[i];g->colors[i]={120,150,190,255};}g->triangles[0]={{0,1,2},0};g->triangles[1]={{2,1,3},0};
 auto a=rw::Atomic::create();a->setGeometry(g,0);Camera c;
 for(unsigned n=0;n<96;++n){
  d.highQuality(n%3!=0);d.bakedLighting(n%7==0);d.lighting({.1f+(n%8)*.03f,.2f,.3f},{.3f,.2f,.1f},{0,0,1});
  g->matList.materials[0]->color={uint8_t(100+n%40),uint8_t(180+n%60),210,uint8_t(n%5==0?0:n%5==1?1:n%5==2?128:255)};
  g->lock(rw::Geometry::LOCKPRELIGHT);for(unsigned i=0;i<4;++i)g->colors[i]={uint8_t(rng()),uint8_t(rng()),uint8_t(rng()),uint8_t(n%2?255:rng())};g->unlock();
  if(n%4==0)SoftwareDevice::opaqueGeometry(g); // separate dirty-flag consumer
  if(n%6==0){g->lock(rw::Geometry::LOCKVERTICES);g->morphTargets[0].vertices[0].x=-2-float(n%3)*.1f;g->unlock();}
  if(n%5==0)g->flags&=~rw::Geometry::MODULATE;else g->flags|=rw::Geometry::MODULATE;
  d.begin(c);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::VERTEXALPHA,1);rw::SetRenderState(rw::SRCBLEND,rw::BLENDSRCALPHA);rw::SetRenderState(rw::DESTBLEND,rw::BLENDINVSRCALPHA);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);rw::SetRenderState(rw::ZWRITEENABLE,n%2);
  d.vertexColorCache(false);r.clear(12345);a->render();auto image=r.pixels();auto depth=r.depth();auto count=r.stats().pixels;
  d.vertexColorCache(true);
  for(unsigned repeat=0;repeat<4;++repeat){r.clear(12345);a->render();assert(image==r.pixels()&&depth==r.depth()&&count==r.stats().pixels);}
  // Alpha changes do not invalidate RGB, but must affect the submitted alpha.
  g->matList.materials[0]->color.alpha=64;d.vertexColorCache(false);r.clear(12345);a->render();image=r.pixels();depth=r.depth();d.vertexColorCache(true);r.clear(12345);a->render();assert(image==r.pixels()&&depth==r.depth());d.end();
  assert(d.vertexColorCacheBytes()<=256*1024);
 }
 assert(d.cachedVertexColors()>500&&d.preparedVertexColors()>100&&d.vertexColorCacheBytes()>0&&!d.unsupported());
 a->destroy();g->destroy();assert(d.vertexColorCacheBytes()==0&&d.clusterCacheBytes()==0);
 // Real pressure: eight 35 KiB requests cannot all fit the 256 KiB budget.
 std::vector<rw::Atomic*> objects;std::vector<rw::Geometry*> meshes;
 d.highQuality(true);d.bakedLighting(false);d.vertexColorCache(true);
 for(unsigned n=0;n<8;++n){
  auto mesh=rw::Geometry::create(7000,1,rw::Geometry::POSITIONS|rw::Geometry::PRELIT|rw::Geometry::LIGHT);auto material=rw::Material::create();mesh->matList.appendMaterial(material);material->destroy();
  for(unsigned i=0;i<7000;++i){mesh->morphTargets[0].vertices[i]={-2,-2,6};mesh->colors[i]={120,150,190,255};}
  for(unsigned i=0;i<3;++i)mesh->morphTargets[0].vertices[i]=p[i];mesh->triangles[0]={{0,1,2},0};auto object=rw::Atomic::create();object->setGeometry(mesh,0);objects.push_back(object);meshes.push_back(mesh);
  d.begin(c);for(unsigned repeat=0;repeat<3;++repeat){r.clear(12345);object->render();}d.end();assert(d.vertexColorCacheBytes()<=256*1024);
 }
 assert(d.vertexColorCacheBytes()>220000);
 for(unsigned i=0;i<objects.size();++i){objects[i]->destroy();meshes[i]->destroy();}
 assert(d.vertexColorCacheBytes()==0&&d.clusterCacheBytes()==0&&!d.unsupported());
 puts("PASS 96 mutable real geometries: cached RGB/depth/counts, ambient/material/alpha/MODULATE/FLAT/SMOOTH/BAKED/opacity/vertex dirty invalidation and ownership");
}
