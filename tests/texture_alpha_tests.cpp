#include "software_device.hpp"
#include <cassert>
#include <cstdio>
using namespace soft;
static bool attach(){return true;}
int main(){Renderer r;SoftwareDevice d(r,attach);Camera c;c.position={0,0,0};
 auto raster=rw::Raster::create(8,8,32,rw::Raster::TEXTURE|rw::Raster::C8888);auto tex=rw::Texture::create(raster);auto g=rw::Geometry::create(3,1,rw::Geometry::POSITIONS|rw::Geometry::TEXTURED|rw::Geometry::PRELIT|rw::Geometry::MODULATE);auto m=rw::Material::create();m->setTexture(tex);g->matList.appendMaterial(m);m->destroy();g->triangles[0]={{0,1,2},0};g->morphTargets[0].vertices[0]={-3,-3,5};g->morphTargets[0].vertices[1]={3,-3,5};g->morphTargets[0].vertices[2]={0,3,5};for(unsigned i=0;i<3;++i){g->colors[i]={255,255,255,255};g->texCoords[0][i]={.5f,.5f};}auto a=rw::Atomic::create();a->setGeometry(g,0);
 for(unsigned alpha:{0u,1u,64u,128u,254u,255u})for(bool smooth:{false,true})for(bool transfer:{false,true}){
  auto b=raster->lock(0,rw::Raster::LOCKWRITE);for(unsigned i=0;i<64;++i){b[4*i]=b[4*i+1]=b[4*i+2]=0;b[4*i+3]=alpha;}raster->unlock(0);d.highQuality(smooth);d.atomicTransfer(transfer);d.begin(c);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAGREATEREQUAL);rw::SetRenderState(rw::ALPHATESTREF,1);rw::SetRenderState(rw::SRCBLEND,rw::BLENDSRCALPHA);rw::SetRenderState(rw::DESTBLEND,rw::BLENDINVSRCALPHA);rw::SetRenderState(rw::VERTEXALPHA,0);rw::SetRenderState(rw::ZWRITEENABLE,1);rw::SetRenderState(rw::ZTESTENABLE,1);
  d.textureAlpha(true);r.clear(rgb(0,0,255));a->render();auto image=r.pixels();auto depth=r.depth();auto center=image[120*320+160];assert(center==(alpha?rgb(0,0,(255-alpha)):rgb(0,0,255)));
  r.effectSpansEnabled(false);r.clear(rgb(0,0,255));a->render();assert(image==r.pixels()&&depth==r.depth());r.effectSpansEnabled(true);
  d.textureAlpha(false);rw::SetRenderState(rw::VERTEXALPHA,1);r.clear(rgb(0,0,255));a->render();assert(image==r.pixels()&&depth==r.depth());rw::SetRenderState(rw::VERTEXALPHA,0);
  if(alpha>0&&alpha<248){r.clear(rgb(0,0,255));a->render();assert(r.pixels()[120*320+160]==0&&r.pixels()!=image);}d.end();
 }
 assert(d.textureAlphaTriangles()>0&&d.transferredAtomicVertices()>0&&!d.unsupported());a->destroy();g->destroy();tex->destroy();puts("PASS texture alpha blends without vertex alpha; FLAT/SMOOTH, staging, raster dirty, alpha rejection and reference depth");}
