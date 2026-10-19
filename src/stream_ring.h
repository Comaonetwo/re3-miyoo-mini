#pragma once
#include <stdint.h>
#include <string.h>
#define CPU_STREAM_RING_FRAMES 32768u
/* SDL callback and producer access this ring only while holding SDL's lock.
   File reads occur in the producer outside that lock. */
typedef struct {int16_t data[CPU_STREAM_RING_FRAMES];unsigned read,count;int eof,active;uint64_t underruns;} CpuStreamRing;
static unsigned streamRingWrite(CpuStreamRing* r,const int16_t* input,unsigned n){unsigned space=CPU_STREAM_RING_FRAMES-r->count;if(n>space)n=space;unsigned at=(r->read+r->count)%CPU_STREAM_RING_FRAMES,first=CPU_STREAM_RING_FRAMES-at;if(first>n)first=n;memcpy(r->data+at,input,first*2);memcpy(r->data,input+first,(n-first)*2);r->count+=n;return n;}
static unsigned streamRingRead(CpuStreamRing* r,int16_t* output,unsigned n){if(n>r->count)n=r->count;unsigned first=CPU_STREAM_RING_FRAMES-r->read;if(first>n)first=n;memcpy(output,r->data+r->read,first*2);memcpy(output+first,r->data,(n-first)*2);r->read=(r->read+n)%CPU_STREAM_RING_FRAMES;r->count-=n;return n;}
static unsigned audioGain(unsigned volume){return (unsigned)((uint64_t)volume*65536/(127*127*127));}
static int audioScale(int sample,unsigned gain){return (int)((int32_t)sample*(int32_t)gain/65536);}
