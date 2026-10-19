#include "software_device.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <array>
using namespace soft;
int main(int argc,char**){std::array<bool,2> modes=argc>1?std::array<bool,2>{{true,false}}:std::array<bool,2>{{false,true}};Renderer r;r.depthRowCullingEnabled(false);r.batchCountersEnabled(false);r.renderScale(3);SoftwareDevice d(r);unsigned side=40;
 auto raster=rw::Raster::create(32,32,16,rw::Raster::TEXTURE|rw::Raster::C565);auto bytes=raster->lock(0,rw::Raster::LOCKWRITE);for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x){uint16_t c=rgb(x*8,y*8,150);std::memcpy(bytes+y*raster->stride+x*2,&c,2);}raster->unlock(0);auto tex=rw::Texture::create(raster);tex->setAddressU(rw::Texture::WRAP);tex->setAddressV(rw::Texture::WRAP);
 auto g=rw::Geometry::create((side+1)*(side+1),side*side*2,rw::Geometry::POSITIONS|rw::Geometry::PRELIT|rw::Geometry::MODULATE|rw::Geometry::TEXTURED);
 for(unsigned i=0;i<2;++i){auto mat=rw::Material::create();mat->setTexture(tex);mat->color={uint8_t(180+i*40),190,180,255};g->matList.appendMaterial(mat);mat->destroy();}
 for(unsigned y=0;y<=side;++y)for(unsigned x=0;x<=side;++x){unsigned i=y*(side+1)+x;g->morphTargets[0].vertices[i]={float(x)*.15f-3,float(y)*.1f-2,8};g->colors[i]={uint8_t(70+x*3),uint8_t(70+y*3),150,255};g->texCoords[0][i]={float(x)/10,float(y)/10};}
 for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x){unsigned t=(y*side+x)*2,v=y*(side+1)+x;g->triangles[t]={{uint16_t(v),uint16_t(v+1),uint16_t(v+side+1)},uint16_t(y%2)};g->triangles[t+1]={{uint16_t(v+side+1),uint16_t(v+1),uint16_t(v+side+2)},uint16_t(y%2)};}
 auto a=rw::Atomic::create();a->setGeometry(g,0);Camera c;c.position={0,0,0};c.yaw=.1f;
 for(bool smooth:{false,true})for(bool enabled:modes){d.highQuality(smooth);d.preparedAtomics(enabled);d.begin(c);rw::SetRenderState(rw::VERTEXALPHA,0);auto beforeMaterials=d.reusedMaterials(),beforeProjections=d.reusedFlatProjections();auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<300;++i){r.clear(0);a->render();}auto end=std::chrono::steady_clock::now();unsigned long long hash=1469598103934665603ull;for(auto p:r.pixels()){hash^=p;hash*=1099511628211ull;}printf("prepared_atomic=%d smooth=%d %.4f ms/frame hash=%llx materials_reused=%llu flat_projection_reused=%llu\n",enabled,smooth,std::chrono::duration<double,std::milli>(end-start).count()/300,hash,(unsigned long long)(d.reusedMaterials()-beforeMaterials),(unsigned long long)(d.reusedFlatProjections()-beforeProjections));d.end();}
 a->destroy();g->destroy();tex->destroy();return d.unsupported()?1:0;
}
