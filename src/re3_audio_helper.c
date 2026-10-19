#include "port_version.h"
#define _GNU_SOURCE
/* Uses the console's SDL 1.2 audio ABI; no vendor library is redistributed. */
#include "re3_audio_ipc.h"
#include "sfx_mixer.h"
#include "stream_ring.h"
#include "audio_clock_watch.h"
#include <time.h>
#ifdef __linux__
#include <sys/syscall.h>
#endif
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#ifdef __linux__
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sched.h>
#include <errno.h>
#endif
#include <signal.h>
typedef struct {int freq;uint16_t format;uint8_t channels,silence;uint16_t samples,padding;uint32_t size;void (*callback)(void*,uint8_t*,int);void* userdata;} AudioSpec;
static CpuAudioShared* shared;
static CpuSfxEntry* sfxTable;static uint32_t sfxCount;static uint8_t* rawData;static size_t rawBytes;static CpuSfxVoice voices[CPU_AUDIO_VOICES];static uint32_t voiceSerial[CPU_AUDIO_VOICES];
static int selfTest=0;static uint64_t callbackSamples=0;
static volatile sig_atomic_t stopped=0;
static void stop(int sig){(void)sig;stopped=1;}
static CpuStreamRing rings[2];static uint64_t audioFrames,audioCalls,mixPeakUs,clippedSamples;
static uint64_t micros(void){struct timespec t;
#ifdef __linux__
 // Avoid a new GLIBC_2.17 clock_gettime dependency on the console runtime.
 if(syscall(SYS_clock_gettime,CLOCK_MONOTONIC,&t)==0)return (uint64_t)t.tv_sec*1000000+t.tv_nsec/1000;
#else
 if(clock_gettime(CLOCK_MONOTONIC,&t)==0)return (uint64_t)t.tv_sec*1000000+t.tv_nsec/1000;
#endif
 struct timeval wall;gettimeofday(&wall,NULL);return (uint64_t)wall.tv_sec*1000000+wall.tv_usec;}
static FILE* files[2];static uint32_t serials[2];static uint64_t samples[2];
static void mixInner(void* unused,uint8_t* bytes,int length){(void)unused;memset(bytes,0,length);if(selfTest){int16_t* out=(int16_t*)bytes;for(int i=0;i<length/2;++i){out[i]=callbackSamples<2*CPU_AUDIO_RATE?(((callbackSamples*880/CPU_AUDIO_RATE)&1)?1000:-1000):0;++callbackSamples;}return;}if(__atomic_load_n(&shared->menuPaused,__ATOMIC_ACQUIRE))return;
 int16_t* output=(int16_t*)bytes;for(int offset=0;offset<length/2;offset+=512){unsigned frames=length/2-offset;if(frames>512)frames=512;int32_t accum[512]={0};
  for(unsigned s=0;s<2;++s){if(!rings[s].active||__atomic_load_n(&shared->stream[s].paused,__ATOMIC_ACQUIRE))continue;
   unsigned volume=__atomic_load_n(&shared->stream[s].volume,__ATOMIC_ACQUIRE)*__atomic_load_n(&shared->master,__ATOMIC_ACQUIRE)*__atomic_load_n(&shared->fade,__ATOMIC_ACQUIRE);unsigned gain=audioGain(volume);int16_t buffer[512];size_t n=streamRingRead(&rings[s],buffer,frames);if(n<frames&&!rings[s].eof)++rings[s].underruns;
   for(size_t k=0;k<n;++k)accum[k]+=audioScale(buffer[k],gain);samples[s]+=n;
   if(rings[s].eof&&!rings[s].count){rings[s].active=0;__atomic_store_n(&shared->stream[s].playing,0,__ATOMIC_RELEASE);}__atomic_store_n(&shared->stream[s].position,(uint32_t)(samples[s]*1000/CPU_AUDIO_RATE),__ATOMIC_RELEASE);
  }
  if(rawData)for(unsigned c=0;c<CPU_AUDIO_VOICES;++c){if(!voices[c].active)continue;CpuAudioVoice* cmd=&shared->voice[c];unsigned volume=__atomic_load_n(&cmd->volume,__ATOMIC_ACQUIRE)*__atomic_load_n(&shared->effects,__ATOMIC_ACQUIRE)*__atomic_load_n(&shared->effectsFade,__ATOMIC_ACQUIRE);
   sfxMix(&voices[c],rawData,accum,frames,__atomic_load_n(&cmd->rate,__ATOMIC_ACQUIRE),volume);__atomic_store_n(&cmd->playing,(uint32_t)voices[c].active,__ATOMIC_RELEASE);}
  for(unsigned i=0;i<frames;++i){int value=accum[i];if(value>32767||value< -32768)++clippedSamples;output[offset+i]=value>32767?32767:value< -32768?-32768:value;}
 }}
