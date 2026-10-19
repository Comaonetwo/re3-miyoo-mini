#include "driving.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace soft {
namespace {
constexpr float rideHeight=.825f;
bool clear(const CollisionWorld& world,Vec3 p,float yaw){
    float ground=p.y-rideHeight,si=std::sin(yaw),co=std::cos(yaw);
    for(float d:{-1.65f,0.f,1.65f})if(world.blocked(p.x+si*d,p.z+co*d,ground+.35f,ground+1.7f,1.05f))return false;
    return true;
}
}
Driving::Driving(const CollisionWorld& world,Vec3 spawn,float yaw):spawn_(spawn),spawnYaw_(yaw){reset(world);}
void Driving::reset(const CollisionWorld& world){float ground;
    if(!world.groundHeight(spawn_.x,spawn_.z,spawn_.y-6,spawn_.y+1,ground))throw std::runtime_error("No drivable ground at spawn");
    position={spawn_.x,ground+rideHeight,spawn_.z};yaw=spawnYaw_;speed=0;pitch=roll=wheelAngle=steeringAngle=0;hit=false;
    if(!clear(world,position,yaw))throw std::runtime_error("Vehicle spawn intersects collision");
    update(world,0,0,0,false);if(hit)throw std::runtime_error("No ground under spawn wheels");
}
void Driving::update(const CollisionWorld& world,float dt,float throttle,float steer,bool brake){
    if(!std::isfinite(dt)||dt<0||!std::isfinite(throttle)||!std::isfinite(steer))return;
    dt=std::min(dt,.05f);throttle=std::max(-1.f,std::min(1.f,throttle));steer=std::max(-1.f,std::min(1.f,steer));hit=false;
    float desiredSteer=steer*.42f,maxChange=2.f*dt;
    steeringAngle+=std::max(-maxChange,std::min(maxChange,desiredSteer-steeringAngle));
    if(brake||throttle==0){float drag=(brake?15.f:1.8f)*dt;speed=std::copysign(std::max(0.f,std::fabs(speed)-drag),speed);}
    else if(throttle<0&&speed>0)speed=std::max(0.f,speed-10*dt);
    else if(throttle>0&&speed<0)speed=std::min(0.f,speed+10*dt);
    else speed+=throttle*(throttle>0?6.f:3.f)*dt;
    speed=std::max(-4.f,std::min(14.f,speed));
    float distance=speed*dt,turn=speed/3.35f*std::tan(steeringAngle)*dt;
    int steps=std::max(1,int(std::ceil((std::fabs(distance)+std::fabs(turn)*2.7f)/.18f)));
    for(int i=0;i<steps;++i){float nextYaw=yaw+turn/steps;
        Vec3 next{position.x+std::sin(nextYaw)*distance/steps,position.y,position.z+std::cos(nextYaw)*distance/steps};
        float ground=position.y-rideHeight,nextGround;
        if(!world.groundHeight(next.x,next.z,ground-1.2f,ground+.45f,nextGround)){speed=0;hit=true;break;}
        float wheelGround[4];bool supported=true;float co=std::cos(nextYaw),si=std::sin(nextYaw);
        int index=0;for(float z:{1.674f,-1.673f})for(float x:{-.835f,.835f}){
            supported&=world.groundHeight(next.x+co*x+si*z,next.z-si*x+co*z,ground-1.2f,ground+.45f,wheelGround[index++]);
        }
        if(!supported){speed=0;hit=true;break;}
        nextGround=(wheelGround[0]+wheelGround[1]+wheelGround[2]+wheelGround[3])*.25f;
        next.y=nextGround+rideHeight;
        if(!clear(world,next,nextYaw)){speed=0;hit=true;break;}
        pitch=std::max(-.4f,std::min(.4f,std::atan2((wheelGround[0]+wheelGround[1]-wheelGround[2]-wheelGround[3])*.5f,3.347f)));
        roll=std::max(-.4f,std::min(.4f,std::atan2((wheelGround[1]+wheelGround[3]-wheelGround[0]-wheelGround[2])*.5f,1.67f)));
        wheelAngle=std::remainder(wheelAngle+distance/steps/.35f,6.283185307f);
        position=next;yaw=std::remainder(nextYaw,6.283185307f);
    }
}
Camera Driving::chaseCamera(float farZ,const CollisionWorld* world) const{
    float distance=8.5f,si=std::sin(yaw),co=std::cos(yaw),height=position.y+1.975f;
    if(world)for(float d=.5f;d<=distance;d+=.5f){
        if(world->blocked(position.x-si*d,position.z-co*d,height-.15f,height+.15f,.15f)){distance=std::max(.5f,d-.5f);break;}
    }
    Camera c;c.position={position.x-si*distance,height,position.z-co*distance};c.yaw=yaw;c.farZ=farZ;return c;
}
namespace {
Vec3 wheelVertex(Vec3 v,unsigned wheel,float co,float si,float cs,float ss){
    if(wheel>=4)return v;
    // Kuruma wheel dummy positions in exported Y-up coordinates.
    float x=(wheel%2==0?-.85833959f:.85833959f),y=-.47500002f,z=wheel<2?1.674f:-1.673f;
    v.x-=x;v.y-=y;v.z-=z;
    float rotatedY=co*v.y-si*v.z,rotatedZ=si*v.y+co*v.z;
    if(wheel<2){float rotatedX=cs*v.x+ss*rotatedZ;rotatedZ=-ss*v.x+cs*rotatedZ;v.x=rotatedX;}
    return {v.x+x,rotatedY+y,rotatedZ+z};
}
}
Vec3 Driving::animateWheelVertex(Vec3 v,unsigned wheel) const{
    return wheelVertex(v,wheel,std::cos(wheelAngle),std::sin(wheelAngle),std::cos(steeringAngle),std::sin(steeringAngle));
}
DrivingPose::DrivingPose(const Driving& d):position(d.position),co(std::cos(d.yaw)),si(std::sin(d.yaw)),cp(std::cos(d.pitch)),sp(std::sin(d.pitch)),cr(std::cos(d.roll)),sr(std::sin(d.roll)),cw(std::cos(d.wheelAngle)),sw(std::sin(d.wheelAngle)),cs(std::cos(d.steeringAngle)),ss(std::sin(d.steeringAngle)){}
Vec3 DrivingPose::transform(Vec3 v,unsigned wheel) const{
    v=wheelVertex(v,wheel,cw,sw,cs,ss);float x=cr*v.x-sr*v.y,y=sr*v.x+cr*v.y,z=-sp*y+cp*v.z;y=cp*y+sp*v.z;
    return {position.x+co*x+si*z,position.y+y,position.z-si*x+co*z};
}
void Driving::draw(Renderer& r,const AssetScene& car,const Camera& c,bool textured) const{
    DrivingPose pose(*this);
    for(size_t i=0;i<car.triangles.size();++i){auto t=car.triangles[i];Vec3 points[3];
        unsigned wheel=4;
        if(car.groups.size()==5)for(unsigned g=1;g<5;++g)if(i>=car.groups[g].begin&&i-car.groups[g].begin<car.groups[g].count){wheel=g-1;break;}
        for(int j=0;j<3;++j)points[j]=view(pose.transform(t.v[j],wheel),c);
        if(textured&&!car.materials.empty()&&car.materials[i].texture!=65535){auto& m=car.materials[i];
            r.texturedTriangle(points,m.uv,t.color,car.textures[m.texture],m.addressU,m.addressV,c);
        }else r.triangle(points,t.color,c);
    }
}
}
