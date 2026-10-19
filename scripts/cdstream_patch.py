"""CPU streamer synchronization patch, shared by preparation and real-API stress checks."""
def wrap(source,signature,body):
 start=source.index(signature+'\n{');brace=source.index('{',start)
 # Pinned POSIX functions close in column zero. Counting raw braces would
 # misread the alternative ONE_THREAD_PER_CHANNEL preprocessor branches.
 end=source.index('\n}',brace)+2
 return source[:brace+1]+'\n#ifdef MIYOO_CPU\n'+body+'\n#else'+source[brace+1:end-1]+'\n#endif\n'+source[end-1:]
def patch_cpu_sync(s):
 anchor='void *CdStreamThread(void* channelId);';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'\n#ifdef MIYOO_CPU\n#include "cdstream_cpu.inc"\n#endif')
 anchor='CdStreamInitThread(void)\n{';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'\n#ifdef MIYOO_CPU\n\tcpuCdReset();\n#endif')
 for signature,body in [('CdStreamRead(int32 channel, void *buffer, uint32 offset, uint32 size)','\treturn cpuCdRead(channel,buffer,offset,size);'),('CdStreamGetStatus(int32 channel)','\treturn cpuCdStatus(channel);'),('CdStreamSync(int32 channel)','\treturn cpuCdSync(channel);'),('CdStreamShutdown(void)','\tcpuCdShutdown();'),('CdStreamThread(void *param)','\treturn cpuCdWorker();')]:s=wrap(s,signature,body)
 s=s.replace('if (status == -1)','if (status != 0)')
 return s

def patch_cdstream(s,replace):
 s=replace(s, '#include <stdarg.h>', '#include <stdarg.h>\n#include <stdexcept>\n#include <errno.h>')
 s=replace(s, '\tsem_t* sem = (sem_t*)malloc(sizeof(sem_t));\n\tif (sem_init(sem, 0, 1) == -1) {\n\t\tsem = SEM_FAILED;\n\t}', '\tsem_t* sem = (sem_t*)malloc(sizeof(sem_t));\n\tif (!sem) throw std::runtime_error(\"Cannot allocate streaming semaphore\");\n\tif (sem_init(sem, 0, 1) == -1) {\n\t\tconst int code = errno;\n\t\tfree(sem);\n\t\tthrow std::runtime_error(std::string(\"Cannot initialize local streaming semaphore: \") + strerror(code));\n\t}')
 s=replace(s, '\tint status;\n#ifndef ONE_THREAD_PER_CHANNEL', '\tint status;\n#if defined(MIYOO_CPU) && defined(USE_UNNAMED_SEM)\n\tprintf(\"CPU streaming sync: process-local unnamed semaphores\\n\");\n#endif\n#ifndef ONE_THREAD_PER_CHANNEL')
 old="\tchar semName[21];\n\tva_list va;\n\tva_start(va, format);\n\tvsprintf(semName, format, va);"
 new="\tchar base[21], semName[32];\n\tva_list va;\n\tva_start(va, format);\n\tvsnprintf(base, sizeof(base), format, va);\n\tva_end(va);\n\tsnprintf(semName, sizeof(semName), \"/r3_%ld_%.15s\", (long)getpid(), base + 1);"
 if s.count(old)!=2:raise ValueError('cdstream semaphore anchors')
 s=s.replace(old,new)
 s=replace(s,'return sem_open(semName, O_CREAT, 0644, 1);','sem_t* sem = sem_open(semName, O_CREAT | O_EXCL, 0644, 1);\n\tif(sem == SEM_FAILED) throw std::runtime_error(std::string(\"Cannot open host streaming semaphore: \") + strerror(errno));\n\tsem_unlink(semName);\n\treturn sem;')
 return patch_cpu_sync(s)
