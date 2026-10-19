#include "loading_watch.hpp"
#include <cassert>
int main(){soft::LoadingWatch w;w.observe(1,0);assert(!w.expired(120000));assert(w.expired(120001));w.observe(7,130000);assert(!w.expired(36000000));w.observe(6,36000000);assert(!w.expired(36000001));assert(!w.expired(36120000));assert(w.expired(36120001));w.observe(8,36120002);assert(!w.expired(40000000));}
