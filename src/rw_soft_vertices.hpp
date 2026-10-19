#pragma once
// This header supplies the actual vertex API used by re3/fakerw with RW_NULL.
// Include it before rwcore.h; no upstream files or backend macros are modified.
#ifndef RW_NULL
#error Software vertex compatibility requires RW_NULL
#endif
#ifndef RWDEVICE
#define RWDEVICE cpu
#endif
namespace rw {
// Windowless CPU device: no platform window handles are needed.
struct EngineOpenParams {};
namespace cpu {
struct Im2DVertex {
    float x=0,y=0,z=0,reciprocalZ=1,u=0,v=0;uint8 r=255,g=255,b=255,a=255;
    void setScreenX(float n){x=n;}void setScreenY(float n){y=n;}void setScreenZ(float n){z=n;}
    void setCameraZ(float n){reciprocalZ=n!=0?1/n:0;}void setRecipCameraZ(float n){reciprocalZ=n;}
    void setU(float n,float){u=n;}void setV(float n,float){v=n;}
    void setColor(uint8 red,uint8 green,uint8 blue,uint8 alpha){r=red;g=green;b=blue;a=alpha;}
    float getScreenX(){return x;}float getScreenY(){return y;}float getScreenZ(){return z;}
    float getCameraZ(){return reciprocalZ!=0?1/reciprocalZ:0;}float getRecipCameraZ(){return reciprocalZ;}
    float getU(){return u;}float getV(){return v;}RGBA getColor(){return {r,g,b,a};}
};
struct Im3DVertex {
    V3d position{0,0,0};float u=0,v=0;uint8 r=255,g=255,b=255,a=255;
    void setX(float n){position.x=n;}void setY(float n){position.y=n;}void setZ(float n){position.z=n;}
    void setU(float n){u=n;}void setV(float n){v=n;}
    void setColor(uint8 red,uint8 green,uint8 blue,uint8 alpha){r=red;g=green;b=blue;a=alpha;}
    float getX(){return position.x;}float getY(){return position.y;}float getZ(){return position.z;}
    float getU(){return u;}float getV(){return v;}RGBA getColor(){return {r,g,b,a};}
};
}}
