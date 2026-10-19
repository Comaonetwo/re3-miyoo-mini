#include "software_device.hpp"
#include <cassert>
#include <cmath>
#include <vector>
int main(){using namespace soft;Renderer r;SoftwareDevice d(r);Camera c;c.position={1,2,3};c.yaw=.37f;d.begin(c);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
// Independent sequential model->world->view reference, nonidentity camera/model.
for(unsigned iteration=0;iteration<60;++iteration){rw::Matrix m;m.setIdentity();float angle=iteration*.013f;m.right={std::cos(angle),0,std::sin(angle)};m.at={-std::sin(angle),0,std::cos(angle)};m.pos={1,2,8+iteration*.2f};m.flags=iteration%2?0:rw::Matrix::IDENTITY;
rw::cpu::Im3DVertex input[9];input[0].position={-1,-1,0};input[1].position={1,-1,.8f};input[2].position={0,1,.2f};for(unsigned i=3;i<9;++i)input[i]=input[i%3];for(auto& v:input)v.setColor(150,190,80,255);
r.clear(0);rw::im3d::Transform(input,9,&m,rw::im3d::EVERYTHING);rw::im3d::RenderPrimitive(rw::PRIMTYPETRILIST);rw::im3d::End();auto actual=r.pixels();auto depths=r.depth();
rw::cpu::Im3DVertex world[9];for(unsigned i=0;i<9;++i){world[i]=input[i];rw::V3d::transformPoints(&world[i].position,&input[i].position,1,&m);}r.clear(0);rw::im3d::Transform(world,9,nullptr,rw::im3d::EVERYTHING);rw::im3d::RenderPrimitive(rw::PRIMTYPETRILIST);rw::im3d::End();const auto& expected=r.pixels();unsigned differences=0,covered=0;for(unsigned i=0;i<actual.size();++i){differences+=actual[i]!=expected[i];covered+=actual[i]!=0;if(actual[i]&&expected[i])assert(std::fabs(depths[i]-r.depth()[i])<1e-5f);}assert(covered>100);assert(differences<8);
}assert(!d.unsupported());d.end();}
