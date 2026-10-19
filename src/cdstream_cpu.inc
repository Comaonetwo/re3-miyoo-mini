// Included by the prepared POSIX streamer after its channel/queue declarations.
// Metadata, queue ownership and the completion predicate share one mutex.
// Disk IO stays outside it; waking is conditional on state, not a semaphore token.
#include <mutex>
#include <condition_variable>
#ifdef ONE_THREAD_PER_CHANNEL
#error The CPU streamer uses the bounded single-worker queue
#endif
static std::mutex cpuCdMutex;
static std::condition_variable cpuCdDone;
static bool cpuCdFailed=false;
static void cpuCdReset(){std::lock_guard<std::mutex> lock(cpuCdMutex);cpuCdFailed=false;}
static void cpuCdCheck(){if(cpuCdFailed)throw std::runtime_error("CPU streaming worker failed");}
static int32 cpuCdSync(int32 channel){
    ASSERT(channel>=0&&channel<gNumChannels);
    std::unique_lock<std::mutex> lock(cpuCdMutex);cpuCdCheck();
    if(gCdStreamThreadStatus==2||!gpReadInfo)return STREAM_NONE;
    auto p=&gpReadInfo[channel];
    while(gCdStreamThreadStatus!=2&&(p->nSectorsToRead!=0||p->bReading))cpuCdDone.wait(lock);
    cpuCdCheck();if(gCdStreamThreadStatus==2)return STREAM_NONE;
#ifdef FLUSHABLE_STREAMING
    if(flushStream[channel]){
        // Finish the active bounded read before its buffer is reused. Do not
        // race a SIGUSR1/cancellation flag against the IO/completion transition.
        flushStream[channel]=false;p->nStatus=STREAM_NONE;
    }
#endif
    return p->nStatus;
}
static int32 cpuCdRead(int32 channel,void* buffer,uint32 offset,uint32 size){
    ASSERT(channel>=0&&channel<gNumChannels);ASSERT(buffer!=nil);
    std::unique_lock<std::mutex> lock(cpuCdMutex);cpuCdCheck();
    if(gCdStreamThreadStatus==2||!gpReadInfo)return STREAM_NONE;
    lastPosnRead=size+offset;ASSERT(_GET_INDEX(offset)<MAX_CDIMAGES);
    int32 image=gImgFiles[_GET_INDEX(offset)];ASSERT(image>0);
    auto p=&gpReadInfo[channel];if(size==0)return STREAM_SUCCESS;
    if(p->nSectorsToRead!=0||p->bReading){
        if(p->hFile==image-1&&p->nSectorOffset==_GET_OFFSET(offset)&&p->nSectorsToRead>=size)return STREAM_SUCCESS;
#ifdef FLUSHABLE_STREAMING
        flushStream[channel]=true;lock.unlock();cpuCdSync(channel);lock.lock();cpuCdCheck();
        if(gCdStreamThreadStatus==2||!gpReadInfo)return STREAM_NONE;
#else
        return STREAM_NONE;
#endif
    }
    p->hFile=image-1;p->nStatus=STREAM_NONE;p->nSectorOffset=_GET_OFFSET(offset);
    p->nSectorsToRead=size;p->pBuffer=buffer;p->bLocked=false;
    AddToQueue(&gChannelRequestQ,channel);lock.unlock();
    if(sem_post(gCdStreamSema)!=0)throw std::runtime_error("Cannot signal CPU streaming worker");
    return STREAM_SUCCESS;
}
static int32 cpuCdStatus(int32 channel){
    ASSERT(channel>=0&&channel<gNumChannels);
    std::lock_guard<std::mutex> lock(cpuCdMutex);cpuCdCheck();
    if(gCdStreamThreadStatus==2||!gpReadInfo)return STREAM_NONE;
    auto p=&gpReadInfo[channel];if(p->bReading)return STREAM_READING;
    if(p->nSectorsToRead!=0)return STREAM_WAITING;
    int32 status=p->nStatus;if(status!=STREAM_NONE)p->nStatus=STREAM_NONE;return status;
}
static void cpuCdShutdown(){
    {std::lock_guard<std::mutex> lock(cpuCdMutex);gCdStreamThreadStatus=2;cpuCdDone.notify_all();}
    sem_post(gCdStreamSema);pthread_join(_gCdStreamThread,nil);
}
static void* cpuCdWorker(){
    printf("CPU CDSTREAM: locked completion, flush drains active read\n");
    for(;;){
        int rc;do{rc=sem_wait(gCdStreamSema);}while(rc==-1&&errno==EINTR);
        std::unique_lock<std::mutex> lock(cpuCdMutex);
        if(rc!=0){cpuCdFailed=true;gCdStreamThreadStatus=2;cpuCdDone.notify_all();break;}
        if(gCdStreamThreadStatus==2)break;
        int32 channel=GetFirstInQueue(&gChannelRequestQ);if(channel==-1)continue;
        auto p=&gpReadInfo[channel];
        if(p->nSectorsToRead==0){RemoveFirstInQueue(&gChannelRequestQ);cpuCdDone.notify_all();continue;}
        p->bReading=true;
        if(gCdStreamThreadStatus==0){gCdStreamThreadStatus=1;
#ifdef __linux__
            pid_t tid=syscall(SYS_gettid);setpriority(PRIO_PROCESS,tid,getpriority(PRIO_PROCESS,getpid())+1);
#endif
        }
        const int fd=p->hFile;const uint32 sectors=p->nSectorsToRead,sector=p->nSectorOffset;
        void* buffer=p->pBuffer;int32 status=p->nStatus;lock.unlock();
        if(status==STREAM_NONE){
            const size_t bytes=size_t(sectors)*CDSTREAM_SECTOR_SIZE;size_t done=0;
            if(bytes/CDSTREAM_SECTOR_SIZE!=sectors||lseek(fd,off_t(sector)*CDSTREAM_SECTOR_SIZE,SEEK_SET)==-1)status=STREAM_ERROR;
            while(status==STREAM_NONE&&done<bytes){
                ssize_t got=read(fd,static_cast<char*>(buffer)+done,bytes-done);
                if(got>0)done+=size_t(got);else if(got<0&&errno==EINTR)continue;else status=STREAM_ERROR;
            }
        }
        lock.lock();p->nStatus=status;RemoveFirstInQueue(&gChannelRequestQ);
        p->nSectorsToRead=0;p->bReading=false;p->bLocked=false;cpuCdDone.notify_all();
    }
    // All API users observe the stopped state under the same mutex before
    // touching the channel table. Shutdown joins this worker before re-init.
    std::lock_guard<std::mutex> lock(cpuCdMutex);
    for(int32 i=0;i<gNumChannels;++i)RE3_SEM_CLOSE(gpReadInfo[i].pDoneSemaphore,"/semaphore_done%d",i);
    RE3_SEM_CLOSE(gCdStreamSema,"/semaphore_cd_stream");free(gChannelRequestQ.items);gChannelRequestQ.items=nil;
    free(gpReadInfo);gpReadInfo=nil;return nil;
}
