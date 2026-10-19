#include "software_device.hpp"
#include <cassert>
#include <cstdio>
#include <random>
#include <limits>
using namespace soft;
static bool attach(){return true;}
int main(){Renderer r;SoftwareDevice d(r,attach);std::mt19937 rng(4401);r.smallDepthEnabled(true);
 auto raster=rw::Raster::create(8,8,32,rw::Raster::TEXTURE|rw::Raster::C8888);auto tex=rw::Texture::create(raster);
 auto g=rw::Geometry::create(192,64,rw::Geometry::POSITIONS|rw::Geometry::TEXTURED|rw::Geometry::PRELIT|rw::Geometry::NORMALS|rw::Geometry::LIGHT|rw::Geometry::MODULATE);
 for(unsigned j=0;j<3;++j){auto m=rw::Material::create();m->setTexture(tex);m->surfaceProps={.2f,0,.7f};g->matList.appendMaterial(m);m->destroy();}
 auto a=rw::Atomic::create();a->setGeometry(g,0);Camera cam;cam.position={0,0,0};cam.farZ=120;
 for(unsigned n=0;n<420;++n){r.renderScale(n%4+1);d.earlyWideVisibility(n%2);d.highQuality(n%3==0);d.bakedLighting(n%3==1);d.flatFading(true);d.vertexColorCache(n%2);d.groupedColors(n%3==0);d.atomicTransfer(n%2);d.preparedScreenValidation(n%19!=0);d.preparedAtomics(n%23!=0);d.worldTextures(n%31!=0);d.effectFiltering(n%2);
  g->lock(rw::Geometry::LOCKVERTICES|rw::Geometry::LOCKNORMALS|rw::Geometry::LOCKPRELIGHT|rw::Geometry::LOCKPOLYGONS);
  for(unsigned t=0;t<64;++t){float x=float(int(rng()%600)-300)/10,y=float(int(rng()%400)-200)/10,z=float(rng()%70+10),s=t%3?float(rng()%20+1)/40:8.f;
   for(unsigned j=0;j<3;++j){unsigned i=t*3+j;g->morphTargets[0].vertices[i]={x+(j==1?s:0),y+(j==2?s:0),z+(t%7==0?j*.3f:0)};g->morphTargets[0].normals[i]={0,0,-1};g->texCoords[0][i]={float(int(rng()%100)-50)/10,float(int(rng()%100)-50)/10};g->colors[i]={uint8_t(rng()),uint8_t(rng()),uint8_t(rng()),uint8_t(n%3?255:rng())};}
   g->triangles[t]={{uint16_t(t*3),uint16_t(t*3+1),uint16_t(t*3+2)},uint16_t(t%3)};if(t%2)std::swap(g->triangles[t].v[1],g->triangles[t].v[2]);
  }
  if(n%29==0)g->texCoords[0][1].u=std::numeric_limits<float>::quiet_NaN();
  if(n%37==0)g->morphTargets[0].vertices[7].z=-.1f; // clipped fallback
  g->unlock();
  for(unsigned j=0;j<3;++j)g->matList.materials[j]->color={uint8_t(160+(n%5?j:0)*20),200,180,uint8_t(n%4?255:128)};
  auto bytes=raster->lock(0,rw::Raster::LOCKWRITE);for(unsigned i=0;i<64;++i){bytes[i*4]=uint8_t(rng());bytes[i*4+1]=uint8_t(rng());bytes[i*4+2]=uint8_t(rng());bytes[i*4+3]=uint8_t(n%3?255:rng());}raster->unlock(0);
  tex->setAddressU(rw::Texture::Addressing(n%4+1));tex->setAddressV(rw::Texture::Addressing((n/4)%4+1));d.begin(cam);
  rw::SetRenderState(rw::CULLMODE,n%3+1);rw::SetRenderState(rw::ZTESTENABLE,n%13!=0);rw::SetRenderState(rw::ZWRITEENABLE,n%3!=0);rw::SetRenderState(rw::VERTEXALPHA,n%2);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAGREATEREQUAL);rw::SetRenderState(rw::ALPHATESTREF,n%4==0?0:n%4==1?1:n%4==2?128:255);rw::SetRenderState(rw::TEXTUREFILTER,n%2?rw::Texture::LINEAR:rw::Texture::NEAREST);
  const unsigned sources[]={rw::BLENDSRCALPHA,rw::BLENDSRCALPHA,rw::BLENDONE,rw::BLENDZERO,rw::BLENDZERO};const unsigned dests[]={rw::BLENDINVSRCALPHA,rw::BLENDONE,rw::BLENDONE,rw::BLENDINVSRCCOLOR,rw::BLENDONE};rw::SetRenderState(rw::SRCBLEND,sources[n%5]);rw::SetRenderState(rw::DESTBLEND,dests[n%5]);
  auto scene=[&](bool early){r.lazyDepthEnabled(early&&n%3!=0);d.directAtomic(early);d.earlyVisibility(early);r.clear(12345);ScreenState s;s.zTest=s.zWrite=true;s.perspectiveColor=true;
   ScreenVertex wall[4]={{-20,-20,.04f,0,0,100,140,180,255,0},{340,-20,.06f,0,0,100,140,180,255,0},{-20,260,.04f,0,0,100,140,180,255,0},{340,260,.06f,0,0,100,140,180,255,0}};
   ScreenVertex q[3]={wall[0],wall[1],wall[2]},v[3]={wall[2],wall[1],wall[3]};r.screenTriangle(q,nullptr,s);r.screenTriangle(v,nullptr,s);
   if(n%17==0){for(auto& p:q)p.reciprocalZ=.01f;for(unsigned k=0;k<300;++k)r.screenTriangle(q,nullptr,s);}
   if(n%41==0)r.clearUiDepth();
   if(n%43==0)r.beginSky(120);if(n%47==0)r.beginRain(120);if(n%53==0)r.beginHud(640);
   a->render();r.endRain();r.endSky();
  };
  scene(false);auto image=r.pixels();auto depth=r.depth();auto count=r.stats().pixels;scene(true);assert(image==r.pixels()&&depth==r.depth()&&count==r.stats().pixels);d.end();
 }
 r.lazyDepthEnabled(false);
 // Large shaded triangles must still drive the original snapshot schedule.
 // Reusing a prepared Z plane is not the same as having tested occlusion.
 r.renderScale(1);d.earlyWideVisibility(false);d.highQuality(true);d.bakedLighting(false);d.preparedScreenValidation(true);d.preparedAtomics(true);d.worldTextures(true);d.effectFiltering(false);
 g->lock(rw::Geometry::LOCKVERTICES|rw::Geometry::LOCKPRELIGHT);
 for(unsigned t=0;t<64;++t)for(unsigned j=0;j<3;++j){unsigned i=t*3+j;g->morphTargets[0].vertices[i]={j==1?5.f:-5.f,j==2?5.f:-5.f,30.f};g->colors[i]={uint8_t(j*80),uint8_t(180-j*50),uint8_t(40+j*60),255};g->texCoords[0][i]={float(j==1),float(j==2)};}
 g->unlock();for(unsigned j=0;j<3;++j)g->matList.materials[j]->color={255,255,255,255};
 d.begin(cam);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::ZTESTENABLE,1);rw::SetRenderState(rw::ZWRITEENABLE,1);rw::SetRenderState(rw::VERTEXALPHA,0);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);rw::SetRenderState(rw::TEXTUREFILTER,rw::Texture::NEAREST);
 auto layered=[&](bool early){d.directAtomic(early);d.earlyVisibility(early);r.clear(12345);ScreenState s;s.zTest=s.zWrite=true;s.perspectiveColor=true;ScreenVertex wall[4]={{-20,-20,.2f,0,0,100,140,180,255,0},{340,-20,.2f,0,0,100,140,180,255,0},{-20,260,.2f,0,0,100,140,180,255,0},{340,260,.2f,0,0,100,140,180,255,0}};ScreenVertex q[3]={wall[0],wall[1],wall[2]},v[3]={wall[2],wall[1],wall[3]};r.screenTriangle(q,nullptr,s);r.screenTriangle(v,nullptr,s);a->render();};
 layered(false);auto image=r.pixels();auto depth=r.depth();auto snapshots=r.stats().occlusionBuilds,checks=r.stats().occlusionChecks;assert(snapshots>0);layered(true);assert(image==r.pixels()&&depth==r.depth()&&r.stats().occlusionBuilds==snapshots);
 d.earlyWideVisibility(true);layered(true);assert(image==r.pixels()&&depth==r.depth()&&r.stats().occlusionBuilds==snapshots&&r.stats().occlusionChecks==checks);
 // FLAT previously had no large-face snapshot. The optional wide query must
 // build bounded minima without changing pixels or adding a second late query.
 d.highQuality(false);layered(false);image=r.pixels();depth=r.depth();assert(r.stats().occlusionBuilds==0);layered(true);assert(image==r.pixels()&&depth==r.depth()&&r.stats().occlusionBuilds>0&&r.stats().occlusionBuilds<=2);d.end();
 assert(d.earlyVisibilityQueries()>1000&&d.earlyVisibilityHidden()>1000);a->destroy();g->destroy();tex->destroy();assert(d.clusterCacheBytes()==0&&d.vertexColorCacheBytes()==0);
 printf("PASS 420 paired mutable atomic frames: exact RGB/depth/fragments across quality, textures, alpha/blends, culling, target guards, clipping, masks and cache invalidation; queries=%llu hidden=%llu\n",(unsigned long long)d.earlyVisibilityQueries(),(unsigned long long)d.earlyVisibilityHidden());
}
