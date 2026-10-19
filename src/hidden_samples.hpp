#pragma once
#include <cstddef>
namespace soft {
// Only widths below rasterSpan's 16-pixel threshold use this exact replay.
// Reproduce the edge/depth additions of the real rasterizer, including the
// initial + 0*dx. No epsilon or vertex-depth approximation is involved.
inline bool hiddenSamples(const float* depth,unsigned stride,int width,int height,
 const float* edge,const float* dx,const float* dy,const bool* topLeft,
 float row,float zx,float zy){
 float rows[3]={edge[0],edge[1],edge[2]};
 for(int y=0;y<height;++y){float e[3]={rows[0]+0*dx[0],rows[1]+0*dx[1],rows[2]+0*dx[2]},z=row;
  for(int x=0;x<width;++x){
   if((e[0]>0||(e[0]==0&&topLeft[0]))&&(e[1]>0||(e[1]==0&&topLeft[1]))&&(e[2]>0||(e[2]==0&&topLeft[2]))&&z>depth[size_t(y)*stride+x])return false;
   for(unsigned k=0;k<3;++k)e[k]+=dx[k];z+=zx;
  }
  for(unsigned k=0;k<3;++k)rows[k]+=dy[k];row+=zy;
 }return true;
}
}
