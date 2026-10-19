#pragma once
#include <algorithm>
#include <cmath>
namespace soft {
// Bound the same float recurrence as the global certificate. Restrict only
// its mathematical extrema, retaining the error for the entire original bbox.
inline double tileDepthUpper(float row,float dx,float dy,int width,int height,
                             int firstX,int lastX,int firstY,int lastY) {
    double ex=double(dx)*width,ey=double(dy)*height;
    double error=.002*(std::fabs(double(row))+std::fabs(ex)+std::fabs(ey))+1e-7;
    return double(row)+std::max(double(dx)*firstX,double(dx)*lastX)
           +std::max(double(dy)*firstY,double(dy)*lastY)+error;
}
}
