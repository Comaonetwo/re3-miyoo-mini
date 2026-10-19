#include "collision_world.hpp"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace soft {
constexpr float CollisionWorld::cellSize;
namespace {
float cross(float ax,float az,float bx,float bz){return ax*bz-az*bx;}
float distance2(float x,float z,Vec3 a,Vec3 b){float dx=b.x-a.x,dz=b.z-a.z,n=dx*dx+dz*dz;
    float t=n>1e-8f?std::max(0.f,std::min(1.f,((x-a.x)*dx+(z-a.z)*dz)/n)):0;
    dx=a.x+t*dx-x;dz=a.z+t*dz-z;return dx*dx+dz*dz;}
bool cylinderTriangle(const CollisionFace& face,float x,float z,float bottom,float top,float radius){
    Vec3 a[8],b[8];std::copy(face.v,face.v+3,a);int n=3;
    for(int plane=0;plane<2&&n;++plane){int m=0;auto prev=a[n-1];float dp=plane?top-prev.y:prev.y-bottom;
        for(int i=0;i<n;++i){auto cur=a[i];float dc=plane?top-cur.y:cur.y-bottom;
            if((dp>=0)!=(dc>=0)){float t=dp/(dp-dc);b[m++]={prev.x+t*(cur.x-prev.x),prev.y+t*(cur.y-prev.y),prev.z+t*(cur.z-prev.z)};}
            if(dc>=0)b[m++]=cur;prev=cur;dp=dc;
        }n=m;std::copy(b,b+n,a);
    }
    if(!n)return false;bool positive=false,negative=false;float area=0;
    for(int i=0;i<n;++i){auto p=a[i],q=a[(i+1)%n];
        if(distance2(x,z,p,q)<=radius*radius)return true;
        float side=cross(q.x-p.x,q.z-p.z,x-p.x,z-p.z);positive|=side>1e-5f;negative|=side< -1e-5f;
        area+=cross(p.x,p.z,q.x,q.z);
    }
    return std::fabs(area)>1e-5f&&!(positive&&negative);
}
}
CollisionWorld CollisionWorld::load(const std::string& path){
    std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Cannot open collision: "+path);
    auto u32=[&](){uint8_t b[4];if(!f.read(reinterpret_cast<char*>(b),4))throw std::runtime_error("Truncated collision");
        return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);};
    char magic[8];if(!f.read(magic,8)||std::memcmp(magic,"M3COL001",8))throw std::runtime_error("Invalid collision format");
    uint32_t count=u32();if(!count||count>100000)throw std::runtime_error("Collision triangle budget exceeded");
    std::vector<Triangle> tris(count);
    for(auto& t:tris)for(auto& v:t.v){float coords[3];for(auto& coord:coords){auto n=u32();std::memcpy(&coord,&n,4);}
        v={coords[0],coords[1],coords[2]};}
    if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Trailing collision data");
    return CollisionWorld(tris);
}
CollisionWorld::CollisionWorld(const std::vector<Triangle>& triangles){
    if(triangles.empty()||triangles.size()>100000)throw std::runtime_error("Invalid collision triangle count");
    float maxX=-1e5f,maxZ=-1e5f;minX_=minZ_=1e5f;
    for(auto t:triangles){for(auto v:t.v){
        if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||std::fabs(v.x)>10000||std::fabs(v.z)>10000||std::fabs(v.y)>5000)
            throw std::runtime_error("Invalid collision coordinates");
        minX_=std::min(minX_,v.x);minZ_=std::min(minZ_,v.z);maxX=std::max(maxX,v.x);maxZ=std::max(maxZ,v.z);}
        Vec3 ab{t.v[1].x-t.v[0].x,t.v[1].y-t.v[0].y,t.v[1].z-t.v[0].z},ac{t.v[2].x-t.v[0].x,t.v[2].y-t.v[0].y,t.v[2].z-t.v[0].z};
        Vec3 normal{ab.y*ac.z-ab.z*ac.y,ab.z*ac.x-ab.x*ac.z,ab.x*ac.y-ab.y*ac.x};
        float length=std::sqrt(normal.x*normal.x+normal.y*normal.y+normal.z*normal.z);if(length<1e-6f)continue;
        CollisionFace face{{t.v[0],t.v[1],t.v[2]},std::fabs(normal.y)/length,
            std::min({t.v[0].y,t.v[1].y,t.v[2].y}),std::max({t.v[0].y,t.v[1].y,t.v[2].y})};faces_.push_back(face);
    }
    columns_=unsigned(std::floor((maxX-minX_)/cellSize))+1;rows_=unsigned(std::floor((maxZ-minZ_)/cellSize))+1;
    if(columns_>128||rows_>128||faces_.empty())throw std::runtime_error("Collision grid budget exceeded");
    cells_.resize(size_t(columns_)*rows_);size_t refs=0;
    for(uint32_t i=0;i<faces_.size();++i){auto& t=faces_[i];
        int x0=int((std::min({t.v[0].x,t.v[1].x,t.v[2].x})-minX_)/cellSize),x1=int((std::max({t.v[0].x,t.v[1].x,t.v[2].x})-minX_)/cellSize);
        int z0=int((std::min({t.v[0].z,t.v[1].z,t.v[2].z})-minZ_)/cellSize),z1=int((std::max({t.v[0].z,t.v[1].z,t.v[2].z})-minZ_)/cellSize);
        for(int z=z0;z<=z1;++z)for(int x=x0;x<=x1;++x){if(++refs>2000000)throw std::runtime_error("Collision grid reference budget exceeded");
            cells_[size_t(z)*columns_+x].push_back(i);}
    }
}
bool CollisionWorld::groundHeight(float x,float z,float minY,float maxY,float& height) const{
    if(!std::isfinite(x)||!std::isfinite(z)||!std::isfinite(minY)||!std::isfinite(maxY)||minY>maxY||std::fabs(x)>10000||std::fabs(z)>10000||std::fabs(minY)>5000||std::fabs(maxY)>5000)return false;
    int cx=int(std::floor((x-minX_)/cellSize)),cz=int(std::floor((z-minZ_)/cellSize));
    if(cx<0||cz<0||cx>=int(columns_)||cz>=int(rows_))return false;bool found=false;height=minY;
    for(auto id:cells_[size_t(cz)*columns_+cx]){auto& t=faces_[id];if(t.slope<.65f||t.maxY<minY||t.minY>maxY)continue;
        auto a=t.v[0],b=t.v[1],c=t.v[2];float det=cross(b.x-a.x,b.z-a.z,c.x-a.x,c.z-a.z);
        float u=cross(x-a.x,z-a.z,c.x-a.x,c.z-a.z)/det,v=cross(b.x-a.x,b.z-a.z,x-a.x,z-a.z)/det;
        if(u< -1e-4f||v< -1e-4f||u+v>1.0001f)continue;float y=a.y+u*(b.y-a.y)+v*(c.y-a.y);
        if(y>=minY&&y<=maxY&&(!found||y>height)){height=y;found=true;}
    }
    return found;
}
bool CollisionWorld::blocked(float x,float z,float bottom,float top,float radius) const{
    if(!std::isfinite(x)||!std::isfinite(z)||!std::isfinite(bottom)||!std::isfinite(top)||!std::isfinite(radius)||radius<0||radius>10||bottom>top||std::fabs(x)>10000||std::fabs(z)>10000||std::fabs(bottom)>5000||std::fabs(top)>5000)return true;
    int x0=int(std::floor((x-radius-minX_)/cellSize)),x1=int(std::floor((x+radius-minX_)/cellSize));
    int z0=int(std::floor((z-radius-minZ_)/cellSize)),z1=int(std::floor((z+radius-minZ_)/cellSize));
    x0=std::max(0,x0);z0=std::max(0,z0);x1=std::min(int(columns_)-1,x1);z1=std::min(int(rows_)-1,z1);
    for(int cz=z0;cz<=z1;++cz)for(int cx=x0;cx<=x1;++cx)for(auto id:cells_[size_t(cz)*columns_+cx]){
        auto& t=faces_[id];if(t.slope>=.65f||t.minY>top||t.maxY<bottom)continue;
        if(cylinderTriangle(t,x,z,bottom,top,radius))return true;
    }
    return false;
}
}
