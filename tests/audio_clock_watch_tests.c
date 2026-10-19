#include "audio_clock_watch.h"
#include <assert.h>
#include <stdio.h>
int main(void){AudioClockWatch w={0};audioClockReset(&w,1000000,0);double rate;
 assert(!audioClockObserve(&w,6000000,110250,22050,5000,&rate));assert(rate==22050);
 assert(!audioClockObserve(&w,11000000,165375,22050,5000,&rate));assert(rate==11025);
 assert(audioClockObserve(&w,16000000,220500,22050,5000,&rate));assert(w.recoveries==1);
 audioClockReset(&w,16000000,220500);
 assert(!audioClockObserve(&w,21000000,330750,22050,5000,&rate));assert(!w.bad);
 assert(!audioClockObserve(&w,26000000,385875,22050,5000,&rate));assert(!audioClockObserve(&w,31000000,441000,22050,5000,&rate));
 assert(!audioClockObserve(&w,1000000,0,22050,5000,&rate));assert(!w.bad);
 puts("PASS cadence watchdog: sustained half-rate, recovery, normal clock, cooldown, counter/clock discontinuity");
}
