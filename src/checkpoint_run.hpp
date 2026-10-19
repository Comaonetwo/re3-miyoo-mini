#pragma once
#include "driving.hpp"
namespace soft {
class CheckpointRun {
    std::vector<Vec3> route_;
    Vec3 previous_;
    size_t next_=0;
    double seconds_=0;
    bool started_=false;
public:
    CheckpointRun(std::vector<Vec3> route,Vec3 start);
    static CheckpointRun street(const CollisionWorld& world,Vec3 start,float yaw);
    void reset(Vec3 start);
    void update(Vec3 position,float speed,double dt,bool active);
    void draw(Renderer& renderer,const Camera& camera) const;
    std::vector<Triangle> marker() const;
    const std::vector<Vec3>& route() const {return route_;}
    size_t completed() const {return next_;}
    bool finished() const {return next_==route_.size();}
    double seconds() const {return seconds_;}
    bool started() const {return started_;}
};
class MiniMap {
    static constexpr int side=72;
    Vec3 center_;
    float span_=300;
    std::vector<uint16_t> pixels_;
    Vec2 map(Vec3 p) const;
public:
    MiniMap(const CollisionWorld& world,Vec3 start,float yaw);
    void draw(std::vector<uint16_t>& frame,Vec3 position,float yaw,const CheckpointRun& run) const;
};
void drawText(std::vector<uint16_t>& frame,int x,int y,const char* text);
void drawDrivingHud(std::vector<uint16_t>& frame,const MiniMap& map,const CheckpointRun& run,
                    const Driving& driver,double fps,float farZ,bool textured,bool paused,bool driveMode);
}
