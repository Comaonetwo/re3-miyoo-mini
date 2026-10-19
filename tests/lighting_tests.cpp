#include "rw_asset_scene.hpp"
#include <stdexcept>
#include <cstdio>
using namespace soft;
static void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
static size_t pixel=120*320+160;
int main(){try{Renderer r;{SoftwareDevice device(r);device.lighting({.1f,.2f,.3f},{.3f,.2f,.1f},{0,0,1});Camera camera;camera.position={0,0,0};device.begin(camera);
    auto g=rw::Geometry::create(3,1,rw::Geometry::POSITIONS|rw::Geometry::NORMALS|rw::Geometry::LIGHT|rw::Geometry::MODULATE);auto m=rw::Material::create();m->color={255,255,255,255};m->surfaceProps={1,0,1};g->triangles[0].matId=uint16_t(g->matList.appendMaterial(m));m->destroy();
    rw::V3d p[3]={{-1,-1,5},{1,-1,5},{0,1,5}};for(int i=0;i<3;++i){g->triangles[0].v[i]=uint16_t(i);g->morphTargets[0].vertices[i]=p[i];g->morphTargets[0].normals[i]={1,0,0};}
    g->calculateBoundingSphere();auto atomic=rw::Atomic::create();auto frame=rw::Frame::create();atomic->setGeometry(g,0);g->destroy();atomic->setFrame(frame);device.world()->addAtomic(atomic);r.clear(0);atomic->render();check(r.pixels()[pixel]==rgb(26,51,77),"ambient only for perpendicular normal");auto baseline=r.pixels();device.farNormalLighting(true);r.clear(0);atomic->render();check(r.pixels()==baseline&&device.farNormalVertices()>=3,"far axis lighting preserves axis-normal image");device.farNormalLighting(false);device.end();
    rw::Matrix matrix;matrix.setIdentity();matrix.right={0,0,-1};matrix.at={1,0,0};matrix.flags=0;frame->transform(&matrix,rw::COMBINEREPLACE);camera.yaw=1.57079632679f;r.clear(0);device.begin(camera);atomic->render();check(r.pixels()[pixel]==rgb(102,102,102),"directional light uses transformed normal");device.end();device.world()->removeAtomic(atomic);atomic->destroy();frame->destroy();
    AssetScene scene;scene.groups.push_back({0,3,{0,0,6},4});
    scene.triangles={{{{-1,-1,8},{1,-1,8},{0,1,8}},rgb(0,0,255)},{{{-1,-1,5},{1,-1,5},{1,1,5}},rgb(255,0,0)},{{{-1,-1,5},{1,1,5},{-1,1,5}},rgb(255,0,0)}};
    scene.materials.resize(3);for(auto& t:scene.materials){t.texture=65535;t.addressU=t.addressV=3;}
    scene.surfaces.resize(3);for(int i=0;i<3;++i){auto& s=scene.surfaces[i];s.flags=2;s.material[0]=i?255:0;s.material[1]=0;s.material[2]=i?0:255;s.material[3]=i?128:255;s.ambient=s.diffuse=1;}
    scene.surfaces[0].flags|=16;camera.yaw=0;device.begin(camera);r.clear(0);{RwAssetScene rw(scene,device.world());std::vector<TransparentAtomic> objects;rw.gather(camera,objects);check(objects.size()==1&&r.pixels()[pixel]==rgb(0,0,255),"glass separated from opaque batch");auto depth=r.depth();renderTransparent(objects);check(r.pixels()[pixel]==rgb(128,0,127),"sorted glass blend, shared edge once");check(r.depth()==depth&&rw::GetRenderState(rw::ZWRITEENABLE)==1,"transparent pass preserves depth and restores state");
        scene.discardConvertedData();rw.paint(0x00ff00,0xff00ff);r.clear(0);objects.clear();rw.gather(camera,objects);check(r.pixels()[pixel]==rgb(0,255,0),"paint affects only flagged material");}
    check(device.unsupported()==0,"real world/light/rich surfaces have no unsupported calls");device.end();}
    check(!rw::Light::numAllocated&&!rw::World::numAllocated&&!rw::Frame::numAllocated&&!rw::Geometry::numAllocated&&!rw::Material::numAllocated,"world and asset objects released");puts("ambient/directional/normals/materials/glass/sort/depth/paint/lifecycle PASS");return 0;
}catch(const std::exception& e){puts(e.what());return 1;}}
