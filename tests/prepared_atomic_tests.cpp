#include "software_device.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <random>
#include <limits>
using namespace soft;
int main(){Renderer r;SoftwareDevice d(r);r.smallDepthEnabled(true);d.earlyVisibility(true);std::mt19937 rng(2301);uint64_t verified=0;
 auto raster=rw::Raster::create(16,8,32,rw::Raster::TEXTURE|rw::Raster::C8888);auto tex=rw::Texture::create(raster);
 auto g=rw::Geometry::create(9,8,rw::Geometry::POSITIONS|rw::Geometry::PRELIT|rw::Geometry::MODULATE|rw::Geometry::TEXTURED);
 for(unsigned i=0;i<3;++i){auto m=rw::Material::create();m->setTexture(tex);g->matList.appendMaterial(m);m->destroy();}
 for(unsigned y=0;y<2;++y)for(unsigned x=0;x<2;++x){unsigned t=(y*2+x)*2,v=y*3+x;g->triangles[t]={{uint16_t(v),uint16_t(v+1),uint16_t(v+3)},uint16_t(t%3)};g->triangles[t+1]={{uint16_t(v+3),uint16_t(v+1),uint16_t(v+4)},uint16_t((t+1)%3)};}
 auto a=rw::Atomic::create();a->setGeometry(g,0);
 for(unsigned n=0;n<900;++n){r.renderScale(n%4+1);d.highQuality(n%2);Camera c;c.position={float(int(rng()%20)-10)/10,0,-1};c.yaw=float(int(rng()%20)-10)/50;c.farZ=30;
  for(unsigned y=0;y<3;++y)for(unsigned x=0;x<3;++x){unsigned i=y*3+x;float z=n%11==0?-.5f:n%13==0?35.f:float(2+rng()%12);g->morphTargets[0].vertices[i]={float(int(x)-1)*4,float(int(y)-1)*3,z};g->texCoords[0][i]={float(int(rng()%400)-200)/100,float(int(rng()%400)-200)/100};g->colors[i]={uint8_t(rng()),uint8_t(rng()),uint8_t(rng()),uint8_t(n%5?255:180)};}
  if(n%47==0)g->morphTargets[0].vertices[4].x=std::numeric_limits<float>::quiet_NaN();
  if(n%53==0)g->texCoords[0][2].v=1e6f;
  if(n%71==0)g->texCoords[0][8].u=std::numeric_limits<float>::quiet_NaN();
  for(unsigned i=0;i<3;++i)g->matList.materials[i]->color={uint8_t(rng()),uint8_t(rng()),uint8_t(rng()),uint8_t(n%7?255:100)};
  auto bytes=raster->lock(0,rw::Raster::LOCKWRITE);for(unsigned y=0;y<8;++y)for(unsigned x=0;x<16;++x){auto px=bytes+y*raster->stride+x*4;px[0]=rng();px[1]=rng();px[2]=rng();px[3]=n%3?255:rng();}raster->unlock(0);
  g->numTexCoordSets=n%19==0?0:1;
  tex->setAddressU(rw::Texture::Addressing(n%4+1));tex->setAddressV(rw::Texture::Addressing((n/4)%4+1));d.begin(c);rw::SetRenderState(rw::CULLMODE,n%3+1);rw::SetRenderState(rw::VERTEXALPHA,n%3==0);rw::SetRenderState(rw::ZWRITEENABLE,n%8!=0);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAGREATEREQUAL);rw::SetRenderState(rw::ALPHATESTREF,n%2?1:128);
  // Saved texture/address state differs from material state and must survive.
  rw::SetRenderStatePtr(rw::TEXTURERASTER,nullptr);unsigned savedAddress=n%89?3:0;rw::SetRenderState(rw::TEXTUREADDRESS,savedAddress);
  uint16_t bg=rng();d.directAtomic(false);d.groupedColors(false);d.preparedScreenValidation(false);d.atomicTransfer(false);d.preparedAtomics(false);r.opaqueTexturesEnabled(false);r.clear(bg);a->render();auto image=r.pixels();auto depth=r.depth();d.directAtomic(true);d.groupedColors(true);d.preparedScreenValidation(true);d.atomicTransfer(true);d.preparedAtomics(true);r.opaqueTexturesEnabled(true);r.clear(bg);a->render();assert(image==r.pixels()&&depth==r.depth());verified+=r.stats().verifiedScreenVertices;
  assert(!rw::GetRenderStatePtr(rw::TEXTURERASTER));assert(rw::GetRenderState(rw::TEXTUREADDRESSU)==savedAddress&&rw::GetRenderState(rw::TEXTUREADDRESSV)==savedAddress);d.end();
 }
 // Prelit-only geometry: BAKED preserves the full SMOOTH gradient at FLAT lighting cost.
 auto baked=rw::Geometry::create(3,1,rw::Geometry::POSITIONS|rw::Geometry::PRELIT|rw::Geometry::MODULATE);auto bm=rw::Material::create();baked->matList.appendMaterial(bm);bm->destroy();baked->triangles[0]={{0,1,2},0};baked->morphTargets[0].vertices[0]={-2,-2,5};baked->morphTargets[0].vertices[1]={2,-2,5};baked->morphTargets[0].vertices[2]={0,2,5};baked->colors[0]={0,0,0,255};baked->colors[1]={255,255,255,255};baked->colors[2]={50,80,120,255};auto ba=rw::Atomic::create();ba->setGeometry(baked,0);Camera bc;d.begin(bc);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::VERTEXALPHA,0);rw::SetRenderState(rw::ZWRITEENABLE,1);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
 d.highQuality(true);d.bakedLighting(false);r.clear(0);ba->render();auto smooth=r.pixels();auto bd=r.depth();d.highQuality(false);d.bakedLighting(true);r.clear(0);ba->render();assert(smooth==r.pixels()&&bd==r.depth());d.bakedLighting(false);r.clear(0);ba->render();assert(smooth!=r.pixels());d.end();ba->destroy();baked->destroy();
 assert(d.directAtomicVertices()>1000);assert(verified>1000);assert(d.reusedMaterials()>100&&d.reusedFlatProjections()>100);assert(!d.unsupported());a->destroy();g->destroy();tex->destroy();puts("PASS 900 layered/rotated/clipped moving meshes, raster locks, materials/alpha/cull/addresses, exact RGB+depth and restored state");
}
