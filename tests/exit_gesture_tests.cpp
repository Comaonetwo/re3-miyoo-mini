#include "exit_gesture.hpp"
#include <initializer_list>
#include <cstdio>
using soft::ExitRequest;
struct Event {unsigned code;int value;};
static ExitRequest sequence(std::initializer_list<Event> events){
    bool menu=false,select=false;
    for(auto e:events){if(e.code==1)menu=e.value!=0;if(e.code==97)select=e.value!=0;
        auto exit=soft::exitGesture(e.code,e.value,menu,select);if(exit!=ExitRequest::None)return exit;}
    return ExitRequest::None;
}
int main(){
    bool ok=sequence({{1,1},{97,1}})==ExitRequest::MainMenu&&
        sequence({{97,1},{1,1}})==ExitRequest::MainMenu&&
        sequence({{97,1},{97,2}})==ExitRequest::None&&
        sequence({{97,1},{97,0}})==ExitRequest::None&&
        sequence({{97,1},{97,0},{1,1},{1,0}})==ExitRequest::None&&
        sequence({{1,1},{1,2},{1,0}})==ExitRequest::None&&
        sequence({{28,1},{28,0}})==ExitRequest::None;
    std::puts(ok?"exit gestures: both chord orders / solo Select ignored / repeats PASS":"exit gestures FAIL");return ok?0:1;
}
