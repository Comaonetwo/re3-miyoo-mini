#pragma once
#include "soft_renderer.hpp"
#include "world_channels.hpp"
#include "exact_rgb_step.hpp"
#include "raster_span.hpp"
#include "depth_row.hpp"
#include "exact_float_advance.hpp"
#include "modulation_tables.hpp"
#include "black_fade.hpp"
#include "covered_run.hpp"
#include <cmath>
namespace soft {
struct NearestWorldJob {
 uint16_t* image;float* depth;unsigned stride;Stats& stats;const Texture& texture;
 int x0,x1,y0,y1;float edge[3],edgeDx[3],edgeDy[3];bool topLeft[3];
 float* row;const float* dx;const float* dy;
 unsigned red,green,blue,alpha,alphaRef,blend;bool white,perspectiveUV,rowReciprocal,advance,cullRows,writeDepth,coverage,opaque;
 const ModulationTables* modulation;
 const uint8_t* alphaRed;const uint8_t* alphaGreen;const uint8_t* alphaBlue;
 const uint16_t* tintRed;const uint16_t* tintGreen;const uint16_t* tintBlue;bool fixedAlpha,whiteBlend,vectorChannels,rgbRegister;
};
inline unsigned worldChannel(float value){return unsigned(std::max(0.f,std::min(255.f,value+.5f)));}
template<bool Wrap> inline int worldSample(float uv,int size){
 float f=(Wrap?uv:std::max(-1.f,std::min(2.f,uv)))*size;int n=int(f);if(float(n)>f)--n;
 return Wrap?n&(size-1):std::max(0,std::min(size-1,n));
}
#if defined(__GNUC__) || defined(__clang__)
#define SOFT_NEAREST_INLINE inline __attribute__((always_inline))
#else
#define SOFT_NEAREST_INLINE inline
#endif
template<bool Covered,bool Flat,bool Wrap,bool Tables,bool Batch,bool ZeroFade,bool AlphaTable,bool WriteDepth,bool Opaque,bool FixedAlpha=false,bool WhiteBlend=false,bool RegisterColor=false>
SOFT_NEAREST_INLINE void nearestWorldSegment(NearestWorldJob& job,int y,int first,int last,float rowInverse,float& e0,float& e1,float& e2,float& z,float& u,float& v,float* col,uint32_t& written,uint32_t& perspective,uint32_t& tinted,const BlackFade* fade){
 auto& stats=job.stats;
 // Output arrays do not alias the read-only texture/job inputs. Preserve the
 // original per-pixel arithmetic while loading invariant metadata only once.
 uint16_t* const image=job.image;float* const depth=job.depth;
 const uint16_t* const texels=job.texture.pixels.data();const uint8_t* const texAlpha=job.texture.alpha.data();
 const int width=job.texture.width,height=job.texture.height;
 const size_t base=size_t(y)*job.stride;const float *dx=job.dx;
 const float zx=dx[0],ux=dx[1],vx=dx[2];
 const float colorDx[4]={Flat?0.f:dx[3],Flat?0.f:dx[4],Flat?0.f:dx[5],(Flat||FixedAlpha)?0.f:dx[6]};
 const float ex0=job.edgeDx[0],ex1=job.edgeDx[1],ex2=job.edgeDx[2];
 const bool tl0=job.topLeft[0],tl1=job.topLeft[1],tl2=job.topLeft[2];
 const bool rowReciprocal=job.rowReciprocal,perspectiveUV=job.perspectiveUV;
 const unsigned flatAlpha=job.alpha,alphaRef=job.alphaRef;
 RgbRegisterSteps registerRgb(col,colorDx);
  for(int x=first;x<=last;++x){
   if(Covered||((e0>0||(e0==0&&tl0))&&(e1>0||(e1==0&&tl1))&&(e2>0||(e2==0&&tl2)))){size_t index=base+x;
    if(z>depth[index]){float iz=rowReciprocal?rowInverse:(!Flat||(perspectiveUV&&!(Opaque&&ZeroFade)))&&z>1e-8f?1/z:1;
     if(rowReciprocal){if(Batch)++perspective;else ++stats.perspectiveSaved;}
     float uvScale=perspectiveUV?iz:1;
     size_t texel=size_t(worldSample<Wrap>(v*uvScale,height))*width+worldSample<Wrap>(u*uvScale,width);
     unsigned alpha=(Flat||FixedAlpha)?flatAlpha:worldChannel(col[3]*iz);if(!Opaque)alpha=alpha*texAlpha[texel]/255;
     if(alpha>=alphaRef){uint16_t tex=texels[texel],pixel;
      if(ZeroFade){pixel=(*fade)(image[index],alpha);}
      else if(AlphaTable){uint16_t old=image[index];pixel=uint16_t((unsigned(job.alphaRed[((tex>>11)<<5)|(old>>11)])<<11)|(unsigned(job.alphaGreen[(((tex>>5)&63)<<6)|((old>>5)&63)])<<5)|job.alphaBlue[((tex&31)<<5)|(old&31)]);}
      else if(Flat&&job.white&&(job.blend==0||alpha==255))pixel=tex;
      else if(Flat&&job.tintRed&&(job.blend==0||alpha==255)){pixel=uint16_t(job.tintRed[tex>>11]|job.tintGreen[(tex>>5)&63]|job.tintBlue[tex&31]);if(Batch)++tinted;else ++stats.tintPixels;}
      else{unsigned tr=tex>>11,tg=(tex>>5)&63,tb=tex&31;
       unsigned cr,cg,cb;
       if(!Flat&&Opaque&&worldChannelsAvailable&&job.vectorChannels){unsigned channels[4];if(RegisterColor)registerRgb.channels(iz,channels);else worldChannels(col,iz,channels);cr=channels[0];cg=channels[1];cb=channels[2];}
       else{cr=Flat?job.red:worldChannel(col[0]*iz);cg=Flat?job.green:worldChannel(col[1]*iz);cb=Flat?job.blue:worldChannel(col[2]*iz);}
       bool pureWhite=WhiteBlend;unsigned r=pureWhite?((tr<<3)|(tr>>2)):Tables?job.modulation->five[(cr<<5)|tr]:cr*((tr<<3)|(tr>>2))/255,g=pureWhite?((tg<<2)|(tg>>4)):Tables?job.modulation->six[(cg<<6)|tg]:cg*((tg<<2)|(tg>>4))/255,b=pureWhite?((tb<<3)|(tb>>2)):Tables?job.modulation->five[(cb<<5)|tb]:cb*((tb<<3)|(tb>>2))/255;
       if(job.blend==1&&alpha!=255){uint16_t old=image[index];unsigned dr=old>>11,dg=(old>>5)&63,db=old&31;
        r=(r*alpha+((dr<<3)|(dr>>2))*(255-alpha)+127)/255;g=(g*alpha+((dg<<2)|(dg>>4))*(255-alpha)+127)/255;b=(b*alpha+((db<<3)|(db>>2))*(255-alpha)+127)/255;}
       pixel=rgb(r,g,b);
      }image[index]=pixel;if(WriteDepth)depth[index]=z;if(Batch)++written;else ++stats.pixels;
     }
    }
   }if(!Covered){e0+=ex0;e1+=ex1;e2+=ex2;}z+=zx;u+=ux;v+=vx;if(!Flat){if(RegisterColor)registerRgb.advance();else for(unsigned i=0;i<(FixedAlpha?3u:4u);++i)col[i]+=colorDx[i];}
  }
 if(RegisterColor)registerRgb.storeRgb(col);
}
// Compile separate flat/gradient and repeat/clamp loops. The sampler contract
// and required colour interpolants are fixed for the entire triangle, so the
// common 3D pass avoids testing them at every fragment. All float recurrences
// and integer modulation/blending retain the original evaluation order.
template<bool Flat,bool Wrap,bool Tables=false,bool Batch=true,bool ZeroFade=false,bool AlphaTable=false,bool WriteDepth=true,bool Opaque=false,bool FixedAlpha=false,bool WhiteBlend=false,bool RegisterColor=false> inline void nearestWorld(NearestWorldJob& job){
 uint32_t written=0,perspective=0,skipped=0,tinted=0,registerSteps=0;const BlackFade* fade=ZeroFade?&BlackFade::get():nullptr;
 float eRow0=job.edge[0],eRow1=job.edge[1],eRow2=job.edge[2];
 auto& t=job.texture;auto& stats=job.stats;const float* dx=job.dx;const float* dy=job.dy;float* row=job.row;
 for(int y=job.y0;y<=job.y1;++y){int first,last;rasterSpan(eRow0,eRow1,eRow2,job.edgeDx[0],job.edgeDx[1],job.edgeDx[2],job.x1-job.x0+1,first,last);
  if(job.cullRows&&hiddenDepthRow(job.depth+size_t(y)*job.stride+job.x0,first,last,row[0],dx[0])){
   unsigned outside=unsigned(job.x1-job.x0+1)-unsigned(std::max(0,last-first+1));if(Batch)skipped+=outside;else stats.spanSkippedPixels+=outside;
   ++stats.hiddenDepthRows;stats.hiddenDepthSamples+=unsigned(last-first+1);
   eRow0+=job.edgeDy[0];eRow1+=job.edgeDy[1];eRow2+=job.edgeDy[2];for(unsigned i=0;i<(Flat?3u:FixedAlpha?6u:7u);++i)row[i]+=dy[i];continue;
  }
  float e0=eRow0+first*job.edgeDx[0],e1=eRow1+first*job.edgeDx[1],e2=eRow2+first*job.edgeDx[2],z=row[0],u=row[1],v=row[2],col[4]={row[3],row[4],row[5],row[6]};
  if(job.advance&&first>=128){z=exactFloatAdvance(z,dx[0],first,&stats.attributeSavedAdds);u=exactFloatAdvance(u,dx[1],first,&stats.attributeSavedAdds);v=exactFloatAdvance(v,dx[2],first,&stats.attributeSavedAdds);if(!Flat)for(unsigned i=0;i<(FixedAlpha?3u:4u);++i)col[i]=exactFloatAdvance(col[i],dx[i+3],first,&stats.attributeSavedAdds);}
  else for(int skip=0;skip<first;++skip){z+=dx[0];u+=dx[1];v+=dx[2];if(!Flat)for(unsigned i=0;i<(FixedAlpha?3u:4u);++i)col[i]+=dx[i+3];}
  float rowInverse=job.rowReciprocal&&row[0]>1e-8f?1/row[0]:1;
  if(RegisterColor)registerSteps+=unsigned(std::max(0,last-first+1));
  if(Batch)skipped+=unsigned(job.x1-job.x0+1)-unsigned(std::max(0,last-first+1));else stats.spanSkippedPixels+=unsigned(job.x1-job.x0+1)-unsigned(std::max(0,last-first+1));
  float edges[3]={e0,e1,e2};uint8_t tail=0;
  if(job.coverage&&last-first+1>=96&&coveredEdgeRun(edges,job.edgeDx,first,last,&tail)){
   nearestWorldSegment<false,Flat,Wrap,Tables,Batch,ZeroFade,AlphaTable,WriteDepth,Opaque,FixedAlpha,WhiteBlend,RegisterColor>(job,y,job.x0+first,job.x0+first+2,rowInverse,e0,e1,e2,z,u,v,col,written,perspective,tinted,fade);
   nearestWorldSegment<true,Flat,Wrap,Tables,Batch,ZeroFade,AlphaTable,WriteDepth,Opaque,FixedAlpha,WhiteBlend,RegisterColor>(job,y,job.x0+first+3,job.x0+last-3,rowInverse,e0,e1,e2,z,u,v,col,written,perspective,tinted,fade);
   for(unsigned k=0;k<3;++k){int x=job.x0+last-2+int(k);
    if(tail&(1u<<k))nearestWorldSegment<true,Flat,Wrap,Tables,Batch,ZeroFade,AlphaTable,WriteDepth,Opaque,FixedAlpha,WhiteBlend,RegisterColor>(job,y,x,x,rowInverse,e0,e1,e2,z,u,v,col,written,perspective,tinted,fade);
    else{z+=dx[0];u+=dx[1];v+=dx[2];if(!Flat)for(unsigned i=0;i<(FixedAlpha?3u:4u);++i)col[i]+=dx[i+3];}
   }
   stats.nearestCoveragePixels+=unsigned(last-first-5);
  }else nearestWorldSegment<false,Flat,Wrap,Tables,Batch,ZeroFade,AlphaTable,WriteDepth,Opaque,FixedAlpha,WhiteBlend,RegisterColor>(job,y,job.x0+first,job.x0+last,rowInverse,e0,e1,e2,z,u,v,col,written,perspective,tinted,fade);
eRow0+=job.edgeDy[0];eRow1+=job.edgeDy[1];eRow2+=job.edgeDy[2];for(unsigned i=0;i<(Flat?3u:FixedAlpha?6u:7u);++i)row[i]+=dy[i];
 }
 if(RegisterColor)stats.rgbRegisterSteps+=registerSteps;
 if(ZeroFade){stats.blackFadePixels+=written;}if(AlphaTable){stats.flatAlphaPixels+=written;}
 if(WhiteBlend)stats.whiteBlendPixels+=written;
 if(Batch){stats.pixels+=written;stats.perspectiveSaved+=perspective;stats.spanSkippedPixels+=skipped;stats.tintPixels+=tinted;}
}
// Depth-write state is invariant for a triangle. Keep a separate no-write
// loop for sorted alpha geometry; never change submission order or depth tests.
template<bool Flat,bool Wrap,bool Tables=false,bool Batch=true,bool ZeroFade=false,bool AlphaTable=false,bool FixedAlpha=false,bool WhiteBlend=false>
inline void nearestWorldResolved(NearestWorldJob& job){
 if constexpr(!Flat&&Wrap&&Tables&&!Batch&&!ZeroFade&&!AlphaTable&&FixedAlpha&&!WhiteBlend){
  if(job.opaque&&job.writeDepth&&job.rgbRegister&&job.vectorChannels&&worldChannelsAvailable&&job.x1-job.x0>=23&&job.y1-job.y0>=7&&rgbRegisterSafe(job.row+3,job.dx+3,job.dy+3)){
   ++job.stats.rgbRegisterTriangles;auto before=job.stats.pixels;
   nearestWorld<Flat,Wrap,Tables,Batch,ZeroFade,AlphaTable,true,true,FixedAlpha,WhiteBlend,true>(job);
   job.stats.nearestOpaquePixels+=job.stats.pixels-before;return;
  }
 }
 if(job.opaque){auto before=job.stats.pixels;
  if(job.writeDepth)nearestWorld<Flat,Wrap,Tables,Batch,ZeroFade,AlphaTable,true,true,FixedAlpha,WhiteBlend>(job);
  else nearestWorld<Flat,Wrap,Tables,Batch,ZeroFade,AlphaTable,false,true,FixedAlpha,WhiteBlend>(job);
  job.stats.nearestOpaquePixels+=job.stats.pixels-before;
 }else{
  if(job.writeDepth)nearestWorld<Flat,Wrap,Tables,Batch,ZeroFade,AlphaTable,true,false,FixedAlpha,WhiteBlend>(job);
  else nearestWorld<Flat,Wrap,Tables,Batch,ZeroFade,AlphaTable,false,false,FixedAlpha,WhiteBlend>(job);
 }

}

}

#undef SOFT_NEAREST_INLINE

namespace soft {
template<bool Flat,bool Wrap,bool Tables=false,bool Batch=true,bool ZeroFade=false,bool AlphaTable=false>
inline void nearestWorldDraw(NearestWorldJob& job){
 if constexpr(Flat){if(job.whiteBlend&&job.white&&job.blend==1){nearestWorldResolved<Flat,Wrap,Tables,Batch,ZeroFade,AlphaTable,false,true>(job);return;}}
 if constexpr(!Flat){if(job.fixedAlpha){nearestWorldResolved<Flat,Wrap,Tables,Batch,ZeroFade,AlphaTable,true>(job);return;}}
 nearestWorldResolved<Flat,Wrap,Tables,Batch,ZeroFade,AlphaTable,false>(job);
}
}
