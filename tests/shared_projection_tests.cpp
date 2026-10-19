#include "software_device.hpp"
#include <cassert>
#include <random>
#include <cstdio>
int main(){using namespace soft;Renderer r;SoftwareDevice d(r);d.highQuality(true);Camera camera;camera.position={1,2,-1};camera.yaw=.2f;camera.farZ=60;d.begin(camera);
 rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::VERTEXALPHA,1);
 auto g=rw::Geometry::create(4,2,rw::Geometry::POSITIONS|rw::Geometry::PRELIT|rw::Geometry::MODULATE);
 for(unsigned i=0;i<2;++i){auto m=rw::Material::create();m->color={uint8_t(150+i*90),uint8_t(210-i*80),180,255};g->matList.appendMaterial(m);m->destroy();}
 g->triangles[0]={{0,1,2},0};g->triangles[1]={{2,1,3},1};auto a=rw::Atomic::create();a->setGeometry(g,0);std::mt19937 random(1931);
 for(unsigned n=0;n<600;++n){float z=n%7==0?.02f:3+random()%70;
  rw::V3d positions[4]={{-2,-2,z},{2,-2,z+.5f},{-2,2,z},{2,2,z+.5f}};
  for(unsigned i=0;i<4;++i){g->morphTargets[0].vertices[i]=positions[i];g->colors[i]={uint8_t(random()%256),uint8_t(random()%256),uint8_t(random()%256),uint8_t(n%3?255:180)};}
  r.clear(0);a->render();auto image=r.pixels();auto depth=r.depth();r.clear(0);
  for(unsigned t=0;t<2;++t){rw::cpu::Im3DVertex v[3];auto tri=g->triangles[t];auto tint=g->matList.materials[t]->color;
   for(unsigned j=0;j<3;++j){auto c=g->colors[tri.v[j]];v[j].position=positions[tri.v[j]];v[j].setColor(unsigned(c.red)*tint.red/255,unsigned(c.green)*tint.green/255,unsigned(c.blue)*tint.blue/255,c.alpha);}
   rw::im3d::Transform(v,3,nullptr,rw::im3d::EVERYTHING);rw::im3d::RenderPrimitive(rw::PRIMTYPETRILIST);rw::im3d::End();}
  assert(image==r.pixels());assert(depth==r.depth());
 }
 assert(!d.unsupported());a->destroy();g->destroy();d.end();puts("PASS shared projection versus unclipped/clipped Im3D, 600 meshes, moving vertices, different materials/alpha, rotated camera");
}
