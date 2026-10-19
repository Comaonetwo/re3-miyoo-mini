#pragma once
#include "soft_renderer.hpp"
#include "raster_span.hpp"
#include "depth_row.hpp"
#include "covered_run.hpp"
#include "texel_run.hpp"
#include "packet_sampler.hpp"
namespace soft {
// The legacy flat path quantizes tint before modulation and rounds by 31/63.
// Keep this distinct from the general 8-bit colour tables (divide by 255).
struct PackedTintTables {
 uint8_t five[32*32],six[64*64];
 PackedTintTables(){for(unsigned c=0;c<32;++c)for(unsigned x=0;x<32;++x)five[c*32+x]=(c*x+15)/31;for(unsigned c=0;c<64;++c)for(unsigned x=0;x<64;++x)six[c*64+x]=(c*x+31)/63;}
 static const PackedTintTables& get(){static const PackedTintTables t;return t;}
};
struct PackedWorldJob {
 uint16_t* image;float* depth;unsigned stride;Stats& stats;const Texture& texture;
 int x0,x1,y0,y1;float edge[3],edgeDx[3],edgeDy[3];bool topLeft[3];
 float rz,ru,rv,zx,ux,vx,zy,uy,vy;bool affine,cullRows,edgeRuns,texelRuns,packetSampler,depthBlocks;unsigned alphaRef;
 const uint8_t* red;const uint8_t* green;const uint8_t* blue;
};
template<bool Wrap> inline int packedSample(float uv,int size){float f=uv*size;int n=int(f);if(float(n)>f)--n;return Wrap?n&(size-1):std::max(0,std::min(size-1,n));}
#if defined(__GNUC__) || defined(__clang__)
#define SOFT_PACKED_INLINE inline __attribute__((always_inline))
#else
#define SOFT_PACKED_INLINE inline
#endif
template<bool Covered,bool Wrap,bool White,bool Batch,bool Opaque> SOFT_PACKED_INLINE void packedSimpleSegment(PackedWorldJob& j,int y,int first,int last,float inverse,float* e,float& z,float& u,float& v,uint32_t& written,uint32_t& perspective,uint32_t& divisions){
  // The renderer owns separate output buffers. Job metadata, texture storage
  // and tint tables stay immutable for the raster call; capture inputs before
  // stores so float steps/dimensions need not be reloaded at every pixel.
  uint16_t* const image=j.image;float* const depth=j.depth;const uint16_t* const texels=j.texture.pixels.data();const uint8_t* const alpha=j.texture.alpha.data();
  const int w=j.texture.width,h=j.texture.height;const uint8_t *red=j.red,*green=j.green,*blue=j.blue;
  const float zx=j.zx,ux=j.ux,vx=j.vx,ex0=j.edgeDx[0],ex1=j.edgeDx[1],ex2=j.edgeDx[2];
  const bool affine=j.affine,tl0=j.topLeft[0],tl1=j.topLeft[1],tl2=j.topLeft[2];const unsigned alphaRef=j.alphaRef;const size_t base=size_t(y)*j.stride;auto& stats=j.stats;
  for(int x=j.x0+first;x<=j.x0+last;++x){
   if(Covered||((e[0]>0||(e[0]==0&&tl0))&&(e[1]>0||(e[1]==0&&tl1))&&(e[2]>0||(e[2]==0&&tl2)))){size_t index=base+x;
    if(z>depth[index]){float iz=affine?inverse:1/z;if(affine){if(Batch)++perspective;else ++stats.perspectiveSaved;}else if(!Opaque)++divisions;
     size_t sample=size_t(packedSample<Wrap>(v*iz,h))*w+packedSample<Wrap>(u*iz,w);
     if(Opaque||alpha[sample]>=alphaRef){uint16_t p=texels[sample];image[index]=White?p:uint16_t((red[p>>11]<<11)|(green[(p>>5)&63]<<5)|blue[p&31]);depth[index]=z;if(Batch)++written;else ++stats.pixels;}
    }
   }if(!Covered){e[0]+=ex0;e[1]+=ex1;e[2]+=ex2;}z+=zx;u+=ux;v+=vx;
  }
}
template<bool Wrap,bool White,bool Batch,bool Opaque> SOFT_PACKED_INLINE void packedBlockedSegment(PackedWorldJob& j,int y,int first,int last,float inverse,float* e,float& z,float& u,float& v,uint32_t& written,uint32_t& perspective,uint32_t& divisions){
 if(!j.depthBlocks||last-first+1<64){packedSimpleSegment<true,Wrap,White,Batch,Opaque>(j,y,first,last,inverse,e,z,u,v,written,perspective,divisions);return;}
 int x=first;
 for(;x+31<=last;x+=32){const float* depth=j.depth+size_t(y)*j.stride+j.x0+x;
  // Failed probes only decline culling. The conservative bound and full
  // scan prove that every original strict depth test would fail.
  if(depth[0]>z&&hiddenDepthRow(depth,0,31,z,j.zx)){
   for(unsigned k=0;k<32;++k){z+=j.zx;u+=j.ux;v+=j.vx;}
   ++j.stats.hiddenDepthBlocks;j.stats.hiddenBlockSamples+=32;
  }else packedSimpleSegment<true,Wrap,White,Batch,Opaque>(j,y,x,x+31,inverse,e,z,u,v,written,perspective,divisions);
 }
 if(x<=last)packedSimpleSegment<true,Wrap,White,Batch,Opaque>(j,y,x,last,inverse,e,z,u,v,written,perspective,divisions);
}
template<bool Covered,bool Wrap,bool White,bool Batch,bool Opaque,bool Experimental> SOFT_PACKED_INLINE void packedSegment(PackedWorldJob& j,int y,int first,int last,float inverse,float* e,float& z,float& u,float& v,uint32_t& written,uint32_t& perspective,uint32_t& divisions,uint32_t& packetPixels,uint32_t& packetFallbacks,uint32_t& texelOpaquePixels){
 if(Experimental&&Covered&&packetSamplerAvailable&&j.packetSampler&&!j.affine&&last-first+1>=4){
  int x=first;
  for(;x+3<=last;x+=4){float zz=z,uu=u,vv=v,zs[4],us[4],vs[4];bool visible[4];unsigned count=0;
   for(unsigned k=0;k<4;++k){zs[k]=zz;us[k]=uu;vs[k]=vv;visible[k]=zz>j.depth[size_t(y)*j.stride+j.x0+x+k];count+=visible[k];zz+=j.zx;uu+=j.ux;vv+=j.vx;}
   if(count<2){packedSimpleSegment<true,Wrap,White,Batch,Opaque>(j,y,x,x+3,inverse,e,z,u,v,written,perspective,divisions);continue;}
   size_t samples[4];bool fast[4];packetSamples<Wrap>(zs,us,vs,j.texture.width,j.texture.height,samples,fast);unsigned fallback=0;
   for(unsigned k=0;k<4;++k)if(visible[k]){size_t sample=samples[k];if(!fast[k]){float iz=1/zs[k];if(!Opaque)++divisions;++fallback;sample=size_t(packedSample<Wrap>(vs[k]*iz,j.texture.height))*j.texture.width+packedSample<Wrap>(us[k]*iz,j.texture.width);}
    if(Opaque||j.texture.alpha[sample]>=j.alphaRef){size_t index=size_t(y)*j.stride+j.x0+x+k;uint16_t p=j.texture.pixels[sample];j.image[index]=White?p:uint16_t((j.red[p>>11]<<11)|(j.green[(p>>5)&63]<<5)|j.blue[p&31]);j.depth[index]=zs[k];if(Batch)++written;else ++j.stats.pixels;}
   }
   packetPixels+=count;packetFallbacks+=fallback;z=zz;u=uu;v=vv;
  }
  if(x<=last)packedSimpleSegment<true,Wrap,White,Batch,Opaque>(j,y,x,last,inverse,e,z,u,v,written,perspective,divisions);return;
 }
 // Heuristic only declines reuse; the certificate decides correctness.
 bool sparse=Experimental&&Covered&&j.texelRuns&&last-first+1>=8&&(std::fabs(j.ux)*j.texture.width+std::fabs(j.vx)*j.texture.height)<std::fabs(z)*.12f;
 if(!sparse){if(Covered){if(Experimental)packedBlockedSegment<Wrap,White,Batch,Opaque>(j,y,first,last,inverse,e,z,u,v,written,perspective,divisions);else packedSimpleSegment<true,Wrap,White,Batch,Opaque>(j,y,first,last,inverse,e,z,u,v,written,perspective,divisions);}else packedSimpleSegment<false,Wrap,White,Batch,Opaque>(j,y,first,last,inverse,e,z,u,v,written,perspective,divisions);return;}
 int x=first;
 for(;x+7<=last;x+=8){size_t sample=0;
  if(z>j.depth[size_t(y)*j.stride+j.x0+x]&&constantTexelRun<Wrap>(z,u,v,j.zx,j.ux,j.vx,j.texture.width,j.texture.height,j.affine,inverse,sample,divisions)){
   bool alpha=Opaque||j.texture.alpha[sample]>=j.alphaRef;uint16_t p=j.texture.pixels[sample];uint16_t pixel=White?p:uint16_t((j.red[p>>11]<<11)|(j.green[(p>>5)&63]<<5)|j.blue[p&31]);
   uint32_t beforeWritten=written;uint64_t beforeStatsPixels=j.stats.pixels;
   for(unsigned k=0;k<8;++k){size_t index=size_t(y)*j.stride+j.x0+x+k;if(z>j.depth[index]){if(j.affine){if(Batch)++perspective;else ++j.stats.perspectiveSaved;}if(alpha){j.image[index]=pixel;j.depth[index]=z;if(Batch)++written;else ++j.stats.pixels;}}z+=j.zx;u+=j.ux;v+=j.vx;}
   if(Opaque)texelOpaquePixels+=uint32_t(Batch?written-beforeWritten:j.stats.pixels-beforeStatsPixels);
   ++j.stats.texelRunBlocks;j.stats.texelRunPixels+=8;
  }else packedSimpleSegment<true,Wrap,White,Batch,Opaque>(j,y,x,x+7,inverse,e,z,u,v,written,perspective,divisions);
 }
 if(x<=last)packedSimpleSegment<true,Wrap,White,Batch,Opaque>(j,y,x,last,inverse,e,z,u,v,written,perspective,divisions);
}
template<bool Wrap,bool White,bool Batch,bool Opaque,bool Experimental> inline void packedWorldKernel(PackedWorldJob& j){
 uint32_t written=0,perspective=0,skipped=0,hiddenRows=0,hiddenSamples=0,divisions=0,packetPixels=0,packetFallbacks=0,texelOpaquePixels=0;
 uint64_t beforePixels=j.stats.pixels;
 float row[3]={j.edge[0],j.edge[1],j.edge[2]},rz=j.rz,ru=j.ru,rv=j.rv;
 for(int y=j.y0;y<=j.y1;++y){float inverse=j.affine?1/rz:0;if(j.affine)++divisions;int first,last;rasterSpan(row[0],row[1],row[2],j.edgeDx[0],j.edgeDx[1],j.edgeDx[2],j.x1-j.x0+1,first,last);
  if(j.cullRows&&hiddenDepthRow(j.depth+size_t(y)*j.stride+j.x0,first,last,rz,j.zx)){
   unsigned skip=unsigned(j.x1-j.x0+1)-unsigned(std::max(0,last-first+1));if(Batch)skipped+=skip;else j.stats.spanSkippedPixels+=skip;
   ++hiddenRows;hiddenSamples+=unsigned(last-first+1);
   for(unsigned k=0;k<3;++k)row[k]+=j.edgeDy[k];rz+=j.zy;ru+=j.uy;rv+=j.vy;continue;
  }
  float e[3]={row[0]+first*j.edgeDx[0],row[1]+first*j.edgeDx[1],row[2]+first*j.edgeDx[2]},z=rz,u=ru,v=rv;
  for(int skip=0;skip<first;++skip){z+=j.zx;u+=j.ux;v+=j.vx;}
  if(Batch)skipped+=unsigned(j.x1-j.x0+1)-unsigned(std::max(0,last-first+1));else j.stats.spanSkippedPixels+=unsigned(j.x1-j.x0+1)-unsigned(std::max(0,last-first+1));
  uint8_t tail=0;
  if(j.edgeRuns&&coveredEdgeRun(e,j.edgeDx,first,last,&tail)){
   packedSegment<false,Wrap,White,Batch,Opaque,Experimental>(j,y,first,first+2,inverse,e,z,u,v,written,perspective,divisions,packetPixels,packetFallbacks,texelOpaquePixels);
   packedSegment<true,Wrap,White,Batch,Opaque,Experimental>(j,y,first+3,last-3,inverse,e,z,u,v,written,perspective,divisions,packetPixels,packetFallbacks,texelOpaquePixels);
   for(unsigned i=0;i<3;++i){int x=last-2+int(i);
    if(tail&(1u<<i))packedSegment<true,Wrap,White,Batch,Opaque,Experimental>(j,y,x,x,inverse,e,z,u,v,written,perspective,divisions,packetPixels,packetFallbacks,texelOpaquePixels);
    else{z+=j.zx;u+=j.ux;v+=j.vx;}
   }
   j.stats.coverageRunPixels+=unsigned(last-first-5);
  }else packedSegment<false,Wrap,White,Batch,Opaque,Experimental>(j,y,first,last,inverse,e,z,u,v,written,perspective,divisions,packetPixels,packetFallbacks,texelOpaquePixels);
 for(unsigned i=0;i<3;++i)row[i]+=j.edgeDy[i];rz+=j.zy;ru+=j.uy;rv+=j.vy;
 }
 if(Opaque&&!j.affine)divisions+=uint32_t((Batch?written:j.stats.pixels-beforePixels)-packetPixels+packetFallbacks-texelOpaquePixels);
 j.stats.packetPixels+=packetPixels;j.stats.packetFallbacks+=packetFallbacks;j.stats.texelOpaquePixels+=texelOpaquePixels;
 j.stats.packedReciprocals+=divisions;
 j.stats.hiddenDepthRows+=hiddenRows;j.stats.hiddenDepthSamples+=hiddenSamples;
 if(Batch){j.stats.pixels+=written;j.stats.perspectiveSaved+=perspective;j.stats.spanSkippedPixels+=skipped;}
}
// The ordinary scalar configuration needs none of the optional packet/texel/
// depth-block code. Keep that loop compact; preserve the original full kernel
// whenever a diagnostic option actually requests it.
template<bool Wrap,bool White,bool Batch=true,bool Opaque=false> inline void packedWorld(PackedWorldJob& j){
 if(j.packetSampler||j.texelRuns||j.depthBlocks)packedWorldKernel<Wrap,White,Batch,Opaque,true>(j);
 else{++j.stats.leanPackedTriangles;packedWorldKernel<Wrap,White,Batch,Opaque,false>(j);}
}
}
#undef SOFT_PACKED_INLINE
