#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/* Original PC SDT is 20-byte little-endian metadata for signed mono PCM16. */
typedef struct {uint32_t offset,bytes,rate,loopStart;int32_t loopEnd;} CpuSfxEntry;
static uint32_t sfxU32(const uint8_t* p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static CpuSfxEntry* sfxReadTable(const char* path,uint64_t rawBytes,uint32_t* count){*count=0;FILE* f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);rewind(f);
 if(n<=0||n%20||n/20>5000){fclose(f);return NULL;}CpuSfxEntry* entries=(CpuSfxEntry*)calloc(n/20,sizeof(CpuSfxEntry));if(!entries){fclose(f);return NULL;}
 for(long i=0;i<n/20;++i){uint8_t b[20];if(fread(b,1,20,f)!=20){free(entries);fclose(f);return NULL;}CpuSfxEntry e={sfxU32(b),sfxU32(b+4),sfxU32(b+8),sfxU32(b+12),(int32_t)sfxU32(b+16)};
  if((e.offset|e.bytes|e.loopStart)&1||!e.bytes||(uint64_t)e.offset+e.bytes>rawBytes||e.rate<1000||e.rate>192000||e.loopStart>=e.bytes||(e.loopEnd!=-1&&(e.loopEnd<=0||(uint32_t)e.loopEnd>e.bytes||(e.loopEnd&1)))){free(entries);fclose(f);return NULL;}entries[i]=e;}
 fclose(f);*count=(uint32_t)(n/20);return entries;}
