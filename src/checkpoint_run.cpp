#include "checkpoint_run.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <cstdio>
namespace soft {
namespace {
bool valid(Vec3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&std::fabs(p.x)<=10000&&std::fabs(p.z)<=10000&&std::fabs(p.y)<=5000;}
float distance2(Vec3 p,Vec3 a,Vec3 b){float dx=b.x-a.x,dz=b.z-a.z,n=dx*dx+dz*dz;
    float t=n>1e-8f?std::max(0.f,std::min(1.f,((p.x-a.x)*dx+(p.z-a.z)*dz)/n)):0;
    float x=a.x+t*dx-p.x,z=a.z+t*dz-p.z;return x*x+z*z;}
void pixel(std::vector<uint16_t>& frame,int x,int y,uint16_t color){if(x>=0&&x<320&&y>=0&&y<240)frame[size_t(y)*320+x]=color;}
}
CheckpointRun::CheckpointRun(std::vector<Vec3> route,Vec3 start):route_(std::move(route)),previous_(start){
    if(route_.empty()||route_.size()>32||!valid(start))throw std::runtime_error("Invalid checkpoint route");
    for(auto p:route_)if(!valid(p))throw std::runtime_error("Invalid checkpoint position");
}
CheckpointRun CheckpointRun::street(const CollisionWorld& world,Vec3 start,float yaw){
    if(!valid(start)||!std::isfinite(yaw))throw std::runtime_error("Invalid route start");
    std::vector<Vec3> route;
    float ground=start.y-.825f;
    for(int i=1;i<=5;++i){Vec3 p{start.x+std::sin(yaw)*35*i,start.y,start.z+std::cos(yaw)*35*i};
        if(!world.groundHeight(p.x,p.z,ground-3,ground+3,ground))throw std::runtime_error("Checkpoint has no road");
        p.y=ground;
        // Validate the same vehicle footprint/support used by real driving.
        Driving test(world,{p.x,ground+.825f,p.z},yaw);
        route.push_back(p);
    }
    return CheckpointRun(route,start);
}
void CheckpointRun::reset(Vec3 start){if(!valid(start))throw std::runtime_error("Invalid run reset");previous_=start;next_=0;seconds_=0;started_=false;}
void CheckpointRun::update(Vec3 position,float speed,double dt,bool active){
    if(!valid(position)||!std::isfinite(speed)||!std::isfinite(dt)||dt<0)return;
    if(!active||finished()){previous_=position;return;}
    if(!started_&&std::fabs(speed)>.25f)started_=true;
    if(started_){seconds_+=dt;auto target=route_[next_];
        // Swept XZ segment also catches a gate crossed between frames.
        if(std::fabs(position.y-.825f-target.y)<2&&distance2(target,previous_,position)<=36)++next_;
    }
    previous_=position;
}
void CheckpointRun::draw(Renderer& r,const Camera& camera) const{
    for(auto t:marker())r.worldTriangle(t,camera);
}
std::vector<Triangle> CheckpointRun::marker() const{
    std::vector<Triangle> triangles;if(finished())return triangles;
    auto p=route_[next_];uint16_t color=rgb(255,205,35);
    triangles.reserve(50);
    // Original geometry: low ring and two chevrons, drawn with the normal depth buffer.
    constexpr float tau=6.283185307f;
    for(int i=0;i<24;++i){float a=i*tau/24,b=(i+1)*tau/24;
        Vec3 v0{p.x+6*std::cos(a),p.y+.12f,p.z+6*std::sin(a)},v1{p.x+6*std::cos(b),p.y+.12f,p.z+6*std::sin(b)};
        Vec3 v2{p.x+5.6f*std::cos(a),p.y+.12f,p.z+5.6f*std::sin(a)},v3{p.x+5.6f*std::cos(b),p.y+.12f,p.z+5.6f*std::sin(b)};
        triangles.push_back({{v0,v1,v2},color});triangles.push_back({{v2,v1,v3},color});
    }
    for(float side:{-1.f,1.f}){Vec3 a{p.x+side*6,p.y,p.z},b{p.x+side*6,p.y+3,p.z},tip{p.x+side*4.5f,p.y+1.5f,p.z};
        triangles.push_back({{a,b,tip},color});}
    return triangles;
}
MiniMap::MiniMap(const CollisionWorld& world,Vec3 start,float yaw):center_(start),pixels_(side*side){
    if(!valid(start)||!std::isfinite(yaw))throw std::runtime_error("Invalid minimap origin");
    center_.x+=std::sin(yaw)*90;center_.z+=std::cos(yaw)*90;
    for(int y=0;y<side;++y)for(int x=0;x<side;++x){
        float wx=center_.x+(x+.5f-side*.5f)*span_/side,wz=center_.z+(side*.5f-y-.5f)*span_/side,height;
        uint16_t color=rgb(15,24,31);
        if(world.groundHeight(wx,wz,start.y-8,start.y+8,height))color=world.blocked(wx,wz,height+.35f,height+1.7f,.8f)?rgb(91,96,104):rgb(47,67,72);
        pixels_[size_t(y)*side+x]=color;
    }
}
Vec2 MiniMap::map(Vec3 p) const{return {(p.x-center_.x)*side/span_+side*.5f,(center_.z-p.z)*side/span_+side*.5f};}
void MiniMap::draw(std::vector<uint16_t>& frame,Vec3 position,float yaw,const CheckpointRun& run) const{
    if(frame.size()!=320*240||!valid(position)||!std::isfinite(yaw))throw std::runtime_error("Invalid minimap frame/pose");
    constexpr int left=244,top=140;
    for(int y=-1;y<=side;++y)for(int x=-1;x<=side;++x)pixel(frame,left+x,top+y,x<0||y<0||x==side||y==side?rgb(145,156,167):pixels_[size_t(y)*side+x]);
    auto dot=[&](Vec3 p,uint16_t color,int radius){auto uv=map(p);int x=int(std::lround(uv.u)),y=int(std::lround(uv.v));
        for(int j=-radius;j<=radius;++j)for(int i=-radius;i<=radius;++i)if(x+i>=0&&x+i<side&&y+j>=0&&y+j<side)pixel(frame,left+x+i,top+y+j,color);};
    for(size_t i=0;i<run.route().size();++i)dot(run.route()[i],i<run.completed()?rgb(70,180,110):i==run.completed()?rgb(255,205,35):rgb(135,137,140),i==run.completed()?2:1);
    auto uv=map(position);int x=int(std::lround(uv.u)),y=int(std::lround(uv.v));
    dot(position,rgb(220,235,255),1);
    for(int d=0;d<=5;++d){int px=x+int(std::lround(std::sin(yaw)*d)),py=y-int(std::lround(std::cos(yaw)*d));
        if(px>=0&&px<side&&py>=0&&py<side)pixel(frame,left+px,top+py,rgb(220,235,255));}
}
void drawDrivingHud(std::vector<uint16_t>& frame,const MiniMap& map,const CheckpointRun& run,const Driving& driver,double fps,float farZ,bool textured,bool paused,bool driveMode){
    if(frame.size()!=320*240)throw std::runtime_error("Invalid HUD frame");
    std::fill(frame.begin(),frame.begin()+320*23,rgb(15,20,28));
    std::fill(frame.end()-320*23,frame.end(),rgb(15,20,28));
    char status[80];std::snprintf(status,sizeof(status),"FPS %.1f D%.0f %s %.0f KMH%s",fps,farZ,textured?"T":"F",driver.speed*3.6,paused?" P":"");drawText(frame,4,3,status);
    std::snprintf(status,sizeof(status),"%s %zu-%zu TIME %.1f%s",run.finished()?"FINISH":"GATES",run.completed(),run.route().size(),run.seconds(),!driveMode?" FREE":!run.started()?" READY":"");drawText(frame,4,14,status);
    map.draw(frame,driver.position,driver.yaw,run);
    drawText(frame,4,220,driveMode?"UP GAS DOWN REVERSE L BRAKE R FREE":"FREE CAMERA D-PAD MOVE L UP R DRIVE");
    drawText(frame,4,230,"A RESTART Y TEX B DIST MENU-SELECT EXIT");
}
}
