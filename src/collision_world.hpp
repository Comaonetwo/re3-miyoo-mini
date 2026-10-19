#pragma once
#include "soft_renderer.hpp"
namespace soft {
struct CollisionFace {Vec3 v[3];float slope,minY,maxY;};
class CollisionWorld {
    float minX_=0,minZ_=0;unsigned columns_=0,rows_=0;
    std::vector<CollisionFace> faces_;
    std::vector<std::vector<uint32_t>> cells_;
    static constexpr float cellSize=12;
public:
    static CollisionWorld load(const std::string& path);
    explicit CollisionWorld(const std::vector<Triangle>& triangles);
    bool groundHeight(float x,float z,float minY,float maxY,float& height) const;
    bool blocked(float x,float z,float bottom,float top,float radius) const;
    size_t faceCount() const {return faces_.size();}
};
}
