#include "software_device.hpp"
#include <cassert>
#include <cstdio>
using namespace soft;
static bool attach(){return true;}
int main(){Renderer r;SoftwareDevice d(r,attach);r.smallDepthEnabled(true);d.earlyVisibility(true);d.directAtomic(true);
 auto raster=rw::Raster::create(8,8,32,rw::Raster::TEXTURE|rw::Raster::C8888);auto tex=rw::Texture::create(raster);
 auto g=rw::Geometry::create(1536,512,rw::Geometry::POSITIONS|rw::Geometry::TEXTURED|rw::Geometry::PRELIT|rw::Geometry::MODULATE);
 auto m=rw::Material::create();m->setTexture(tex);g->matList.appendMaterial(m);m->destroy();
 for(unsigned t=0;t<512;++t)for(unsigned j=0;j<3;++j){unsigned i=t*3+j;g->triangles[t].v[j]=i;g->triangles[t].matId=0;
  g->morphTargets[0].vertices[i]={j==0?-2.f:j==1?2.f:0.f,j==2?2.f:-2.f,25.f};g->texCoords[0][i]={j==1?1.f:0.f,j==2?1.f:0.f};g->colors[i]={220,240,160,255};}
 auto a=rw::Atomic::create();a->setGeometry(g,0);Camera cam;cam.position={0,0,0};cam.farZ=100;uint64_t hidden=0,tiles=0;
 for(unsigned n=0;n<72;++n){r.renderScale(n%4+1);d.highQuality(n%3==0);d.bakedLighting(n%3==1);
  auto bytes=raster->lock(0,rw::Raster::LOCKWRITE);for(unsigned i=0;i<64;++i){bytes[i*4]=40;bytes[i*4+1]=200;bytes[i*4+2]=60;bytes[i*4+3]=n%3||i%3?255:0;}raster->unlock(0);
  d.begin(cam);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::ZTESTENABLE,1);rw::SetRenderState(rw::ZWRITEENABLE,1);rw::SetRenderState(rw::VERTEXALPHA,0);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAGREATEREQUAL);rw::SetRenderState(rw::ALPHATESTREF,128);rw::SetRenderState(rw::TEXTUREFILTER,rw::Texture::NEAREST);
  auto fill=[&](float z,bool test=true,bool screen=false){ScreenState state;state.zTest=test;state.zWrite=true;state.screenDepth=screen;
   ScreenVertex v[4]={{-20,-20,z,0,0,80,100,120,255,1-z},{340,-20,z,0,0,80,100,120,255,1-z},{-20,260,z,0,0,80,100,120,255,1-z},{340,260,z,0,0,80,100,120,255,1-z}};
   ScreenVertex q[3]={v[0],v[1],v[2]},w[3]={v[2],v[1],v[3]};r.screenTriangle(q,nullptr,state);r.screenTriangle(w,nullptr,state);
  };
  // Prime the adaptive budget with a dense frame, independently of this
  // case's changing scale/holes. Each invalidation case must exercise a
  // populated cache, rather than accidentally testing an inactive probe slot.
  r.lazyDepthEnabled(true);unsigned warm=0;
  for(;warm<12;++warm){r.clear(0);fill(.2f);a->render();if(r.stats().lazyHidden>100)break;}
  assert(warm<12);
  auto scene=[&](bool enabled){r.lazyDepthEnabled(enabled);r.clear(12345);fill(.2f);
   // A tiny depth hole away from the quick centre probe must retain samples.
   if(n%4==0){ScreenState hole;hole.zTest=false;hole.zWrite=true;ScreenVertex v[4]={{156,125,0,0,0,0,0,0,255,0},{157,125,0,0,0,0,0,0,255,0},{156,127,0,0,0,0,0,0,255,0},{157,127,0,0,0,0,0,0,255,0}};ScreenVertex q[3]={v[0],v[1],v[2]},w[3]={v[2],v[1],v[3]};r.screenTriangle(q,nullptr,hole);r.screenTriangle(w,nullptr,hole);}
   a->render();
   if(enabled){hidden+=r.stats().lazyHidden;tiles+=r.stats().lazyTiles;}
   switch(n%9){
    case 0:break; // immutable cache, increasing depth only
    case 1:r.clearUiDepth();break; // clear only depth, retain image
    case 2:r.clearBuffers(6789,false,true);break;
    case 3:fill(.01f,false);break; // unconditional world writes lower depth
    case 4:fill(.01f,false,true);break; // screen-depth writes lower depth
    case 5:r.beginHud(640);r.clearUiDepth();r.clearBuffers(0,false,false);break;
    case 6:for(unsigned k=0;k<80;++k)fill(.3f);break; // legacy full scans may refresh lazy cells safely
    case 7:r.beginSky(120);r.endSky();break;
    case 8:if(r.beginRain(120))r.cancelRain();break;
   }
   a->render();
  };
  scene(false);auto rgb=r.pixels();auto z=r.depth();auto count=r.stats().pixels;scene(true);assert(rgb==r.pixels()&&z==r.depth()&&count==r.stats().pixels);d.end();
 }
 assert(hidden>1000&&tiles>100);assert(!d.unsupported());a->destroy();g->destroy();tex->destroy();assert(d.clusterCacheBytes()==0);
 printf("PASS 72 paired multi-pass cache scenes: exact RGB/depth/fragments after depth clears, lower-depth writes, screen/HUD/sky/rain swaps and scale/quality changes; hidden=%llu tiles=%llu\n",(unsigned long long)hidden,(unsigned long long)tiles);
}
