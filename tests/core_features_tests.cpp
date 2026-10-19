#include "software_device.hpp"
#include "core_menu.hpp"
#include <stdexcept>
#include <cstdio>
using namespace soft;
static void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
static bool plugins(){rw::registerMatFXPlugin();return true;}
int main(int argc,char**argv){try{
 check(argc==2,"settings path");CoreMenu menu;menu.toggle();menu.row=1;menu.key(CoreKey::Right);check(menu.settings.scale()==3,"resolution advances from 160x120 to 240x180");
 menu.key(CoreKey::Down);menu.key(CoreKey::Right);check(menu.settings.distance()==80,"distance changes");
 for(int i=0;i<9;++i)menu.key(CoreKey::Down);menu.key(CoreKey::Accept);check(!menu.settings.fastEffects(),"effects standard selected");for(int i=0;i<5;++i)menu.key(CoreKey::Down);menu.key(CoreKey::Accept);check(menu.help&&menu.open,"help keeps game paused");
 menu.key(CoreKey::Back);check(!menu.help&&menu.open,"back from help keeps menu");menu.key(CoreKey::Back);check(!menu.open,"back resumes game");
 check(menu.settings.save(argv[1]),"save settings");check(CoreSettings::load(argv[1]).bits==menu.settings.bits,"persistent settings round trip");
 FILE* f=std::fopen(argv[1],"wb");check(f,"settings file");std::fprintf(f,"GTA3_CPU_SETTINGS 1\n1536\n");std::fclose(f);check(CoreSettings::load(argv[1]).bits==CoreSettings::defaultBits,"invalid resolution/distance fallback");std::remove(argv[1]);
 check(!CoreSettings::valid(1u<<20),"reserved flags rejected");menu.toggle();for(int i=0;i<2;++i)menu.key(CoreKey::Down);menu.key(CoreKey::Accept);check(menu.exitRequested,"menu exit action");
 Renderer renderer;SoftwareDevice device(renderer,plugins);Camera camera;camera.position={0,0,0};device.begin(camera);
 rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);
 auto g=rw::Geometry::create(3,1,rw::Geometry::POSITIONS|rw::Geometry::MODULATE|rw::Geometry::LIGHT);
 auto m=rw::Material::create();m->color={255,0,0,255};g->triangles[0].matId=uint16_t(g->matList.appendMaterial(m));m->destroy();
 rw::V3d positions[3]={{-2,-2,5},{2,-2,5},{0,2,5}};for(int i=0;i<3;++i){g->morphTargets[0].vertices[i]=positions[i];g->triangles[0].v[i]=uint16_t(i);}
 auto a=rw::Atomic::create();auto frame=rw::Frame::create();a->setFrame(frame);a->setGeometry(g,0);g->destroy();
 rw::MatFX::enableEffects(a);check(a->pipeline==rw::matFXGlobals.dummypipe,"real MatFX pipeline selected");
 auto point=rw::Light::create(rw::Light::POINT);auto pf=rw::Frame::create();point->setFrame(pf);point->setColor(1,1,1);point->radius=100;device.world()->addLight(point);
 renderer.clear(0);a->render();check(device.atomicCalls()==1&&renderer.pixels()[120*320+160]!=0,"MatFX base model drawn despite local lights");
 device.highQuality(false);renderer.renderScale(2);renderer.clear(0);a->render();check(renderer.pixels()[120*320+160]==rgb(255,0,0),"flat model rendering at 160x120");
 rw::SetRenderState(rw::CULLMODE,rw::CULLBACK);renderer.clear(0);a->render();bool back=renderer.pixels()[120*320+160]!=0;
 rw::SetRenderState(rw::CULLMODE,rw::CULLFRONT);renderer.clear(0);a->render();bool front=renderer.pixels()[120*320+160]!=0;check(back!=front,"fast opaque model preserves winding culling");
 device.world()->removeLight(point);point->setFrame(nullptr);point->destroy();pf->destroy();a->setFrame(nullptr);a->destroy();frame->destroy();
 device.end();check(device.unsupported()==0,"supported CPU compatibility paths");puts("PASS persistent settings, pause/help/exit, actual MatFX base model and local-light fallback");return 0;
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
