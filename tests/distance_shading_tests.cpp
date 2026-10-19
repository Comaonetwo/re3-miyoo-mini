#include "software_device.hpp"
#include <cassert>
#include <cstdio>
using namespace soft;
static bool attach(){return true;}
int main(){Renderer r;SoftwareDevice d(r,attach);Camera c;c.position={0,0,0};auto raster=rw::Raster::create(4,4,32,rw::Raster::TEXTURE|rw::Raster::C8888);auto bytes=raster->lock(0,rw::Raster::LOCKWRITE);for(unsigned i=0;i<16;++i){bytes[4*i]=120+i*3;bytes[4*i+1]=170;bytes[4*i+2]=210;bytes[4*i+3]=255;}raster->unlock(0);auto t=rw::Texture::create(raster);auto g=rw::Geometry::create(3,1,rw::Geometry::POSITIONS|rw::Geometry::TEXTURED|rw::Geometry::PRELIT|rw::Geometry::MODULATE);auto m=rw::Material::create();m->setTexture(t);g->matList.appendMaterial(m);m->destroy();g->triangles[0]={{0,1,2},0};g->morphTargets[0].vertices[0]={-2,-2,5};g->morphTargets[0].vertices[1]={2,-2,7};g->morphTargets[0].vertices[2]={0,2,5};for(unsigned i=0;i<3;++i)g->texCoords[0][i]={i==1?1.f:0.f,i==2?1.f:0.f};for(unsigned i=0;i<3;++i){g->morphTargets[0].vertices[i].x*=6;g->morphTargets[0].vertices[i].y*=6;g->morphTargets[0].vertices[i].z+=30;}auto a=rw::Atomic::create();a->setGeometry(g,0);
 for(unsigned alpha:{0u,1u,64u,128u,254u,255u})for(bool baked:{false,true}){rw::RGBA colors[3]={{130,150,190,255},{145,160,210,255},{150,140,205,255}};for(unsigned i=0;i<3;++i)g->colors[i]=colors[i];g->matList.materials[0]->color.alpha=alpha;d.highQuality(true);d.bakedLighting(baked);d.begin(c);rw::SetRenderState(rw::VERTEXALPHA,1);rw::SetRenderState(rw::SRCBLEND,rw::BLENDSRCALPHA);rw::SetRenderState(rw::DESTBLEND,rw::BLENDINVSRCALPHA);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAGREATEREQUAL);rw::SetRenderState(rw::ALPHATESTREF,1);rw::SetRenderState(rw::ZTESTENABLE,1);rw::SetRenderState(rw::ZWRITEENABLE,1);d.distanceShading(true);r.clear(12243);a->render();auto image=r.pixels(),depthImage=image;auto depth=r.depth();d.distanceShading(false);
 if(!baked)for(unsigned i=0;i<3;++i)g->colors[i]={(130+145+150)/3,(150+160+140)/3,(190+210+205)/3,255};r.clear(12243);a->render();assert(image==r.pixels()&&depth==r.depth());d.end();}

 d.bakedLighting(false);d.highQuality(true);
 for(unsigned variant=0;variant<4;++variant){
  for(unsigned i=0;i<3;++i){g->morphTargets[0].vertices[i].z=35;g->colors[i]={uint8_t(130+i*10),150,200,255};}
  rw::Clump* clump=nullptr;
  if(variant==0)g->morphTargets[0].vertices[0].z=10;
  if(variant==1)g->colors[0].red=0;
  if(variant==2)g->colors[0].alpha=100;
  if(variant==3){clump=rw::Clump::create();clump->addAtomic(a);}
  d.begin(c);d.distanceShading(false);r.clear(12243);a->render();auto original=r.pixels();auto originalDepth=r.depth();
  auto before=d.distanceShadingTriangles();d.distanceShading(true);r.clear(12243);a->render();
  assert(original==r.pixels()&&originalDepth==r.depth()&&before==d.distanceShadingTriangles());d.end();
  if(clump){clump->removeAtomic(a);clump->destroy();}
 }
 assert(d.distanceShadingTriangles()>0&&!d.unsupported());a->destroy();g->destroy();t->destroy();puts("PASS distant low-contrast shading equals reference average; alpha/depth preserved; BAKED unchanged");}
