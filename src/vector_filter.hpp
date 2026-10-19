#pragma once
#include "bilinear_sample.hpp"
#include <cstdint>
namespace soft {
inline int exactFilterFloor(float x){int n=int(x);if(float(n)>x)--n;return n;}
// Exact integer interpolation of three channels in parallel. UV, texel
// addresses, alpha rejection and quantisation are deliberately unchanged.
inline uint16_t vectorBilinear565(const unsigned* p,unsigned fx,unsigned fy){
#if defined(__GNUC__) || defined(__clang__)
 using I4=int32_t __attribute__((vector_size(16)));
 I4 packed={int32_t(p[0]),int32_t(p[1]),int32_t(p[2]),int32_t(p[3])};
 I4 r=packed>>11;r=(r<<3)|(r>>2);
 I4 g=(packed>>5)&I4{63,63,63,63};g=(g<<2)|(g>>4);
 I4 b=packed&I4{31,31,31,31};b=(b<<3)|(b>>2);
 I4 a={r[0],g[0],b[0],0},bb={r[1],g[1],b[1],0},c={r[2],g[2],b[2],0},d={r[3],g[3],b[3],0};
 I4 xx={int32_t(fx),int32_t(fx),int32_t(fx),int32_t(fx)},yy={int32_t(fy),int32_t(fy),int32_t(fy),int32_t(fy)};
 I4 upper=(a<<8)+xx*(bb-a),lower=(c<<8)+xx*(d-c);
 I4 result=((upper<<8)+yy*(lower-upper)+I4{32768,32768,32768,32768})>>16;
 return uint16_t((result[0]>>3)<<11|(result[1]>>2)<<5|(result[2]>>3));
#else
 unsigned r[4],g[4],b[4];for(unsigned i=0;i<4;++i){unsigned rr=p[i]>>11,gg=(p[i]>>5)&63,bb=p[i]&31;r[i]=(rr<<3)|(rr>>2);g[i]=(gg<<2)|(gg>>4);b[i]=(bb<<3)|(bb>>2);}
 return uint16_t((bilinearChannel(r[0],r[1],r[2],r[3],fx,fy)>>3)<<11|(bilinearChannel(g[0],g[1],g[2],g[3],fx,fy)>>2)<<5|(bilinearChannel(b[0],b[1],b[2],b[3],fx,fy)>>3));
#endif
}
}
namespace soft {
inline uint16_t vectorSourceOver565(unsigned r,unsigned g,unsigned b,unsigned alpha,unsigned dr,unsigned dg,unsigned db){
#if defined(__GNUC__) || defined(__clang__)
 using I4=int32_t __attribute__((vector_size(16)));
 I4 source={int32_t(r),int32_t(g),int32_t(b),0},dest={int32_t(dr),int32_t(dg),int32_t(db),0};
 I4 a={int32_t(alpha),int32_t(alpha),int32_t(alpha),0},inv={int32_t(255-alpha),int32_t(255-alpha),int32_t(255-alpha),0};
 I4 n=source*a+dest*inv+I4{127,127,127,0};
 // Exact divide by 255 for 0 <= n <= 65152 (including rounding bias).
 I4 q=(n+I4{1,1,1,1}+(n>>8))>>8;
 return uint16_t((q[0]>>3)<<11|(q[1]>>2)<<5|(q[2]>>3));
#else
 return uint16_t((((r*alpha+dr*(255-alpha)+127)/255)>>3)<<11|(((g*alpha+dg*(255-alpha)+127)/255)>>2)<<5|((b*alpha+db*(255-alpha)+127)/255)>>3);
#endif
}
}
