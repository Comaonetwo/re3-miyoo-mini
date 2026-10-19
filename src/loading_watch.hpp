#pragma once
#include <cstdint>
namespace soft {
struct LoadingWatch {
 unsigned stage=0;uint64_t since=0;
 void observe(unsigned value,uint64_t now){if(value!=stage||now<since){stage=value;since=now;}}
 bool expired(uint64_t now)const{return stage<7&&now>=since&&now-since>120000;}
};
}
