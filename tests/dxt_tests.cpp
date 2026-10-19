#include "dxt_decoder.hpp"
#include <cassert>
#include <cstring>
int main(){uint8_t block[16]={},out[4];block[0]=0;block[1]=248;block[2]=224;block[3]=7;
 soft::dxtPixel(block,8,4,4,1,0,0,out);assert(out[0]==255&&out[1]==0&&out[2]==0&&out[3]==255);
 block[4]=1;soft::dxtPixel(block,8,4,4,1,0,0,out);assert(out[0]==0&&out[1]==255);
 std::memset(block,0,16);block[2]=255;block[3]=255;block[4]=3;soft::dxtPixel(block,8,4,4,1,0,0,out);assert(out[3]==0);
 std::memset(block,0,16);block[0]=0xa3;block[9]=248;soft::dxtPixel(block,16,4,4,3,0,0,out);assert(out[0]==255&&out[3]==51);soft::dxtPixel(block,16,4,4,3,1,0,out);assert(out[3]==170);
 block[0]=255;block[1]=0;block[2]=2;soft::dxtPixel(block,16,4,4,5,0,0,out);assert(out[3]==218);
 block[0]=0;block[1]=255;block[2]=6;soft::dxtPixel(block,16,4,4,5,0,0,out);assert(out[3]==0);block[2]=7;soft::dxtPixel(block,16,4,4,5,0,0,out);assert(out[3]==255);
 bool rejected=false;try{soft::dxtPixel(block,15,4,4,5,0,0,out);}catch(...){rejected=true;}assert(rejected);rejected=false;try{soft::dxtPixel(block,16,4,4,5,4,0,out);}catch(...){rejected=true;}assert(rejected);
}
