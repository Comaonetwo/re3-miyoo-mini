#include "software_device.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <stdexcept>
using namespace soft;
static void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
static size_t at(int x,int y){return size_t(y)*320+x;}
static unsigned red(uint16_t c){return ((c>>11)&31)*255/31;}
static unsigned green(uint16_t c){return ((c>>5)&63)*255/63;}
static unsigned blue(uint16_t c){return (c&31)*255/31;}
static void draw(rw::cpu::Im3DVertex* v){rw::im3d::Transform(v,3,nullptr,rw::im3d::EVERYTHING);rw::im3d::RenderPrimitive(rw::PRIMTYPETRILIST);rw::im3d::End();}
static void vertices(rw::cpu::Im3DVertex* v,float z0=5,float z1=5,float z2=5){
    v[0].position={-60*z0/180,-50*z0/180,z0};v[1].position={60*z1/180,-50*z1/180,z1};v[2].position={0,70*z2/180,z2};
    v[0].setColor(255,0,0,255);v[1].setColor(0,255,0,255);v[2].setColor(0,0,255,255);
}
int main(){try{
    Renderer renderer;SoftwareDevice device(renderer);Camera camera;camera.position={0,0,0};device.begin(camera);
    rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);rw::cpu::Im3DVertex v[3];
    vertices(v,2,8,4);renderer.clear(0);draw(v);
    // At pixel centre (160.5,120.5), screen barycentrics are known analytically.
    float w2=(170-120.5f)/120,w1=((160.5f-100)/120)-w2*.5f,w0=1-w1-w2;
    float z=w0/2+w1/8+w2/4;auto color=renderer.pixels()[at(160,120)];
    check(std::fabs(red(color)-255*w0/2/z)<9&&std::fabs(green(color)-255*w1/8/z)<9&&std::fabs(blue(color)-255*w2/4/z)<9,"perspective-correct Gouraud colors");
    check(std::fabs(renderer.depth()[at(160,120)]-z)<1e-5,"interpolated reciprocal depth");
    vertices(v);for(auto& p:v)p.setColor(255,0,0,128);
    rw::SetRenderState(rw::VERTEXALPHA,1);rw::SetRenderState(rw::ZWRITEENABLE,0);
    renderer.clear(rgb(0,0,255));draw(v);check(renderer.pixels()[at(160,120)]==rgb(128,0,127),"3D source alpha blend");check(renderer.depth()[at(160,120)]==0,"transparent pass leaves depth untouched");
    rw::SetRenderState(rw::DESTBLEND,rw::BLENDONE);renderer.clear(rgb(0,0,255));draw(v);check(renderer.pixels()[at(160,120)]==rgb(128,0,255),"3D additive alpha blend");
    rw::SetRenderState(rw::SRCBLEND,rw::BLENDZERO);rw::SetRenderState(rw::DESTBLEND,rw::BLENDINVSRCCOLOR);
    for(auto& p:v)p.setColor(128,0,0,255);renderer.clear(rgb(255,255,255));draw(v);
    check(renderer.pixels()[at(160,120)]==rgb(127,255,255)&&renderer.depth()[at(160,120)]==0,"inverse source color shadow preserves depth");
    rw::SetRenderState(rw::SRCBLEND,rw::BLENDONE);rw::SetRenderState(rw::DESTBLEND,rw::BLENDZERO);
    for(auto& p:v)p.setColor(255,0,0,0);renderer.clear(rgb(0,0,255));draw(v);
    check(renderer.pixels()[at(160,120)]==rgb(255,0,0),"ONE/ZERO water pass replaces destination regardless of alpha");
    rw::SetRenderState(rw::SRCBLEND,rw::BLENDZERO);rw::SetRenderState(rw::DESTBLEND,rw::BLENDONE);
    renderer.clear(rgb(0,0,255));draw(v);check(renderer.pixels()[at(160,120)]==rgb(0,0,255),"ZERO/ONE pass preserves destination");
    rw::SetRenderState(rw::SRCBLEND,rw::BLENDSRCALPHA);
    rw::SetRenderState(rw::DESTBLEND,rw::BLENDINVSRCALPHA);rw::SetRenderState(rw::VERTEXALPHA,0);rw::SetRenderState(rw::ZWRITEENABLE,1);
    vertices(v,2,2,2);for(auto& p:v)p.setColor(0,255,0,255);renderer.clear(0);draw(v);
    auto depth=renderer.depth();vertices(v);for(auto& p:v)p.setColor(255,0,0,128);rw::SetRenderState(rw::VERTEXALPHA,1);rw::SetRenderState(rw::ZWRITEENABLE,0);draw(v);
    check(renderer.pixels()[at(160,120)]==rgb(0,255,0)&&renderer.depth()==depth,"opaque foreground occludes transparent surface");
    // Texture alpha is continuous in a blending pass, not a binary cutout.
    auto raster=rw::Raster::create(1,1,32,rw::Raster::TEXTURE|rw::Raster::C8888);auto bytes=raster->lock(0,rw::Raster::LOCKWRITE);uint8_t rgba[4]={255,255,255,64};std::memcpy(bytes,rgba,4);raster->unlock(0);
    for(auto& p:v)p.setColor(255,0,0,255);rw::SetRenderStatePtr(rw::TEXTURERASTER,raster);renderer.clear(rgb(0,0,255));draw(v);check(renderer.pixels()[at(160,120)]==rgb(64,0,191),"3D texture alpha blend");
    rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAGREATEREQUAL);rw::SetRenderState(rw::ALPHATESTREF,128);renderer.clear(rgb(0,0,255));draw(v);check(renderer.pixels()[at(160,120)]==rgb(0,0,255),"3D alpha cutoff");
    rw::SetRenderStatePtr(rw::TEXTURERASTER,nullptr);raster->destroy();rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);rw::SetRenderState(rw::VERTEXALPHA,0);rw::SetRenderState(rw::ZWRITEENABLE,1);
    // Opposite winding must alternate which culling mode keeps the triangle.
    vertices(v);for(auto& p:v)p.setColor(255,255,255,255);
    rw::SetRenderState(rw::CULLMODE,rw::CULLBACK);renderer.clear(0);draw(v);bool back=renderer.pixels()[at(160,120)]!=0;
    rw::SetRenderState(rw::CULLMODE,rw::CULLFRONT);renderer.clear(0);draw(v);bool front=renderer.pixels()[at(160,120)]!=0;check(back!=front,"3D winding culling");rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);
    // Actual Geometry PRELIT/MODULATE path must preserve per-vertex colors.
    auto g=rw::Geometry::create(3,1,rw::Geometry::POSITIONS|rw::Geometry::PRELIT|rw::Geometry::MODULATE);auto m=rw::Material::create();m->color={128,128,128,255};g->triangles[0].matId=uint16_t(g->matList.appendMaterial(m));m->destroy();
    vertices(v);for(int i=0;i<3;++i){g->triangles[0].v[i]=uint16_t(i);g->morphTargets[0].vertices[i]=v[i].position;g->colors[i]={v[i].r,v[i].g,v[i].b,255};}
    g->calculateBoundingSphere();auto atomic=rw::Atomic::create();atomic->setGeometry(g,0);g->destroy();renderer.clear(0);atomic->render();color=renderer.pixels()[at(160,120)];
    check(red(color)>20&&green(color)>20&&blue(color)>20&&red(color)<70&&green(color)<70&&blue(color)<70,"Atomic prelit colors modulated by material");
    m=g->matList.materials[0];m->color={255,0,0,128};for(int i=0;i<3;++i)g->colors[i]={255,255,255,255};rw::SetRenderState(rw::ZWRITEENABLE,0);renderer.clear(rgb(0,0,255));atomic->render();
    check(renderer.pixels()[at(160,120)]==rgb(128,0,127),"Atomic material alpha enables blending");check(rw::GetRenderState(rw::VERTEXALPHA)==0,"Atomic restores alpha state");atomic->destroy();
    // Independently split a colored near-plane crossing before rasterization.
    Renderer manual;renderer.clear(0);manual.clear(0);rw::SetRenderState(rw::VERTEXALPHA,0);rw::SetRenderState(rw::ZWRITEENABLE,1);
    ShadedVertex clip[3]={{{-.2f,.3f,.05f},{0,0},255,0,0,255},{{-1,-1,3},{0,0},0,255,0,255},{{1,-1,3},{0,0},0,0,255,255}};
    for(int i=0;i<3;++i){v[i].position={clip[i].position.x,clip[i].position.y,clip[i].position.z};v[i].setColor(uint8_t(clip[i].r),uint8_t(clip[i].g),uint8_t(clip[i].b),255);}draw(v);
    auto intersect=[&](ShadedVertex q){float t=(camera.nearZ-clip[0].position.z)/(q.position.z-clip[0].position.z);auto mix=[&](float a,float b){return a+t*(b-a);};return ShadedVertex{{mix(clip[0].position.x,q.position.x),mix(clip[0].position.y,q.position.y),camera.nearZ},{0,0},mix(255,q.r),mix(0,q.g),mix(0,q.b),255};};
    auto a=intersect(clip[1]),b=intersect(clip[2]);ScreenState state;state.zTest=state.zWrite=true;state.cull=1;
    ShadedVertex piece1[3]={b,a,clip[1]},piece2[3]={b,clip[1],clip[2]};manual.shadedTriangle(piece1,nullptr,state,camera);manual.shadedTriangle(piece2,nullptr,state,camera);
    size_t differences=0;for(size_t i=0;i<renderer.pixels().size();++i)differences+=renderer.pixels()[i]!=manual.pixels()[i];check(differences<10,"clipped Gouraud attributes match independent near-plane split");
    CheckpointRun run({{0,0,8}},{0,0,0});renderer.clear(0);device.checkpoint(run,camera,true);
    check(rw::GetRenderState(rw::ZWRITEENABLE)==1&&rw::GetRenderState(rw::VERTEXALPHA)==0&&rw::GetRenderState(rw::DESTBLEND)==rw::BLENDINVSRCALPHA,"checkpoint gradient restores render states");
    check(device.shadedTriangles()>5&&device.unsupported()==0,"general 3D path used without unsupported callbacks");device.end();
    puts("3D Gouraud/perspective/depth/alpha/additive/texture/culling/prelit/material alpha PASS");return 0;
}catch(const std::exception& e){puts(e.what());return 1;}}
