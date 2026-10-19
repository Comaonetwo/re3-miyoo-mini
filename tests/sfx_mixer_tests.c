#include "sfx_mixer.h"
#include <assert.h>
#include <stdio.h>
int main(void){uint8_t raw[]={232,3,208,7,184,11,160,15};CpuSfxEntry e={0,8,22050,0,-1};CpuSfxVoice v;int32_t out[8]={0};
 sfxBegin(&v,e,0,4,1);sfxMix(&v,raw,out,8,22050,127*127*127);assert(out[0]==1000&&out[3]==4000&&out[4]==0&&!v.active);
 memset(out,0,sizeof(out));sfxBegin(&v,e,1,3,0);sfxMix(&v,raw,out,8,22050,127*127*127);assert(out[0]==1000&&out[3]==2000&&out[4]==3000&&v.active);
 memset(out,0,sizeof(out));sfxBegin(&v,e,0,4,1);sfxMix(&v,raw,out,4,44100,127*127*127);assert(out[0]==1000&&out[1]==3000&&out[2]==0&&!v.active);
 memset(out,0,sizeof(out));sfxBegin(&v,e,0,4,1);sfxMix(&v,raw,out,8,11025,0);assert(!v.active&&out[7]==0);
 FILE* f=fopen("synthetic-sfx.sdt","wb");assert(f);uint8_t bad[20]={};fwrite(bad,1,20,f);fclose(f);uint32_t n=99;assert(!sfxReadTable("synthetic-sfx.sdt",8,&n)&&n==0);remove("synthetic-sfx.sdt");
 puts("PASS bounded SFX metadata, one-shot/loop, resampling, zero-volume advancement");return 0;}
