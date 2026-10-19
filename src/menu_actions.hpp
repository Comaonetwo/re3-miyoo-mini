#pragma once
namespace soft {
enum class MenuAction {None,Lighting,Paint,Quality};
// Hold Menu before Y/B. Solo Menu acts on release, avoiding accidental actions
// when used for the exit chord. Select remains reserved for Menu+Select only.
class MenuActions {
    bool used_=false;
public:
    MenuAction event(unsigned code,int value,bool menu,bool select){
        if(code==1&&value==1)used_=select;
        if(code==1&&value==0){bool solo=!used_&&!select;used_=false;return solo?MenuAction::Lighting:MenuAction::None;}
        if(value==1&&menu&&code!=1){used_=true;if(code==56)return MenuAction::Paint;if(code==29)return MenuAction::Quality;}
        return MenuAction::None;
    }
    void reset(){used_=true;}
};
}
