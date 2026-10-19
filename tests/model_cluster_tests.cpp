#include "software_device.hpp"
#include <cassert>
#include <random>
#include <cstdio>
using namespace soft;
int main(){Renderer r;SoftwareDevice d(r);d.hierarchicalBounds(false);d.highQuality(true);std::mt19937 random(2081);
 const unsigned side=24,vertices=(side+1)*(side+1),triangles=side*side*2;
 auto g=rw::Geometry::create(vertices,triangles,rw::Geometry::POSITIONS|rw::Geometry::PRELIT|rw::Geometry::MODULATE);
 auto mat=rw::Material::create();mat->color={210,180,170,255};g->matList.appendMaterial(mat);mat->destroy();
 for(unsigned y=0;y<=side;++y)for(unsigned x=0;x<=side;++x){unsigned i=y*(side+1)+x;g->morphTargets[0].vertices[i]={float(x*5)-60,float((x+y)%3)-1,float(y*5)+1};g->colors[i]={uint8_t(random()%256),uint8_t(random()%256),uint8_t(random()%256),255};}
 for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x){unsigned i=(y*side+x)*2,v=y*(side+1)+x;g->triangles[i]={{uint16_t(v),uint16_t(v+1),uint16_t(v+side+1)},0};g->triangles[i+1]={{uint16_t(v+side+1),uint16_t(v+1),uint16_t(v+side+2)},0};}
 auto a=rw::Atomic::create();auto frame=rw::Frame::create();a->setFrame(frame);a->setGeometry(g,0);
 for(unsigned n=0;n<80;++n){Camera camera;camera.position={float(int(n%9)*4-16),3,float(n%7)*5};camera.yaw=float(int(n%11)-5)*.17f;camera.farZ=20+n%4*20;camera.nearZ=.2f;
  rw::Matrix matrix;matrix.setIdentity();float angle=n*.025f;matrix.right={std::cos(angle)*1.2f,0,std::sin(angle)*1.2f};matrix.up={0,.8f,0};matrix.at={-std::sin(angle),0,std::cos(angle)};matrix.pos={float(int(n%5)-2)*10,0,0};matrix.flags=n%2?0:rw::Matrix::IDENTITY;frame->transform(&matrix,rw::COMBINEREPLACE);
  if(n%5==0){g->lock(rw::Geometry::LOCKPRELIGHT);g->colors[100].red^=255;g->unlock();}
  if(n%10==0){g->lock(rw::Geometry::LOCKVERTICES);g->morphTargets[0].vertices[300].y=float(n)*.01f;g->unlock();}
  d.begin(camera);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);rw::SetRenderState(rw::VERTEXALPHA,1);rw::SetRenderState(rw::ZWRITEENABLE,n%3!=0);
  d.modelClusters(false);r.clear(0);a->render();auto expected=r.pixels();auto depth=r.depth();
  d.modelClusters(true);d.fastClusterBounds(false);r.clear(0);a->render();assert(expected==r.pixels());assert(depth==r.depth());
  d.fastClusterBounds(true);r.clear(0);a->render();assert(expected==r.pixels());assert(depth==r.depth());d.end();
 }
 assert(d.culledClusters()>200&&d.skippedVertices()>1000);assert(d.clusterRebuilds()==8);assert(d.clusterCacheBytes()>0&&d.clusterCacheBytes()<=1024*1024);assert(!d.unsupported());
 // Polygon edits must invalidate cached bounds, including topology alone.
 g->lock(rw::Geometry::LOCKPOLYGONS);std::swap(g->triangles[0],g->triangles[1100]);g->unlock();Camera camera;camera.position={0,3,0};d.begin(camera);d.modelClusters(false);r.clear(0);a->render();auto image=r.pixels();auto depth=r.depth();d.modelClusters(true);r.clear(0);a->render();assert(image==r.pixels()&&depth==r.depth());assert(d.clusterRebuilds()==9);d.end();
 a->destroy();frame->destroy();g->destroy();assert(d.clusterCacheBytes()==0);
 d.begin(camera);r.clear(0);rw::cpu::Im3DVertex immediate[3];immediate[0].position={-1,-1,5};immediate[1].position={1,-1,5};immediate[2].position={0,1,5};
 rw::SetRenderState(rw::ZWRITEENABLE,1);rw::im3d::Transform(immediate,3,nullptr,rw::im3d::EVERYTHING);rw::im3d::RenderPrimitive(rw::PRIMTYPETRILIST);rw::im3d::End();assert(std::any_of(r.pixels().begin(),r.pixels().end(),[](uint16_t pixel){return pixel!=0;}));d.end();
 // Repeated streamed geometry destruction must release cache ownership/budget.
 for(unsigned n=0;n<30;++n){auto mesh=rw::Geometry::create(192,64,rw::Geometry::POSITIONS);auto material=rw::Material::create();mesh->matList.appendMaterial(material);material->destroy();for(unsigned i=0;i<192;++i)mesh->morphTargets[0].vertices[i]={float(i%3)-1,float(i%2),100};for(unsigned i=0;i<64;++i)mesh->triangles[i]={{uint16_t(i*3),uint16_t(i*3+1),uint16_t(i*3+2)},0};auto object=rw::Atomic::create();object->setGeometry(mesh,0);d.begin(camera);r.clear(0);object->render();d.end();object->destroy();mesh->destroy();assert(d.clusterCacheBytes()==0);}
 // Continuously edited rigid mesh (like wavy water): preserve the rendered
 // result and stop rebuilding static bounds after three consecutive updates.
 auto waves=rw::Geometry::create(192,64,rw::Geometry::POSITIONS|rw::Geometry::PRELIT);auto waveMaterial=rw::Material::create();waves->matList.appendMaterial(waveMaterial);waveMaterial->destroy();
 for(unsigned i=0;i<192;++i){waves->morphTargets[0].vertices[i]={float(i%3)-1,float(i%2),5};waves->colors[i]={140,180,220,255};}for(unsigned i=0;i<64;++i)waves->triangles[i]={{uint16_t(i*3),uint16_t(i*3+1),uint16_t(i*3+2)},0};
 auto waveObject=rw::Atomic::create();waveObject->setGeometry(waves,0);auto beforeRebuilds=d.clusterRebuilds();
 for(unsigned n=0;n<12;++n){waves->lock(rw::Geometry::LOCKVERTICES);for(unsigned i=0;i<192;++i)waves->morphTargets[0].vertices[i].y=float(i%2)+n*.03f;waves->unlock();
  d.begin(camera);d.modelClusters(false);r.clear(0);waveObject->render();auto expected=r.pixels();auto depth=r.depth();d.modelClusters(true);r.clear(0);waveObject->render();assert(expected==r.pixels()&&depth==r.depth());d.end();}
 assert(d.clusterRebuilds()-beforeRebuilds==3);assert(d.dynamicGeometryFallbacks()>=9);waveObject->destroy();waves->destroy();assert(d.clusterCacheBytes()==0);
 // Cache pressure: keep 19 large offscreen geometries alive. The last one
 // must fall back instead of growing the 1 MiB cache budget.
 std::vector<rw::Atomic*> objects;std::vector<rw::Geometry*> meshes;
 for(unsigned n=0;n<19;++n){auto mesh=rw::Geometry::create(64,65535,rw::Geometry::POSITIONS);auto material=rw::Material::create();mesh->matList.appendMaterial(material);material->destroy();
  for(unsigned i=0;i<64;++i)mesh->morphTargets[0].vertices[i]={float(i%3)-1,0,100};for(unsigned i=0;i<65535;++i)mesh->triangles[i]={{0,1,2},0};
  auto object=rw::Atomic::create();object->setGeometry(mesh,0);d.begin(camera);r.clear(0);object->render();d.end();objects.push_back(object);meshes.push_back(mesh);assert(d.clusterCacheBytes()<=1024*1024);
 }
 assert(d.clusterCacheBytes()>1000000);for(unsigned i=0;i<objects.size();++i){objects[i]->destroy();meshes[i]->destroy();}assert(d.clusterCacheBytes()==0);assert(!d.unsupported());
 printf("PASS 80 full-frame model cluster comparisons, masks/transparency/dirty vertices/topology/stale matrix flags/streamed ownership; culled=%llu skipped=%llu\n",(unsigned long long)d.culledClusters(),(unsigned long long)d.skippedVertices());
}
