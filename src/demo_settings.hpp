#pragma once
#include "rw_asset_scene.hpp"
#include <algorithm>
namespace soft {
struct DemoSettings {
    bool night=false,quality=true;unsigned paintIndex=0;
    void apply(SoftwareDevice& device,RwAssetScene* car) const{
        device.highQuality(quality);
        device.lighting(night?Vec3{.035f,.045f,.08f}:Vec3{.25f,.26f,.30f},night?Vec3{0,0,0}:Vec3{.75f,.70f,.60f},{-.35f,-.85f,.39f});
        static const uint32_t palette[]={0x2d69be,0xb52f29,0xa7adb2,0xd5b02c,0x327947};
        if(car)car->paint(palette[paintIndex%5],palette[paintIndex%5]);
    }
    void hud(std::vector<uint16_t>& frame,bool driving=true) const{
        std::fill(frame.end()-320*23,frame.end(),rgb(15,20,28));
        drawText(frame,4,220,driving?"D-PAD DRIVE L BRAKE R FREE M-Y PAINT":"D-PAD MOVE L UP R DRIVE M-Y PAINT");
        drawText(frame,4,230,"A RESET Y TEX B DIST M LIGHT M-B Q M-S EXIT");
        drawText(frame,268,14,quality?"RW HI":"RW LO");drawText(frame,224,14,night?"NIGHT":"DAY");
    }
    uint16_t sky() const{return night?rgb(18,24,42):rgb(114,157,187);}
};
}
