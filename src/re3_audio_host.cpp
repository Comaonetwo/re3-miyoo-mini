#include "re3_audio_host.hpp"
#include "re3_audio_ipc.h"
#include "re3_sfx.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <sys/stat.h>
#include <cerrno>
#include <fcntl.h>
#include <limits.h>
#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#endif
static CpuAudioShared* shared=nullptr;
static unsigned lengths[256]={};
static char audioRoot[PATH_MAX]={};
void cpuAudioSetRoot(){if(!getcwd(audioRoot,sizeof(audioRoot)))audioRoot[0]=0;}
static int audioPid=-1;
static CpuSfxEntry* sfxTable=nullptr;static uint32_t sfxCount=0;
void cpuAudioTrack(unsigned id,const char* name){if(id>=256)return;char path[PATH_MAX];const char* base=name;for(const char* p=name;*p;++p)if(*p=='/'||*p=='\\')base=p+1;
 char file[64];std::snprintf(file,sizeof(file),"%s",base);for(char* p=file;*p;++p){if(*p>='A'&&*p<='Z')*p+=32;if(*p=='.'){*p=0;break;}}
 std::snprintf(path,sizeof(path),"%s/audio_pcm/%03u.pcm",audioRoot[0]?audioRoot:".",id);struct stat s{};if(!stat(path,&s)&&s.st_size>0&&s.st_size%2==0)lengths[id]=unsigned(uint64_t(s.st_size)*1000/(2*CPU_AUDIO_RATE));
}
bool cpuAudioInit(){
#if defined(__linux__) || defined(__APPLE__)
 if(shared)return __atomic_load_n(&shared->ready,__ATOMIC_ACQUIRE)==1;
 char path[]="/tmp/gta3-audio-XXXXXX";int fd=mkstemp(path);if(fd<0)return false;unlink(path);
 if(ftruncate(fd,sizeof(CpuAudioShared))){close(fd);return false;}
 shared=(CpuAudioShared*)mmap(nullptr,sizeof(*shared),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);if(shared==MAP_FAILED){shared=nullptr;close(fd);return false;}
 std::memset(shared,0,sizeof(*shared));shared->abi=CPU_AUDIO_ABI;shared->effects=shared->effectsFade=127;
 char rawPath[PATH_MAX],tablePath[PATH_MAX];std::snprintf(rawPath,sizeof(rawPath),"%s/audio/sfx.raw",audioRoot);std::snprintf(tablePath,sizeof(tablePath),"%s/audio/sfx.sdt",audioRoot);struct stat raw{};if(!stat(rawPath,&raw)&&raw.st_size>0){sfxTable=sfxReadTable(tablePath,raw.st_size,&sfxCount);std::printf("AUDIO SFX metadata: samples=%u raw_bytes=%lld\n",sfxCount,(long long)raw.st_size);}
 shared->master=shared->fade=127;for(auto& s:shared->stream)s.volume=127;
 int flags=fcntl(fd,F_GETFD);if(flags<0||fcntl(fd,F_SETFD,flags&~FD_CLOEXEC)<0){std::fprintf(stderr,"AUDIO IPC fd flags failed: %s\n",strerror(errno));close(fd);cpuAudioTerminate();return false;}
 char arg[24],defaultHelper[PATH_MAX];std::snprintf(arg,sizeof(arg),"%d",fd);std::snprintf(defaultHelper,sizeof(defaultHelper),"%s/../audio-launch.sh",audioRoot[0]?audioRoot:".");if(access(defaultHelper,X_OK))std::snprintf(defaultHelper,sizeof(defaultHelper),"%s/../re3_audio",audioRoot[0]?audioRoot:".");const char* helper=getenv("GTA3_AUDIO_HELPER");if(!helper)helper=defaultHelper;
 std::printf("AUDIO launch helper=%s root=%s fd=%d\n",helper,audioRoot,fd);
 audioPid=fork();if(audioPid==0){if(audioRoot[0]&&chdir(audioRoot)){dprintf(2,"AUDIO chdir failed: %s\n",strerror(errno));_exit(126);}execl(helper,"re3_audio",arg,(char*)nullptr);dprintf(2,"AUDIO exec failed: %s\n",strerror(errno));_exit(127);}close(fd);
 if(audioPid<0){munmap(shared,sizeof(*shared));shared=nullptr;return false;}
 for(int i=0;i<800&&!__atomic_load_n(&shared->ready,__ATOMIC_ACQUIRE);++i){int status=0;if(waitpid(audioPid,&status,WNOHANG)==audioPid){std::fprintf(stderr,"AUDIO helper exited before ready: status=%d exit=%d signal=%d\n",status,WIFEXITED(status)?WEXITSTATUS(status):-1,WIFSIGNALED(status)?WTERMSIG(status):0);audioPid=-1;break;}usleep(10000);}
 if(__atomic_load_n(&shared->ready,__ATOMIC_ACQUIRE)!=1){std::printf("AUDIO unavailable ready=%u; startup deadline 8 seconds; continuing silently\n",__atomic_load_n(&shared->ready,__ATOMIC_ACQUIRE));cpuAudioTerminate();return false;}puts("AUDIO ready: SDL 22050 Hz mono, independent stream process");return true;
#else
 puts("AUDIO host: no Miyoo output; track lengths validated");return true;
#endif
}
void cpuAudioTerminate(){
#if defined(__linux__) || defined(__APPLE__)
 if(shared){__atomic_store_n(&shared->quit,1,__ATOMIC_RELEASE);if(audioPid>0){int status;bool done=false;for(int i=0;i<50;++i){if(waitpid(audioPid,&status,WNOHANG)==audioPid){done=true;break;}usleep(10000);}if(!done){kill(audioPid,SIGKILL);waitpid(audioPid,&status,0);}}munmap(shared,sizeof(*shared));shared=nullptr;audioPid=-1;}free(sfxTable);sfxTable=nullptr;sfxCount=0;
#endif
}
bool cpuAudioStart(unsigned t,unsigned p,unsigned i){if(i>=2||t>=256||!lengths[t]||!shared||__atomic_load_n(&shared->ready,__ATOMIC_ACQUIRE)!=1)return false;auto& s=shared->stream[i];s.track=t+1;s.seek=p%lengths[t];s.paused=0;std::printf("AUDIO START track=%u seek_ms=%u length_ms=%u stream=%u\n",t,s.seek,lengths[t],i);__atomic_add_fetch(&s.serial,1,__ATOMIC_RELEASE);return true;}
void cpuAudioStop(unsigned i){if(shared&&i<2){shared->stream[i].track=0;__atomic_add_fetch(&shared->stream[i].serial,1,__ATOMIC_RELEASE);}}
void cpuAudioPause(bool b,unsigned i){if(shared&&i<2)__atomic_store_n(&shared->stream[i].paused,uint32_t(b),__ATOMIC_RELEASE);}
void cpuAudioMenuPause(bool b){if(shared)__atomic_store_n(&shared->menuPaused,uint32_t(b),__ATOMIC_RELEASE);}
void cpuAudioVolume(unsigned v,unsigned i){if(shared&&i<2)__atomic_store_n(&shared->stream[i].volume,std::min(v,127u),__ATOMIC_RELEASE);}
void cpuAudioMaster(unsigned v,bool fade){if(shared)__atomic_store_n(fade?&shared->fade:&shared->master,std::min(v,127u),__ATOMIC_RELEASE);}
unsigned cpuAudioPosition(unsigned i){return shared&&i<2?__atomic_load_n(&shared->stream[i].position,__ATOMIC_ACQUIRE):0;}
unsigned cpuAudioLength(unsigned t){return t<256?lengths[t]:0;}
bool cpuAudioPlaying(unsigned i){return shared&&i<2&&(__atomic_load_n(&shared->stream[i].playing,__ATOMIC_ACQUIRE)||(shared->stream[i].track&&__atomic_load_n(&shared->stream[i].serial,__ATOMIC_ACQUIRE)!=__atomic_load_n(&shared->stream[i].ack,__ATOMIC_ACQUIRE)));}

