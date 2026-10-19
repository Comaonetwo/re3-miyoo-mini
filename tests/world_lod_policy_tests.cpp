#include "world_lod_policy.hpp"
#include <cassert>
#include <cstdio>
using namespace soft;
int main(){ModelBounds b;b.include({-10,-10,50});b.include({10,10,60});std::vector<ModelBounds> regions={b};assert(safeChangedGeometry(regions,{0,0,0},49));assert(!safeChangedGeometry(regions,{0,0,40},11));assert(!safeChangedGeometry(regions,{0,0,55},1));assert(!safeChangedGeometry({}, {0,0,0},1));regions[0].valid=false;assert(!safeChangedGeometry(regions,{0,0,0},1));puts("PASS changed-region near/far/boundary/invalid camera guard");}
