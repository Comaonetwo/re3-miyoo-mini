#pragma once
#include "soft_renderer.hpp"
#include <cmath>
#include <limits>
namespace soft {
struct ModelBounds {
    Vec3 minimum{0,0,0},maximum{0,0,0};bool valid=false;
    void include(Vec3 p){if(!valid){minimum=maximum=p;valid=true;}else{
        minimum={std::min(minimum.x,p.x),std::min(minimum.y,p.y),std::min(minimum.z,p.z)};
        maximum={std::max(maximum.x,p.x),std::max(maximum.y,p.y),std::max(maximum.z,p.z)};}}
};
// Camera transform coefficients already include the model transform used by
// Im3D. Double precision here bounds the float vertex path conservatively.
struct ModelFrustum {
    double planes[6][4],magnitude[3][4];double vertical,sx,sy,nearZ,farZ;
    ModelFrustum(const float matrix[12],float yScale,const Camera& camera):vertical(yScale),sx(160/camera.focal),sy(120/camera.focal),nearZ(camera.nearZ),farZ(camera.farZ){
        for(unsigned i=0;i<4;++i){double x=-matrix[i],y=matrix[4+i]*vertical,z=matrix[8+i];
            planes[0][i]=z;planes[1][i]=-z;planes[2][i]=x+sx*z;planes[3][i]=sx*z-x;planes[4][i]=y+sy*z;planes[5][i]=sy*z-y;
            magnitude[0][i]=std::fabs(matrix[i]);magnitude[1][i]=std::fabs(matrix[4+i]);magnitude[2][i]=std::fabs(matrix[8+i]);}
        planes[0][3]-=nearZ;planes[1][3]+=farZ;
    }
    double boundMargin(const ModelBounds& b) const {
        if(!b.valid)return std::numeric_limits<double>::infinity();
        double lo[3]={b.minimum.x,b.minimum.y,b.minimum.z},hi[3]={b.maximum.x,b.maximum.y,b.maximum.z},scale=0;
        for(unsigned axis=0;axis<3;++axis){double sum=magnitude[axis][3];for(unsigned i=0;i<3;++i){if(!std::isfinite(lo[i])||!std::isfinite(hi[i])||hi[i]<lo[i])return std::numeric_limits<double>::infinity();sum+=magnitude[axis][i]*std::max(std::fabs(lo[i]),std::fabs(hi[i]));}scale+=sum;}
        return 1e-4+8e-6*(scale*(1+std::fabs(vertical)+std::fabs(sx)+std::fabs(sy))+std::fabs(nearZ)+std::fabs(farZ));
    }
    // Whole-model margin bounds the rounding error of every enclosed cluster.
    // Bit 7 means entirely outside; low six bits select intersected planes.
    unsigned activePlanes(const ModelBounds& b,double margin)const {
        if(!b.valid||!std::isfinite(margin))return 63;
        double lo[3]={b.minimum.x,b.minimum.y,b.minimum.z},hi[3]={b.maximum.x,b.maximum.y,b.maximum.z};unsigned mask=0;
        for(unsigned k=0;k<6;++k){double lower=planes[k][3],upper=lower;for(unsigned i=0;i<3;++i){double c=planes[k][i];lower+=c*(c>=0?lo[i]:hi[i]);upper+=c*(c>=0?hi[i]:lo[i]);}
            if(!std::isfinite(lower)||!std::isfinite(upper))return 63;
            if(upper < -margin)return 128;
            if(!(lower>margin))mask|=1u<<k;
        }return mask;
    }
    bool outsideMasked(const ModelBounds& b,unsigned mask,double margin)const {
        if(!b.valid||!std::isfinite(margin))return false;
        double lo[3]={b.minimum.x,b.minimum.y,b.minimum.z},hi[3]={b.maximum.x,b.maximum.y,b.maximum.z};
        for(unsigned k=0;k<6;++k)if(mask&(1u<<k)){double upper=planes[k][3];for(unsigned i=0;i<3;++i){double c=planes[k][i];upper+=c*(c>=0?hi[i]:lo[i]);}if(std::isfinite(upper)&&upper < -margin)return true;}
        return false;
    }
    bool outside(const ModelBounds& b) const {
        if(!b.valid)return false;
        double lo[3]={b.minimum.x,b.minimum.y,b.minimum.z},hi[3]={b.maximum.x,b.maximum.y,b.maximum.z};
        double absolute[3],center[3],extent[3];for(unsigned i=0;i<3;++i){if(!std::isfinite(lo[i])||!std::isfinite(hi[i])||hi[i]<lo[i])return false;
            absolute[i]=std::max(std::fabs(lo[i]),std::fabs(hi[i]));center[i]=(lo[i]+hi[i])*.5;extent[i]=(hi[i]-lo[i])*.5;}
        double errorScale=0;for(unsigned axis=0;axis<3;++axis){double scale=magnitude[axis][3];for(unsigned i=0;i<3;++i)scale+=magnitude[axis][i]*absolute[i];errorScale+=scale;}
        // Includes float dot products, Y adaptation and frustum comparison.
        // Large cancellation, near-plane contact and uncertain inputs fall back.
        double margin=1e-4+8e-6*(errorScale*(1+std::fabs(vertical)+std::fabs(sx)+std::fabs(sy))+std::fabs(nearZ)+std::fabs(farZ));
        if(!std::isfinite(margin))return false;
        for(auto& plane:planes){double maximum=plane[3];for(unsigned i=0;i<3;++i)maximum+=plane[i]*center[i]+std::fabs(plane[i])*extent[i];if(std::isfinite(maximum)&&maximum < -margin)return true;}
        return false;
    }
};
// Prepare once per atomic. The whole-box margin dominates the original
// per-cluster transform margin; extra error covers coefficient conversion and
// float support-point accumulation, including cancellation. Uncertain inputs
// retain geometry. This deliberately rejects a subset of the double path.
struct PreparedModelFrustum {
    float planes[6][4],margin=0;unsigned count=0; bool valid=false;
    PreparedModelFrustum(const ModelFrustum& source,const ModelBounds& whole,bool enabled=true,unsigned mask=63){
        if(!enabled)return;
        double base=source.boundMargin(whole);
        if(!whole.valid||!std::isfinite(base))return;
        // boundMargin includes 8e-6 times the absolute transform/plane sum.
        // 64 float eps < 8e-6 covers conversion, support products and additions.
        // A second copy therefore covers float errors beyond the old margin.
        margin=std::nextafter(float(2*base),std::numeric_limits<float>::infinity());
        if(!std::isfinite(margin))return;
        for(unsigned k=0;k<6;++k)if(mask&(1u<<k)){
            for(unsigned j=0;j<4;++j){planes[count][j]=float(source.planes[k][j]);if(!std::isfinite(planes[count][j]))return;}++count;
        }
        valid=true;
    }
    // Only cached, ordered, finite bounds enclosed by the constructor's whole
    // box may use this entry. boundsFor certifies these on geometry mutation.
    bool outsideCached(const ModelBounds& box)const{
        if(!valid||!box.valid)return false;
        for(unsigned k=0;k<count;++k){const auto& p=planes[k];
            float upper=p[3];
            upper+=p[0]*(p[0]>=0?box.maximum.x:box.minimum.x);
            upper+=p[1]*(p[1]>=0?box.maximum.y:box.minimum.y);
            upper+=p[2]*(p[2]>=0?box.maximum.z:box.minimum.z);
            if(std::isfinite(upper)&&upper < -margin)return true;
        }
        return false;
    }
    bool outside(const ModelBounds& box)const{
        const float lo[3]={box.minimum.x,box.minimum.y,box.minimum.z},hi[3]={box.maximum.x,box.maximum.y,box.maximum.z};
        for(unsigned j=0;j<3;++j)if(!std::isfinite(lo[j])||!std::isfinite(hi[j])||hi[j]<lo[j])return false;
        return outsideCached(box);
    }
};
}
