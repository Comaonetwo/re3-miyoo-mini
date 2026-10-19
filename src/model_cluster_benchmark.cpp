#include "software_device.hpp"
#include <chrono>
#include <cstdio>
using namespace soft;
int main(){Renderer r;r.renderScale(3);SoftwareDevice d(r);d.highQuality(true);unsigned side=64,vertices=(side+1)*(side+1),triangles=side*side*2;
 auto g=rw::Geometry::create(vertices,triangles,rw::Geometry::POSITIONS|rw::Geometry::PRELIT);auto material=rw::Material::create();g->matList.appendMaterial(material);material->destroy();
 for(unsigned y=0;y<=side;++y)for(unsigned x=0;x<=side;++x){unsigned i=y*(side+1)+x;g->morphTargets[0].vertices[i]={float(x*4)-128,0,float(y*4)};g->colors[i]={uint8_t(80+x*2),uint8_t(70+y*2),140,255};}
 for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x){unsigned i=(y*side+x)*2,v=y*(side+1)+x;g->triangles[i]={{uint16_t(v),uint16_t(v+1),uint16_t(v+side+1)},0};g->triangles[i+1]={{uint16_t(v+side+1),uint16_t(v+1),uint16_t(v+side+2)},0};}
 auto a=rw::Atomic::create();a->setGeometry(g,0);Camera camera;camera.position={40,3,5};camera.yaw=.65f;
 for(unsigned distance:{40u,80u,100u})for(bool enabled:{false,true}){camera.farZ=distance;d.modelClusters(enabled);auto before=d.skippedVertices();auto start=std::chrono::steady_clock::now();
  for(unsigned f=0;f<300;++f){d.begin(camera);rw::SetRenderState(rw::VERTEXALPHA,1);r.clear(0);a->render();d.end();}
  unsigned long long hash=1469598103934665603ull;for(auto p:r.pixels()){hash^=p;hash*=1099511628211ull;}
  printf("model clusters=%d distance=%u %.4f ms/frame skipped_vertices=%llu cache_bytes=%llu hash=%llx\n",enabled,distance,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/300,(unsigned long long)(d.skippedVertices()-before),(unsigned long long)d.clusterCacheBytes(),hash);
 }
 a->destroy();g->destroy();return d.unsupported()?1:0;
}