static void mix(void* unused,uint8_t* bytes,int length){uint64_t start=micros();mixInner(unused,bytes,length);uint64_t end=micros();if(end>=start&&end-start>mixPeakUs)mixPeakUs=end-start;audioFrames+=length/2;++audioCalls;}
static void audioScheduling(void){
#ifdef __linux__
 cpu_set_t available;if(!sched_getaffinity(0,sizeof(available),&available)&&CPU_ISSET(0,&available)&&CPU_ISSET(1,&available)){cpu_set_t target;CPU_ZERO(&target);CPU_SET(1,&target);int rc=sched_setaffinity(0,sizeof(target),&target);fprintf(stderr,"AUDIO affinity cpu=1 result=%d\n",rc);}
 errno=0;int rc=setpriority(PRIO_PROCESS,0,-5);fprintf(stderr,"AUDIO priority nice=-5 result=%d errno=%d\n",rc,errno);
#endif
}
int main(int argc,char** argv){setvbuf(stdout,NULL,_IONBF,0);fprintf(stderr,"AUDIO HELPER v" GTA3_PORT_VERSION " buffered start\n");if(argc!=2){fprintf(stderr,"AUDIO invalid args\n");return 2;}selfTest=!strcmp(argv[1],"--self-test");CpuAudioShared testShared={0};
 if(selfTest)shared=&testShared;else{int fd=atoi(argv[1]);struct stat ipc;if(fstat(fd,&ipc)||ipc.st_size!=sizeof(*shared)){fprintf(stderr,"AUDIO IPC size mismatch; expected=%zu\n",sizeof(*shared));close(fd);return 2;}shared=mmap(NULL,sizeof(*shared),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);if(shared==MAP_FAILED){perror("AUDIO IPC mmap failed");close(fd);return 2;}close(fd);if(shared->abi!=CPU_AUDIO_ABI){fprintf(stderr,"AUDIO IPC version mismatch\n");return 2;}}
 pid_t parent=getppid();
#ifdef __linux__
 prctl(PR_SET_PDEATHSIG,SIGTERM);
#endif
if(getppid()!=parent)return 2;
 signal(SIGTERM,stop);signal(SIGINT,stop);
 audioScheduling();
 const char* override=getenv("GTA3_AUDIO_SDL");
 void* lib=dlopen(override?override:"/mnt/SDCARD/miyoo/lib/libSDL-1.2.so.0",RTLD_NOW|RTLD_LOCAL);if(!lib)lib=dlopen("libSDL-1.2.so.0",RTLD_NOW|RTLD_LOCAL);
 if(!lib){const char* error=dlerror();fprintf(stderr,"AUDIO dlopen failed: %s\n",error?error:"unknown loader error");__atomic_store_n(&shared->ready,2,__ATOMIC_RELEASE);return 3;}
 const char* (*getError)(void)=dlsym(lib,"SDL_GetError");char* (*driverName)(char*,int)=dlsym(lib,"SDL_AudioDriverName");const char* (*currentDriver)(void)=dlsym(lib,"SDL_GetCurrentAudioDriver");
 fprintf(stderr,"AUDIO SDL loaded; initializing device\n");int (*init)(uint32_t)=lib?dlsym(lib,"SDL_Init"):NULL;int (*openAudio)(AudioSpec*,AudioSpec*)=lib?dlsym(lib,"SDL_OpenAudio"):NULL;
 void (*pauseAudio)(int)=lib?dlsym(lib,"SDL_PauseAudio"):NULL;void (*lockAudio)(void)=lib?dlsym(lib,"SDL_LockAudio"):NULL;void (*unlockAudio)(void)=lib?dlsym(lib,"SDL_UnlockAudio"):NULL;void (*closeAudio)(void)=lib?dlsym(lib,"SDL_CloseAudio"):NULL;void (*quit)(void)=lib?dlsym(lib,"SDL_Quit"):NULL;
 if(!init||!openAudio||!pauseAudio||!lockAudio||!unlockAudio||!closeAudio||!quit||init(0x10)){fprintf(stderr,"AUDIO SDL load/init failed: %s\n",getError?getError():"missing SDL audio symbol");__atomic_store_n(&shared->ready,2,__ATOMIC_RELEASE);return 3;}
 if(!selfTest){int raw=open("audio/sfx.raw",O_RDONLY);struct stat info;if(raw>=0&&fstat(raw,&info)==0&&info.st_size>0&&info.st_size<=128*1024*1024){rawBytes=(size_t)info.st_size;sfxTable=sfxReadTable("audio/sfx.sdt",rawBytes,&sfxCount);if(sfxTable){void* data=mmap(NULL,rawBytes,PROT_READ,MAP_PRIVATE,raw,0);if(data!=MAP_FAILED)rawData=data;}}
 if(raw>=0)close(raw);fprintf(stderr,"AUDIO SFX ready: samples=%u mapped_bytes=%zu enabled=%d\n",sfxCount,rawBytes,rawData!=NULL);}
 AudioSpec desired={0},actual={0};desired.freq=CPU_AUDIO_RATE;desired.format=0x8010;desired.channels=1;desired.samples=512;const char* block=getenv("GTA3_AUDIO_BLOCK");if(block&&!strcmp(block,"1024"))desired.samples=1024;desired.callback=mix;
 if(openAudio(&desired,&actual)||actual.freq!=CPU_AUDIO_RATE||actual.format!=0x8010||actual.channels!=1){fprintf(stderr,"AUDIO SDL format/open failed: %s; actual freq=%d format=%x channels=%u\n",getError?getError():"unknown",actual.freq,actual.format,actual.channels);quit();__atomic_store_n(&shared->ready,2,__ATOMIC_RELEASE);return 4;}
 char driver[64]={0};if(driverName)driverName(driver,sizeof(driver));else if(currentDriver){const char* name=currentDriver();if(name)snprintf(driver,sizeof(driver),"%s",name);}
 fprintf(stderr,"AUDIO device opened: driver=%s freq=%d format=%x channels=%u samples=%u\n",driver,actual.freq,actual.format,actual.channels,actual.samples);
 __atomic_store_n(&shared->ready,1,__ATOMIC_RELEASE);pauseAudio(0);
 if(selfTest){for(int i=0;i<300&&!stopped;++i)usleep(10000);pauseAudio(1);closeAudio();quit();fprintf(stderr,"AUDIO SELF TEST callback_samples=%llu expected>=44100\n",(unsigned long long)callbackSamples);return callbackSamples>=2*CPU_AUDIO_RATE?0:5;}
 unsigned watchMs=5000;const char* watchSetting=getenv("GTA3_AUDIO_WATCH_MS");if(watchSetting){unsigned value=(unsigned)atoi(watchSetting);if(value>=250&&value<=5000)watchMs=value;}
 AudioClockWatch watch={0};lockAudio();uint64_t initialFrames=audioFrames;unlockAudio();audioClockReset(&watch,micros(),initialFrames);
 uint64_t lastClock=micros(),lastFrames=initialFrames,sfxSetupPeak=0;unsigned sfxStarts=0;
 while(!stopped&&!__atomic_load_n(&shared->quit,__ATOMIC_ACQUIRE)&&getppid()==parent){
  for(unsigned c=0;c<CPU_AUDIO_VOICES;++c){CpuAudioVoice* cmd=&shared->voice[c];uint32_t serial=__atomic_load_n(&cmd->serial,__ATOMIC_ACQUIRE);if(serial==voiceSerial[c])continue;uint64_t setupStart=micros();unsigned wanted=cmd->sample;
#ifdef __linux__
 if(rawData&&wanted&&wanted<=sfxCount){CpuSfxEntry entry=sfxTable[wanted-1];long page=sysconf(_SC_PAGESIZE);if(page>0){size_t offset=entry.offset/(size_t)page*(size_t)page;size_t count=entry.bytes+entry.offset-offset;if(count>128*1024)count=128*1024;madvise(rawData+offset,count,MADV_WILLNEED);}}
#endif
 // Fault the beginning of a newly selected sample in the producer, before
 // activation. MADV_WILLNEED alone need not finish before the callback reads it.
 if(rawData&&wanted&&wanted<=sfxCount){CpuSfxEntry entry=sfxTable[wanted-1];size_t bytes=entry.bytes;if(bytes>128*1024)bytes=128*1024;volatile const uint8_t* data=rawData+entry.offset;unsigned warm=0;for(size_t i=0;i<bytes;i+=4096)warm^=data[i];if(bytes)warm^=data[bytes-1];(void)warm;}
 lockAudio();voiceSerial[c]=serial;unsigned id=cmd->sample;voices[c].active=0;if(rawData&&id&&id<=sfxCount)sfxBegin(&voices[c],sfxTable[id-1],cmd->start,cmd->end,cmd->loops);__atomic_store_n(&cmd->playing,(uint32_t)voices[c].active,__ATOMIC_RELEASE);__atomic_store_n(&cmd->ack,serial,__ATOMIC_RELEASE);unlockAudio();uint64_t setupEnd=micros();if(setupEnd>=setupStart&&setupEnd-setupStart>sfxSetupPeak)sfxSetupPeak=setupEnd-setupStart;if(wanted)++sfxStarts;}
  for(unsigned s=0;s<2;++s){uint32_t serial=__atomic_load_n(&shared->stream[s].serial,__ATOMIC_ACQUIRE);
   if(serial!=serials[s]){
    unsigned track=shared->stream[s].track,seek=shared->stream[s].seek;
    lockAudio();rings[s].active=0;rings[s].count=0;rings[s].read=0;rings[s].eof=0;unlockAudio();
    if(files[s])fclose(files[s]);files[s]=NULL;
    if(track&&track<=256){char path[80];snprintf(path,sizeof(path),"audio_pcm/%03u.pcm",track-1);files[s]=fopen(path,"rb");if(!files[s])perror("AUDIO PCM open failed");if(files[s]&&fseek(files[s],(long)((uint64_t)seek*CPU_AUDIO_RATE/1000*2),SEEK_SET)){fclose(files[s]);files[s]=NULL;}}
    // Prime before publishing ready/playing; storage never holds SDL's lock.
    int16_t prime[4096];unsigned filled=0;int eof=files[s]==NULL;
    while(files[s]&&filled<CPU_STREAM_RING_FRAMES){unsigned count=CPU_STREAM_RING_FRAMES-filled;if(count>4096)count=4096;size_t n=fread(prime,2,count,files[s]);lockAudio();streamRingWrite(&rings[s],prime,n);unlockAudio();filled+=n;if(n<count){eof=1;break;}}
    lockAudio();serials[s]=serial;samples[s]=(uint64_t)seek*CPU_AUDIO_RATE/1000;rings[s].eof=eof;rings[s].active=files[s]!=NULL&&filled!=0;
    __atomic_store_n(&shared->stream[s].position,seek,__ATOMIC_RELEASE);__atomic_store_n(&shared->stream[s].playing,rings[s].active,__ATOMIC_RELEASE);__atomic_store_n(&shared->stream[s].ack,serial,__ATOMIC_RELEASE);unlockAudio();
   }else if(files[s]){
    lockAudio();unsigned space=CPU_STREAM_RING_FRAMES-rings[s].count;int eof=rings[s].eof;unlockAudio();
    if(!eof&&space>=4096){int16_t buffer[4096];size_t n=fread(buffer,2,4096,files[s]);lockAudio();streamRingWrite(&rings[s],buffer,n);if(n<4096)rings[s].eof=1;unlockAudio();}
   }
  }
  uint64_t now=micros();if(now>=lastClock+5000000){lockAudio();uint64_t frames=audioFrames,calls=audioCalls,peak=mixPeakUs,under=rings[0].underruns+rings[1].underruns;unsigned q0=rings[0].count,q1=rings[1].count,active=0;for(unsigned c=0;c<CPU_AUDIO_VOICES;++c)active+=voices[c].active!=0;uint64_t clipped=clippedSamples;clippedSamples=0;mixPeakUs=0;unlockAudio();fprintf(stderr,"AUDIO CLOCK rate=%.1f expected=%d callbacks=%llu mix_peak_us=%llu underruns=%llu queued=%u/%u active_sfx=%u clipped_samples=%llu sfx_starts=%u sfx_setup_peak_us=%llu\n",(frames-lastFrames)*1000000.0/(now-lastClock),CPU_AUDIO_RATE,(unsigned long long)calls,(unsigned long long)peak,(unsigned long long)under,q0,q1,active,(unsigned long long)clipped,sfxStarts,(unsigned long long)sfxSetupPeak);sfxStarts=0;sfxSetupPeak=0;lastFrames=frames;lastClock=now;}
  lockAudio();uint64_t delivered=audioFrames;unlockAudio();double deliveryRate=0;
  if(audioClockObserve(&watch,micros(),delivered,CPU_AUDIO_RATE,watchMs,&deliveryRate)){
   fprintf(stderr,"AUDIO RESYNC begin measured=%.1f expected=%d recovery=%u (preserving streams/voices)\n",deliveryRate,CPU_AUDIO_RATE,watch.recoveries);
   pauseAudio(1);closeAudio();memset(&actual,0,sizeof(actual));
   if(openAudio(&desired,&actual)||actual.freq!=CPU_AUDIO_RATE||actual.format!=0x8010||actual.channels!=1){fprintf(stderr,"AUDIO RESYNC failed: %s freq=%d channels=%u\n",getError?getError():"unknown",actual.freq,actual.channels);__atomic_store_n(&shared->ready,2,__ATOMIC_RELEASE);stopped=1;break;}
   // The new SDL device has a new lock. The PCM rings, sample positions,
   // voice phases and command acknowledgements remain in this process.
   lockAudio();delivered=audioFrames;audioClockReset(&watch,micros(),delivered);lastClock=watch.time;lastFrames=delivered;mixPeakUs=0;unlockAudio();pauseAudio(0);
   fprintf(stderr,"AUDIO RESYNC complete freq=%d channels=%u samples=%u\n",actual.freq,actual.channels,actual.samples);
  }
  usleep(5000);
 }
 pauseAudio(1);closeAudio();quit();for(unsigned s=0;s<2;++s)if(files[s])fclose(files[s]);if(rawData)munmap(rawData,rawBytes);free(sfxTable);munmap(shared,sizeof(*shared));return 0;}
