#include "software_device.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <cstdio>
using namespace soft;
static void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
static bool plugins(){rw::registerHAnimPlugin();rw::registerSkinPlugin();return true;}
int main(){try{Renderer r;SoftwareDevice device(r,plugins);Camera camera;camera.position={0,0,0};device.begin(camera);
 rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);
 auto g=rw::Geometry::create(9,3,rw::Geometry::POSITIONS|rw::Geometry::MODULATE|rw::Geometry::NORMALS|rw::Geometry::LIGHT|rw::Geometry::PRELIT|rw::Geometry::TEXTURED);
 auto material=rw::Material::create();material->color={255,0,0,255};g->triangles[0].matId=g->matList.appendMaterial(material);material->destroy();
 rw::V3d p[3]={{-1,-1,5},{1,-1,5},{0,1,5}};for(int i=0;i<9;++i){g->morphTargets[0].vertices[i]=p[i%3];g->texCoords[0][i]={float(i%3)*.5f,.5f};g->morphTargets[0].normals[i]={0,0,-1};g->colors[i]={0,0,0,255};g->triangles[i/3].v[i%3]=i;g->triangles[i/3].matId=0;}
 auto skin=rwNewT(rw::Skin,1,rw::MEMDUR_EVENT|rw::ID_SKIN);std::memset(skin,0,sizeof(*skin));skin->init(2,2,9);skin->numWeights=2;rw::Skin::set(g,skin);
 for(int i=0;i<2;++i){rw::Matrix m;m.setIdentity();std::memcpy(skin->inverseMatrices+i*16,&m,64);}
 for(int i=0;i<9;++i){skin->indices[i*4]=0;skin->indices[i*4+1]=1;skin->weights[i*4]=.25f;skin->weights[i*4+1]=.75f;skin->weights[i*4+2]=skin->weights[i*4+3]=0;}
 auto a=rw::Atomic::create();auto frame=rw::Frame::create();a->setFrame(frame);a->setGeometry(g,0);g->destroy();a->pipeline=rw::skinGlobals.dummypipe;
 rw::Matrix matrices[2];for(auto& m:matrices)m.setIdentity();rw::HAnimHierarchy h{};h.numNodes=2;h.matrices=matrices;h.flags=rw::HAnimHierarchy::LOCALSPACEMATRICES;rw::Skin::setHierarchy(a,&h);
 auto render=[&](){device.directAtomic(false);r.clear(0);a->render();auto rgb=r.pixels();auto z=r.depth();device.directAtomic(true);r.clear(0);a->render();check(rgb==r.pixels()&&z==r.depth(),"direct deformed vertices preserve exact RGB/depth");double x=0,count=0;for(int y=0;y<240;++y)for(int i=0;i<320;++i)if(r.pixels()[y*320+i]){x+=i;++count;}check(count>100,"skin triangle visible");return x/count;};
 device.highQuality(false);double bind=render();matrices[1].pos.x=1;matrices[1].flags=0;double moved=render();check(moved>bind+20&&moved<bind+35,"two weighted bones deform positions");
 rw::Matrix object;object.setIdentity();object.pos.x=1;object.flags=0;frame->transform(&object,rw::COMBINEREPLACE);
 double local=render();check(local>moved+30&&local<moved+40,"local hierarchy applies object transform once");
 h.flags=0;for(auto& m:matrices){m.pos.x+=1;m.flags=0;}double world=render();check(std::fabs(world-local)<.1,"world hierarchy removes atomic transform before skinning");
 check(device.skinCalls()==8&&device.skinnedVertices()==72&&device.directAtomicVertices()==36,"real skin pipeline counters");check(device.unsupported()==0,"supported skin paths");
 skin->indices[0]=2;r.clear(0);a->render();check(device.unsupported()==1,"invalid weighted bone rejected");skin->indices[0]=0;
 skin->weights[0]=NAN;a->render();check(device.unsupported()==2,"invalid weight rejected");
 rw::Skin::setHierarchy(a,nullptr);a->setFrame(nullptr);a->destroy();frame->destroy();device.end();puts("PASS real Skin pipeline: weighted bones, local/world hierarchies, normals, invalid bone/weight bounds");return 0;
}catch(const std::exception& e){fprintf(stderr,"%s\n",e.what());return 1;}}
