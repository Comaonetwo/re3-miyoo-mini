#pragma once
#include "asset_scene.hpp"
#include "collision_world.hpp"
namespace soft {
class Driving {
    Vec3 spawn_;float spawnYaw_;
public:
    Vec3 position{0,0,0};float yaw=0,speed=0,pitch=0,roll=0,wheelAngle=0,steeringAngle=0;bool hit=false;
    Driving(const CollisionWorld& world,Vec3 spawn,float yaw);
    void reset(const CollisionWorld& world);
    void update(const CollisionWorld& world,float dt,float throttle,float steer,bool brake);
    Camera chaseCamera(float farZ=80,const CollisionWorld* world=nullptr) const;
    Vec3 animateWheelVertex(Vec3 vertex,unsigned wheel) const;
    void draw(Renderer& renderer,const AssetScene& car,const Camera& camera,bool textured) const;
};
// Cache trigonometry once per pose; shared by direct and librw drawing.
struct DrivingPose {
    Vec3 position;float co,si,cp,sp,cr,sr,cw,sw,cs,ss;
    explicit DrivingPose(const Driving& driver);
    Vec3 transform(Vec3 vertex,unsigned wheel=4) const;
};
}
