#pragma once
#include <cstdint>
#include <cstddef>
#include <stdexcept>
namespace soft {
inline void dxtPixel(const uint8_t* bytes,size_t size,unsigned width,unsigned height,unsigned type,unsigned x,unsigned y,uint8_t out[4]){
 if(!bytes||!width||!height||x>=width||y>=height||(type!=1&&type!=3&&type!=5))throw std::runtime_error("Invalid DXT parameters");
 size_t blockBytes=type==1?8:16,cols=(width+3)/4,rows=(height+3)/4;
 if(size!=cols*rows*blockBytes)throw std::runtime_error("Invalid DXT byte count");
 auto block=bytes+((y/4)*cols+x/4)*blockBytes;auto color=block+(type==1?0:8);unsigned pixel=(y%4)*4+x%4;
 unsigned c0=color[0]|unsigned(color[1])<<8,c1=color[2]|unsigned(color[3])<<8;
 uint8_t palette[4][4]={{},{},{},{}};for(int i=0;i<2;++i){unsigned v=i?c1:c0;palette[i][0]=uint8_t(((v>>11)*255+15)/31);palette[i][1]=uint8_t((((v>>5)&63)*255+31)/63);palette[i][2]=uint8_t(((v&31)*255+15)/31);palette[i][3]=255;}
 if(type!=1||c0>c1){for(int j=0;j<3;++j){palette[2][j]=(2*palette[0][j]+palette[1][j])/3;palette[3][j]=(palette[0][j]+2*palette[1][j])/3;}palette[2][3]=palette[3][3]=255;}
 else{for(int j=0;j<3;++j)palette[2][j]=(palette[0][j]+palette[1][j])/2;palette[2][3]=255;}
 unsigned indices=unsigned(color[4])|unsigned(color[5])<<8|unsigned(color[6])<<16|unsigned(color[7])<<24,index=(indices>>(2*pixel))&3;
 for(int j=0;j<4;++j)out[j]=palette[index][j];
 if(type==3)out[3]=((block[pixel/2]>>((pixel%2)*4))&15)*17;
 if(type==5){uint8_t alpha[8]={block[0],block[1]};if(alpha[0]>alpha[1])for(int i=2;i<8;++i)alpha[i]=((8-i)*alpha[0]+(i-1)*alpha[1])/7;
 else{for(int i=2;i<6;++i)alpha[i]=((6-i)*alpha[0]+(i-1)*alpha[1])/5;alpha[6]=0;alpha[7]=255;}
 uint64_t bits=0;for(int i=0;i<6;++i)bits|=uint64_t(block[2+i])<<(8*i);out[3]=alpha[(bits>>(3*pixel))&7];}
}
}
