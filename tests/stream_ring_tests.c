#include "stream_ring.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
int main(void){CpuStreamRing* r=calloc(1,sizeof(*r));int16_t in[4096],out[4096];for(int i=0;i<4096;++i)in[i]=i;
 for(int round=0;round<100;++round){assert(streamRingWrite(r,in,4096)==4096);assert(streamRingRead(r,out,3091)==3091);for(int i=0;i<3091;++i)assert(out[i]==in[i]);assert(streamRingRead(r,out,2000)==1005);for(int i=0;i<1005;++i)assert(out[i]==in[i+3091]);assert(r->count==0);}
 for(int i=0;i<8;++i)assert(streamRingWrite(r,in,4096)==4096);assert(streamRingWrite(r,in,1)==0);assert(r->count==CPU_STREAM_RING_FRAMES);free(r);
 for(unsigned v=0;v<=127;++v)for(unsigned m=0;m<=127;m+=7)for(unsigned f=0;f<=127;f+=9){unsigned volume=v*m*f,gain=audioGain(volume);for(int sample=-32768;sample<=32767;sample+=1023){int exact=(int)((int64_t)sample*volume/(127*127*127));assert(abs(audioScale(sample,gain)-exact)<=1);}}
 puts("PASS ring wrap, bounded writes/reads, FIFO, Q16 volume error <= 1 PCM unit");return 0;}
