#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace soft {
// Prove the same signs as the original binary32 edge recurrences. The three
// leading additions are performed exactly; the remaining error is bounded
// conservatively for at most 640 candidates. Ambiguous tail signs fall back.
inline bool coveredEdgeRun(const float* edge,const float* step,int first,int last,uint8_t* tail=nullptr){
 int count=last-first+1;if(count<64||count>640)return false;uint8_t mask=7;
 for(unsigned k=0;k<3;++k){float start=edge[k];for(unsigned i=0;i<3;++i)start+=step[k];
  double end=double(start)+double(step[k])*(count-7);
  double margin=.0001*(std::fabs(double(start))+std::fabs(double(step[k]))*(count-7))+1e-7;
  if(!(std::min(double(start),end)>margin))return false;
  if(tail){double bound=.0001*(std::fabs(double(edge[k]))+std::fabs(double(step[k]))*(count-1))+1e-7;
   for(unsigned j=0;j<3;++j){double value=double(edge[k])+double(step[k])*(count-3+j);
    if(value>bound)continue;if(value< -bound)mask&=uint8_t(~(1u<<j));else return false;
   }
  }
 }
 if(tail)*tail=mask;return true;
}
}
