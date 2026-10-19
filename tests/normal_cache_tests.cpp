#include "software_device.hpp"
#include <cassert>
#include <cstdio>
using namespace soft;
static bool attach(){return true;}
int main(){Renderer r;SoftwareDevice d(r,attach);const unsigned side=8,n=(side+1)*(side+1);auto g=rw::Geometry::create(n,side*side*2,rw::Geometry::POSITIONS|rw::Geometry::NORMALS|rw::Geometry::LIGHT|rw::Geometry::PRELIT|rw::Geometry::MODULATE);auto m=rw::Material::create();m->color={190,210,230,255};g->matList.appendMaterial(m);m->destroy();
 for(unsigned y=0;y<=side;++y)for(unsigned x=0;x<=side;++x){unsigned i=y*(side+1)+x;g->morphTargets[0].vertices[i]={float(x)*.5f-2,float(y)*.5f-2,6};g->morphTargets[0].normals[i]={float(int(i%3)-1)*.2f,.1f,-1};g->colors[i]={40,35,30,255};}
 for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x){unsigned t=(y*side+x)*2,v=y*(side+1)+x;g->triangles[t]={{uint16_t(v),uint16_t(v+1),uint16_t(v+side+1)},0};g->triangles[t+1]={{uint16_t(v+side+1),uint16_t(v+1),uint16_t(v+side+2)},0};}
 g->calculateBoundingSphere();auto a=rw::Atomic::create();auto f=rw::Frame::create();a->setGeometry(g,0);a->setFrame(f);d.world()->addAtomic(a);Camera c;c.position={0,0,0};
 for(unsigned k=0;k<60;++k){rw::Matrix mat;mat.setIdentity();float angle=float(k/5)*.03f;mat.right={std::cos(angle)*1.2f,0,std::sin(angle)*1.2f};mat.up={0,.8f,0};mat.at={-std::sin(angle),0,std::cos(angle)};mat.pos={float(k%3)*.02f,0,0};mat.flags=0;f->transform(&mat,rw::COMBINEREPLACE);
  if(k%7==0){g->lock(rw::Geometry::LOCKNORMALS);g->morphTargets[0].normals[0].x+=.03f;g->unlock();}if(k%11==0){g->lock(rw::Geometry::LOCKVERTICES);g->morphTargets[0].vertices[3].z+=.01f;g->unlock();}
  d.lighting({.1f,.2f,.3f},{.4f,.3f,.2f},{-.3f,float(k%4)*.1f,1});d.begin(c);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);d.normalCache(false);r.clear(0);a->render();auto image=r.pixels();auto depth=r.depth();d.normalCache(true);for(unsigned repeat=0;repeat<2;++repeat){r.clear(0);a->render();assert(image==r.pixels()&&depth==r.depth());}d.end();assert(d.normalCacheBytes()<=512*1024);
 }
 assert(d.cachedNormalsReused()>n&&d.cachedNormalsPrepared()>n&&d.normalCacheBytes()>0&&!d.unsupported());
 // Continuous geometry edits must release both bounds and normal arrays.
 for(unsigned k=0;k<5;++k){g->lock(rw::Geometry::LOCKVERTICES);g->morphTargets[0].vertices[2].z+=.001f;g->unlock();d.begin(c);r.clear(0);a->render();d.end();}assert(d.dynamicGeometryFallbacks()>0&&d.normalCacheBytes()==0);
 d.begin(c);d.normalCache(false);r.clear(0);a->render();auto dynamicImage=r.pixels();d.normalCache(true);r.clear(0);a->render();assert(dynamicImage==r.pixels());d.end();
 d.world()->removeAtomic(a);a->destroy();f->destroy();g->destroy();assert(d.normalCacheBytes()==0&&d.clusterCacheBytes()==0);puts("PASS 60 normal-cache frames: changing lights, translation/rotation/nonuniform scale, normals/vertices dirty, exact RGB/depth, dynamic fallback and release");}
