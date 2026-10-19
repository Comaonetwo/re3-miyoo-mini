#include "software_device.hpp"
#include "model_clusters.hpp"
#include <cassert>
#include <cstdio>
using namespace soft;
static rw::Geometry* low=nullptr;
static rw::Geometry* choose(rw::Atomic* a){return low?low:a->geometry;}
static rw::Geometry* mesh(unsigned repeats){auto g=rw::Geometry::create(4,2*repeats,rw::Geometry::POSITIONS|rw::Geometry::PRELIT);auto m=rw::Material::create();g->matList.appendMaterial(m);m->destroy();rw::V3d p[]={{-2,-2,6},{2,-2,6},{2,2,6},{-2,2,6}};for(int i=0;i<4;++i){g->morphTargets[0].vertices[i]=p[i];g->colors[i]={120,170,200,255};}for(unsigned n=0;n<repeats;++n){g->triangles[n*2]={{0,1,2},0};g->triangles[n*2+1]={{0,2,3},0};}return g;}
int main(){Renderer r;SoftwareDevice d(r);auto high=mesh(8);low=mesh(1);auto a=rw::Atomic::create();a->setGeometry(high,0);auto f=rw::Frame::create();a->setFrame(f);Camera c;d.begin(c);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
 assert(SoftwareDevice::opaqueGeometry(high)&&SoftwareDevice::opaqueGeometry(low));ModelBounds bounds;assert(SoftwareDevice::geometryBounds(low,bounds)&&bounds.valid);
 d.worldLodSelector(choose);r.clear(0);a->render();auto pixels=r.pixels();auto depth=r.depth();assert(a->geometry==high&&d.worldLodAtomics()==1&&d.worldLodInputSaved()==14);
 a->setGeometry(low,0);d.worldLodSelector(nullptr);r.clear(0);a->render();assert(pixels==r.pixels()&&depth==r.depth());a->setGeometry(high,0);
 high->lock(rw::Geometry::LOCKPRELIGHT);high->colors[0].alpha=100;high->unlock();assert(!SoftwareDevice::opaqueGeometry(high));high->lock(rw::Geometry::LOCKPRELIGHT);high->colors[0].alpha=255;high->unlock();assert(SoftwareDevice::opaqueGeometry(high));
 low->lock(rw::Geometry::LOCKVERTICES);low->morphTargets[0].vertices[0].x=-3;low->unlock();assert(SoftwareDevice::opaqueGeometry(low));assert(SoftwareDevice::geometryBounds(low,bounds)&&bounds.minimum.x==-3);
 high->matList.materials[0]->color.alpha=160;assert(!SoftwareDevice::opaqueGeometry(high));high->matList.materials[0]->color.alpha=255;
 d.end();a->destroy();f->destroy();high->destroy();low->destroy();low=nullptr;assert(d.clusterCacheBytes()==0&&!d.unsupported());puts("PASS original atomic ownership retained, paired geometry RGB/depth, input reduction, alpha/topology dirty certificates and cache release");}
