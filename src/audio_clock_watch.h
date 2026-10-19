#pragma once
#include <stdint.h>
typedef struct {uint64_t time,frames,lastRecovery;unsigned bad,recoveries;} AudioClockWatch;
static void audioClockReset(AudioClockWatch* w,uint64_t now,uint64_t frames){w->time=now;w->frames=frames;w->bad=0;}
/* Callback cadence measures device delivery, not ring underruns. A sustained
   deviation triggers a bounded reopen; no rate changes or pitch correction. */
static int audioClockObserve(AudioClockWatch* w,uint64_t now,uint64_t frames,unsigned rate,unsigned intervalMs,double* measured){
 if(now<w->time){w->lastRecovery=now;audioClockReset(w,now,frames);return 0;}
 if(frames<w->frames){audioClockReset(w,now,frames);return 0;}
 uint64_t elapsed=now-w->time;if(elapsed<(uint64_t)intervalMs*1000)return 0;
 double actual=(frames-w->frames)*1000000.0/elapsed;if(measured)*measured=actual;w->time=now;w->frames=frames;
 if(actual<rate*.85||actual>rate*1.15)++w->bad;else w->bad=0;
 if(w->bad<2|| (w->recoveries&&now-w->lastRecovery<30000000))return 0;
 w->bad=0;w->lastRecovery=now;++w->recoveries;return 1;
}
