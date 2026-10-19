#include "sampler_policy.hpp"
#include <cassert>
int main(){double a[3]={40,41,42},b[3]={35,36,37};assert(soft::selectPacketSampler(a,b));double slow[3]={41,42,43};assert(!soft::selectPacketSampler(a,slow));double noise[3]={30,41,37};assert(!soft::selectPacketSampler(a,noise));double tiny[3]={39.5,40.5,41.5};assert(!soft::selectPacketSampler(a,tiny));double bad[3]={0,36,37};assert(!soft::selectPacketSampler(a,bad));}
