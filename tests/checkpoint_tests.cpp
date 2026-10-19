#include "checkpoint_run.hpp"
#include <stdexcept>
#include <limits>
#include <cstdio>
#include <cmath>
using namespace soft;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
    CheckpointRun run({{0,0,20},{0,0,50}},{0,.825f,0});
    run.update({0,.825f,10},0,1,true);check(!run.started()&&run.seconds()==0,"stationary ready");
    run.update({0,.825f,10},5,1,false);check(!run.started(),"free camera does not start run");
    run.update({0,.825f,10},5,1,true);check(run.started()&&run.seconds()==1,"start on motion");
    run.update({0,.825f,30},5,1,true);check(run.completed()==1,"swept gate crossing");
    run.update({0,6,50},5,1,true);check(run.completed()==1,"wrong-height gate ignored");
    double time=run.seconds();run.update({0,.825f,50},5,30,false);check(run.seconds()==time&&run.completed()==1,"pause freezes timer and gates");
    run.update({0,.825f,50},0,1,true);check(run.finished(),"finish last gate");
    time=run.seconds();run.update({0,.825f,50},5,20,true);check(run.seconds()==time,"finish freezes time");
    run.reset({0,.825f,0});check(!run.started()&&!run.finished()&&run.completed()==0&&run.seconds()==0,"restart");
    run.update({0,.825f,50},5,std::numeric_limits<double>::quiet_NaN(),true);check(!run.started(),"invalid timestep ignored");
    CheckpointRun ordered({{20,0,0},{0,0,20}},{0,.825f,0});ordered.update({0,.825f,20},5,1,true);check(ordered.completed()==0,"gate order");
    CollisionWorld world({{{{-100,0,-100},{100,0,-100},{100,0,220}},0},{{{-100,0,-100},{100,0,220},{-100,0,220}},0}});
    auto route=CheckpointRun::street(world,{0,.825f,0},0);check(route.route().size()==5&&route.route()[4].z==175,"grounded route");
    Driving driver(world,{0,.825f,0},0);float startAngle=driver.wheelAngle;
    for(int i=0;i<30;++i)driver.update(world,1.f/30,1,1,false);
    check(driver.wheelAngle!=startAngle&&driver.steeringAngle>.4f,"wheel travel and steering");
    Vec3 center{-.85833959f,-.47500002f,1.674f};auto pivot=driver.animateWheelVertex(center,0);
    check(std::fabs(pivot.x-center.x)<1e-6&&std::fabs(pivot.y-center.y)<1e-6&&std::fabs(pivot.z-center.z)<1e-6,"wheel pivot invariant");
    driver.wheelAngle=0;driver.steeringAngle=.42f;Vec3 front{center.x,center.y,center.z+.35f};auto moved=driver.animateWheelVertex(front,0);
    check(moved.x>front.x+.1f,"front wheel steer");
    Vec3 rear{center.x,center.y,-1.673f+.35f};auto fixed=driver.animateWheelVertex(rear,2);check(fixed.x==rear.x&&fixed.z==rear.z,"rear wheels do not steer");
    driver.steeringAngle=0;driver.wheelAngle=.1f;Vec3 bottom{center.x,center.y-.35f,center.z};
    check(driver.animateWheelVertex(bottom,0).z<bottom.z,"forward wheel bottom rolls backward relative to car");
    driver.wheelAngle=-.1f;check(driver.animateWheelVertex(bottom,0).z>bottom.z,"reverse rolling direction");
    driver.reset(world);check(driver.wheelAngle==0&&driver.steeringAngle==0,"wheel reset");
    MiniMap map(world,driver.position,0);std::vector<uint16_t> frame(320*240,rgb(0,0,0));
    drawDrivingHud(frame,map,route,driver,30,80,true,false,true);check(frame[140*320+244]!=rgb(0,0,0),"minimap draws");
    bool rejected=false;try{std::vector<uint16_t> small(1);map.draw(small,driver.position,0,route);}catch(const std::exception&){rejected=true;}check(rejected,"small buffer rejected");
    Renderer renderer;Camera camera;route.draw(renderer,camera);check(renderer.stats().submitted==50,"depth-tested checkpoint marker");
    std::puts("timer/order/pause/restart/swept gates/wheel pivots/steering/minimap PASS");return 0;
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
