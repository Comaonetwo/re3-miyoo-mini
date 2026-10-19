#include "soft_renderer.hpp"
#include "world_channels.hpp"
#include <cassert>
#include <cstdio>
using namespace soft;
int main(){unsigned cases=0;
 for(unsigned scale:{1u,2u,3u,4u})for(unsigned pattern=0;pattern<4;++pattern)for(bool linear:{false,true})for(unsigned blend:{0u,1u,2u,3u}){
  Texture t;t.width=t.height=8;t.decodedUniform=pattern<2;t.decodedOpaque=pattern%2==0;
  for(unsigned i=0;i<64;++i){t.pixels.push_back(t.decodedUniform?65535:uint16_t(i*7919));t.alpha.push_back(t.decodedOpaque?255:uint8_t(i*37));}
  Renderer fast,slow;slow.worldChannelsEnabled(false);for(auto r:{&fast,&slow}){r->renderScale(scale);r->clear(17291);r->alphaSpansEnabled(true);r->filterStages(31);}
  for(unsigned layer=0;layer<4;++layer){ScreenVertex v[3]={{2.3f,4.7f,.1f,-.173f,.19f,13,233,71,255,0},{312.3f,29.7f,.08f,1.73f,.19f,249,7,253,255,0},{27.3f,233.7f,.15f,-.173f,1.61f,89,171,3,255,0}};
   ScreenState s;s.linear=linear;s.blend=blend;s.zTest=true;s.zWrite=layer%2;s.screenDepth=layer>=2;s.perspectiveColor=layer<2;s.alphaRef=layer==1?120:0;s.addressU=s.addressV=layer+1;
   if(s.screenDepth)for(auto& q:v){q.reciprocalZ=1;q.depthZ=.5f;q.r=117;q.g=209;q.b=31;q.a=129;}
   fast.screenTriangle(v,&t,s);slow.screenTriangle(v,&t,s);
   assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());assert(fast.stats().pixels==slow.stats().pixels);
  }++cases;
 }
 uint64_t grouped=0;unsigned skyCases=0;
 for(unsigned scale:{1u,2u,3u,4u})for(bool opaque:{false,true})for(unsigned blend:{0u,1u,2u,3u})for(unsigned address:{1u,2u,3u,4u}){
  Texture t;t.width=t.height=8;t.decodedUniform=true;t.decodedOpaque=opaque;t.pixels.assign(64,65535);for(unsigned i=0;i<64;++i)t.alpha.push_back(opaque?255:uint8_t(i*37));
  Renderer fast,slow;fast.skyChannelsEnabled(true);slow.skyChannelsEnabled(false);for(auto r:{&fast,&slow}){r->renderScale(scale);r->clear(17291);r->filterStages(31);}
  for(unsigned layer=0;layer<6;++layer){ScreenVertex v[3]={{-2.3f,4.7f,1,-.173f,.19f,13,233,71,255,0},{332.3f,29.7f,1,1.73f,.19f,249,7,253,13,0},{27.3f,243.7f,1,-.173f,1.61f,89,171,3,127,0}};
   ScreenState state;state.linear=true;state.blend=blend;state.zTest=state.zWrite=false;state.screenDepth=layer==2;state.perspectiveColor=false;state.alphaRef=layer==1?120:0;state.addressU=state.addressV=address;
   fast.screenTriangle(v,layer>=3?nullptr:&t,state);slow.screenTriangle(v,layer>=3?nullptr:&t,state);assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());assert(fast.stats().pixels==slow.stats().pixels);++skyCases;
  }grouped+=fast.stats().skyColorPixels;
 }
 assert(!pixelDiagnosticsEnabled()||grouped>0||!worldChannelsAvailable);std::printf("PASS %u sky gradient cases exact RGB/depth/fragments, grouped=%llu\n",skyCases,(unsigned long long)grouped);
 std::printf("PASS %u world shader cases: exact RGB/depth/fragments vs old arithmetic, gradients, opaque/masked/uniform textures, filter/address/blend/depth states, four resolutions\n",cases);
}
