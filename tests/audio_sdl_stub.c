/* Deterministic native SDL audio ABI double, for IPC/mixer integration tests. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
typedef struct {int freq;uint16_t format;uint8_t channels,silence;uint16_t samples,padding;uint32_t size;void (*callback)(void*,uint8_t*,int);void* userdata;} Spec;
static unsigned opens;static int opened;
static Spec spec;static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;static pthread_t thread;static int stopped,paused=1;static FILE* output;
int SDL_Init(uint32_t flags){return flags==0x10?0:-1;}
static uint64_t micros(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000+t.tv_nsec/1000;}
static void* worker(void* unused){(void)unused;uint64_t next=micros();unsigned period=(unsigned)((uint64_t)spec.samples*1000000/spec.freq)*(getenv("GTA3_AUDIO_TEST_SLOW_FIRST")&&opens==1?2:1);
 while(!__atomic_load_n(&stopped,__ATOMIC_ACQUIRE)){pthread_mutex_lock(&mutex);if(!paused){uint8_t bytes[32768];unsigned length=spec.samples*2;spec.callback(spec.userdata,bytes,length);fwrite(bytes,1,length,output);fflush(output);}pthread_mutex_unlock(&mutex);
  // Absolute cadence: callback/file work must not accumulate into device drift.
  next+=period;uint64_t now=micros();if(next>now)usleep((unsigned)(next-now));
 }return NULL;}

int SDL_OpenAudio(Spec* desired,Spec* actual){++opens;stopped=0;paused=1;spec=*desired;*actual=spec;output=fopen(getenv("GTA3_AUDIO_TEST_OUTPUT"),opens==1?"wb":"ab");if(!output)return -1;int result=pthread_create(&thread,NULL,worker,NULL);opened=result==0;return result;}
void SDL_PauseAudio(int value){pthread_mutex_lock(&mutex);paused=value;pthread_mutex_unlock(&mutex);}
void SDL_LockAudio(void){pthread_mutex_lock(&mutex);}void SDL_UnlockAudio(void){pthread_mutex_unlock(&mutex);}
void SDL_CloseAudio(void){if(!opened)return;opened=0;__atomic_store_n(&stopped,1,__ATOMIC_RELEASE);pthread_join(thread,NULL);fclose(output);}void SDL_Quit(void){}
