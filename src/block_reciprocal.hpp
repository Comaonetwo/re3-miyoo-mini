#pragma once
#include <cmath>
#include <cstdint>
namespace soft {
// Experimental: retain the original attribute recurrences and depth tests.
// Refresh a true reciprocal every eight visible candidates or after a relative
// depth change of 1/128. A second-order expansion bounds ideal relative error
// by |d|^3; float rounding adds a small residual. Opaque texture alpha only.
// This is NOT an exact RGB certificate: nearest texel boundaries may differ.
struct BlockReciprocal {
 float origin=0,seed=0;unsigned age=8;uint32_t reused=0,seeds=0;
 inline float get(float z){
  float d=(z-origin)*seed;
  if(age>=8||!(z>1e-8f)||!std::isfinite(d)||std::fabs(d)>1.f/128){origin=z;seed=1/z;age=1;++seeds;return seed;}
  ++age;++reused;return seed*(1-d*(1-d));
 }
};
}
