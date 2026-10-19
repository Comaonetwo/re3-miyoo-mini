#include "software_device.hpp"
#include <cassert>
int main(){using namespace soft;Renderer r;SoftwareDevice device(r);Camera camera;camera.position={0,0,0};camera.farZ=50;device.begin(camera);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
auto g=rw::Geometry::create(3,1,rw::Geometry::POSITIONS|rw::Geometry::PRELIT);auto material=rw::Material::create();g->triangles[0].matId=g->matList.appendMaterial(material);material->destroy();for(unsigned i=0;i<3;++i){g->triangles[0].v[i]=i;g->colors[i]={255,255,255,255};}auto a=rw::Atomic::create();a->setGeometry(g,0);
for(auto centre:{rw::V3d{0,0,60},rw::V3d{0,0,-5},rw::V3d{-100,0,5},rw::V3d{100,0,5},rw::V3d{0,-100,5},rw::V3d{0,100,5}}){g->morphTargets[0].vertices[0]=rw::add(centre,{-1,-1,0});g->morphTargets[0].vertices[1]=rw::add(centre,{1,-1,0});g->morphTargets[0].vertices[2]=rw::add(centre,{0,1,0});r.clear(0);a->render();for(auto pixel:r.pixels())assert(!pixel);}assert(device.culledTriangles()==6);
// A triangle crossing an edge must survive common-plane rejection and be clipped.
g->morphTargets[0].vertices[0]={-10,-1,5};g->morphTargets[0].vertices[1]={1,-1,5};g->morphTargets[0].vertices[2]={0,1,5};r.clear(0);a->render();assert(device.culledTriangles()==6);assert(r.pixels()[120*320+160]);assert(!device.unsupported());a->destroy();g->destroy();device.end();}
