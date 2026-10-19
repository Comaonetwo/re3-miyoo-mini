#include "soft_renderer.hpp"
#include <algorithm>
#include <cmath>
namespace soft {
void Renderer::shadedTriangle(const ShadedVertex* input,const Texture* texture,ScreenState state,const Camera& c){
    auto reject=[&](){++stats_.submitted;++stats_.rejected;};
    if(!std::isfinite(c.nearZ)||!std::isfinite(c.farZ)||!std::isfinite(c.focal)||c.nearZ<.001f||c.farZ<=c.nearZ||c.farZ>10000||c.focal<1||c.focal>10000){reject();return;}
    ShadedVertex a[16],b[16];int n=3;
    for(int i=0;i<3;++i){auto v=input[i];
        for(float f:{v.position.x,v.position.y,v.position.z,v.uv.u,v.uv.v,v.r,v.g,v.b,v.a})if(!std::isfinite(f)){reject();return;}
        if(std::fabs(v.position.x)>1e6f||std::fabs(v.position.y)>1e6f||std::fabs(v.position.z)>1e6f||std::fabs(v.uv.u)>1e5f||std::fabs(v.uv.v)>1e5f||v.r<0||v.r>255||v.g<0||v.g>255||v.b<0||v.b>255||v.a<0||v.a>255){reject();return;}
        a[i]=v;
    }
    float sx=160/c.focal,sy=120/c.focal;
    unsigned combined=0,common=63;for(int i=0;i<3;++i){unsigned code=frustumCode(input[i].position,c);combined|=code;common&=code;}
    if(common){reject();return;}
    for(int plane=0;plane<6&&n&&combined;++plane){
        auto distance=[&](Vec3 p){switch(plane){case 0:return p.z-c.nearZ;case 1:return c.farZ-p.z;case 2:return p.x+sx*p.z;case 3:return sx*p.z-p.x;case 4:return p.y+sy*p.z;default:return sy*p.z-p.y;}};
        auto prev=a[n-1];float dp=distance(prev.position);int m=0;
        for(int i=0;i<n;++i){auto cur=a[i];float dc=distance(cur.position);
            if((dp>=0)!=(dc>=0)){float t=dp/(dp-dc);auto mix=[&](float x,float y){return x+t*(y-x);};
                b[m++]={{mix(prev.position.x,cur.position.x),mix(prev.position.y,cur.position.y),mix(prev.position.z,cur.position.z)},
                        {mix(prev.uv.u,cur.uv.u),mix(prev.uv.v,cur.uv.v)},mix(prev.r,cur.r),mix(prev.g,cur.g),mix(prev.b,cur.b),mix(prev.a,cur.a)};}
            if(dc>=0)b[m++]=cur;prev=cur;dp=dc;
        }
        n=m;std::copy(b,b+n,a);
    }
    if(n<3){reject();return;}
    state.perspectiveColor=true;
    auto screen=[&](ShadedVertex v){float iz=1/v.position.z;return ScreenVertex{160+c.focal*v.position.x*iz,120-c.focal*v.position.y*iz,iz,v.uv.u,v.uv.v,v.r,v.g,v.b,v.a};};
    for(int i=1;i<n-1;++i){ScreenVertex v[3]={screen(a[0]),screen(a[i]),screen(a[i+1])};screenTriangle(v,texture,state);}
}
}
