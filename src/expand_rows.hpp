#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstddef>
namespace soft {
// Distinct buffers, bounded renderer targets. Same nearest integer mapping;
// repeated destination rows reuse the already-computed identical row.
inline void expandRows565(uint16_t* destination,unsigned dstStride,unsigned dstWidth,unsigned dstHeight,const uint16_t* source,unsigned srcStride,unsigned srcWidth,unsigned srcHeight){
 assert(dstWidth&&dstWidth<=640&&dstHeight&&srcWidth&&srcHeight&&dstWidth<=dstStride&&srcWidth<=srcStride&&destination!=source);
 unsigned columns[640];if(dstWidth!=srcWidth)for(unsigned x=0;x<dstWidth;++x)columns[x]=x*srcWidth/dstWidth;
 unsigned previous=~0u;
 for(unsigned y=0;y<dstHeight;++y){unsigned row=y*srcHeight/dstHeight;uint16_t* dst=destination+size_t(y)*dstStride;
  if(row==previous)std::copy_n(dst-dstStride,dstWidth,dst);
  else{const uint16_t* src=source+size_t(row)*srcStride;if(dstWidth==srcWidth)std::copy_n(src,dstWidth,dst);else for(unsigned x=0;x<dstWidth;++x)dst[x]=src[columns[x]];}
  previous=row;
 }
}
}