unsigned cpuSfxInfo(unsigned s,unsigned field){if(!sfxTable||s>=sfxCount)return field==4?unsigned(-1):0;const auto& e=sfxTable[s];switch(field){case 0:return e.rate;case 1:return e.loopStart;case 2:return unsigned(e.loopEnd);case 3:return e.bytes/2;case 4:return 1;default:return 0;}}
bool cpuSfxAvailable(){return sfxCount!=0;}
bool cpuSfxInitChannel(unsigned c,unsigned s){if(!shared||c>=CPU_AUDIO_VOICES||s>=sfxCount)return false;auto& v=shared->voice[c];v.sample=s+1;v.rate=sfxTable[s].rate;v.volume=127;v.loops=1;v.start=sfxTable[s].loopStart/2;v.end=sfxTable[s].loopEnd<0?sfxTable[s].bytes/2:unsigned(sfxTable[s].loopEnd)/2;return true;}
void cpuSfxChannelVolume(unsigned c,unsigned n){if(shared&&c<CPU_AUDIO_VOICES)__atomic_store_n(&shared->voice[c].volume,std::min(n,127u),__ATOMIC_RELEASE);}
void cpuSfxChannelRate(unsigned c,unsigned n){if(shared&&c<CPU_AUDIO_VOICES)__atomic_store_n(&shared->voice[c].rate,std::max(1u,std::min(n,192000u)),__ATOMIC_RELEASE);}
void cpuSfxLoop(unsigned c,unsigned start,int end){if(shared&&c<CPU_AUDIO_VOICES){shared->voice[c].start=start/2;shared->voice[c].end=end<0?0:unsigned(end)/2;}}
void cpuSfxCount(unsigned c,unsigned n){if(shared&&c<CPU_AUDIO_VOICES)shared->voice[c].loops=n;}
void cpuSfxStart(unsigned c){if(shared&&c<CPU_AUDIO_VOICES&&shared->voice[c].sample){__atomic_add_fetch(&shared->voice[c].serial,1,__ATOMIC_RELEASE);static unsigned logs=0;if(logs++<20)std::printf("AUDIO SFX START channel=%u sample=%u rate=%u loops=%u\n",c,shared->voice[c].sample-1,shared->voice[c].rate,shared->voice[c].loops);}}
void cpuSfxStop(unsigned c){if(shared&&c<CPU_AUDIO_VOICES){shared->voice[c].sample=0;__atomic_add_fetch(&shared->voice[c].serial,1,__ATOMIC_RELEASE);}}
bool cpuSfxPlaying(unsigned c){if(!shared||c>=CPU_AUDIO_VOICES)return false;auto& v=shared->voice[c];return v.sample&&(__atomic_load_n(&v.playing,__ATOMIC_ACQUIRE)||__atomic_load_n(&v.serial,__ATOMIC_ACQUIRE)!=__atomic_load_n(&v.ack,__ATOMIC_ACQUIRE));}
void cpuSfxMaster(unsigned v,bool fade){if(shared)__atomic_store_n(fade?&shared->effectsFade:&shared->effects,std::min(v,127u),__ATOMIC_RELEASE);}
