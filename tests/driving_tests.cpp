#include "driving.hpp"
#include <cstdio>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <fstream>
#include <cstring>
using namespace soft;
static void check(bool b,const char* what){if(!b)throw std::runtime_error(what);}
static Triangle tri(Vec3 a,Vec3 b,Vec3 c){return {{a,b,c},0};}
static void u32(std::string& b,uint32_t x){for(int i=0;i<4;++i)b.push_back(char(x>>(8*i)));}
static void save(const char* path,const std::string& b){std::ofstream f(path,std::ios::binary);f.write(b.data(),b.size());check(bool(f),"fixture write");}
int main(int argc,char** argv){if(argc!=2)return 2;try{
    std::vector<Triangle> faces={tri({-20,0,-20},{20,0,-20},{20,0,20}),tri({-20,0,-20},{20,0,20},{-20,0,20}),
        tri({-20,0,8},{20,0,8},{20,4,8}),tri({-20,0,8},{20,4,8},{-20,4,8}),
        tri({-3,5,-3},{3,5,-3},{3,5,3}),tri({-3,5,-3},{3,5,3},{-3,5,3})};
    CollisionWorld world(faces);float ground;
    check(world.groundHeight(0,0,-1,1,ground)&&std::fabs(ground)<1e-5,"street height");
    check(world.groundHeight(0,0,4,6,ground)&&ground==5,"roof height");
    check(!world.groundHeight(100,0,-1,1,ground),"map bounds");
    check(!world.groundHeight(1e30f,0,-1,1,ground),"large query rejected");
    check(world.blocked(0,7.1f,.35f,1.7f,1.05f),"wall contact");
    check(!world.blocked(0,8,6,7,1.05f),"height-separated wall");
    Driving car(world,{0,2,0},0);check(std::fabs(car.position.y-.825f)<1e-5,"grounded spawn");
    for(int i=0;i<300;++i)car.update(world,.05f,1,0,false);
    check(car.position.z>4&&car.position.z<=5.31f&&car.speed==0,"thin wall / no tunneling");
    auto camera=car.chaseCamera(80,&world);check(camera.position.z<car.position.z,"chase behind car");
    car.reset(world);for(int i=0;i<25;++i)car.update(world,1.f/30,1,0,false);
    check(car.speed>4&&car.position.z>1,"acceleration");
    for(int i=0;i<30;++i)car.update(world,1.f/30,0,0,true);
    check(car.speed==0,"brake stops");
    car.reset(world);for(int i=0;i<60;++i)car.update(world,1.f/30,-1,0,false);
    check(car.speed<0&&car.position.z<0,"reverse");
    car.reset(world);for(int i=0;i<30;++i)car.update(world,1.f/30,1,.7f,false);
    check(car.position.x>0&&car.yaw>0,"forward steer");
    auto old=car.position;car.update(world,std::numeric_limits<float>::quiet_NaN(),1,1,false);
    check(car.position.x==old.x&&car.position.z==old.z,"invalid timestep");
    car.reset(world);car.update(world,0,1,0,false);check(car.position.z==0,"zero timestep");
    CollisionWorld slope({tri({-20,-3,-20},{20,-1,-20},{20,3,20}),tri({-20,-3,-20},{20,3,20},{-20,1,20})});
    Driving tilted(slope,{0,2,0},0);
    check(std::fabs(tilted.pitch-std::atan(.1f))<1e-4&&std::fabs(tilted.roll-std::atan(.05f))<1e-4,"four wheel slope pose");
    CollisionWorld edge({faces[0],faces[1]});Driving bounded(edge,{0,2,0},0);
    for(int i=0;i<300;++i)bounded.update(edge,.05f,1,0,false);
    check(bounded.hit&&bounded.speed==0&&bounded.position.z<=18.33f,"four wheel map edge support");
    std::string bytes="M3COL001";u32(bytes,uint32_t(faces.size()));
    for(auto t:faces)for(auto v:t.v)for(float n:{v.x,v.y,v.z}){uint32_t bits;std::memcpy(&bits,&n,4);u32(bytes,bits);}
    save(argv[1],bytes);check(CollisionWorld::load(argv[1]).faceCount()==faces.size(),"COL load");
    for(size_t n=0;n<bytes.size();++n){save(argv[1],bytes.substr(0,n));bool rejected=false;
        try{CollisionWorld::load(argv[1]);}catch(const std::exception&){rejected=true;}check(rejected,"truncation");}
    std::remove(argv[1]);std::puts("ground/wall/sweep/brake/reverse/steer/collision loader PASS");return 0;
}catch(const std::exception& e){std::remove(argv[1]);std::fprintf(stderr,"%s\n",e.what());return 1;}}
