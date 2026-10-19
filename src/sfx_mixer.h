#pragma once
#include "re3_sfx.h"
#include "stream_ring.h"
#include <string.h>
typedef struct {CpuSfxEntry entry;uint64_t position;uint32_t start,end,left;int active;} CpuSfxVoice;
static void sfxBegin(CpuSfxVoice* v,CpuSfxEntry e,uint32_t start,uint32_t end,uint32_t count){memset(v,0,sizeof(*v));v->entry=e;uint32_t frames=e.bytes/2;v->end=end&&end<=frames?end:frames;v->start=start<v->end?start:0;v->left=count;v->active=frames!=0;}
static void sfxMix(CpuSfxVoice* v,const uint8_t* raw,int32_t* out,unsigned frames,unsigned rate,unsigned volume){if(!v->active)return;uint64_t step=(uint64_t)rate*65536/22050;if(!step)step=1;
 unsigned gain=audioGain(volume);uint32_t end=v->left==1?v->entry.bytes/2:v->end;
 for(unsigned i=0;i<frames;++i){uint64_t pos=v->position>>16;if(pos>=end){if(v->left==1){v->active=0;break;}if(v->left>1)--v->left;uint64_t span=(uint64_t)(end-v->start)<<16;v->position=((uint64_t)v->start<<16)+(v->position-((uint64_t)end<<16))%span;pos=v->position>>16;}
  const uint8_t* p=raw+v->entry.offset+pos*2;int sample=(int16_t)((unsigned)p[0]|(unsigned)p[1]<<8);out[i]+=audioScale(sample,gain);v->position+=step;
 }
 if((v->position>>16)>=v->entry.bytes/2&&v->left==1)v->active=0;
}
