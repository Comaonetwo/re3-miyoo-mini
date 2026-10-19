#pragma once
namespace soft {
enum class ExitRequest {None,PortList,MainMenu};
// Miyoo evdev: Menu=1, Select=97. Only the chord exits, in either press
// order. Solo presses, releases and autorepeat are ignored.
inline ExitRequest exitGesture(unsigned code,int value,bool menuHeld,bool selectHeld) {
    if(value==1&&(code==1||code==97)&&menuHeld&&selectHeld)return ExitRequest::MainMenu;
    return ExitRequest::None;
}
constexpr int mainMenuExitCode=42; // Launcher translates this request to successful exit.
}
