// Stable appearance identity and a bounded refresh burst after native model rebuilds.
#pragma once
#include <cstdint>
namespace HazzureGloveLifecycle {
struct Appearance {
    uintptr_t pawn{},scene{},hud{},hudScene{},arms{},armsScene{};uint32_t handle{},hudHandle{},armsHandle{},spawn{};uint8_t team{};
    uint64_t model{},armsModel{},meshMask{},agentEpoch{};
};
inline bool Valid(const Appearance& a){return a.pawn&&a.scene&&a.handle&&a.handle!=0xffffffffu&&a.model&&(a.team==2||a.team==3);}
inline bool Same(const Appearance& a,const Appearance& b){
    return a.pawn==b.pawn&&a.scene==b.scene&&a.hud==b.hud&&a.hudScene==b.hudScene&&a.handle==b.handle&&
        a.hudHandle==b.hudHandle&&a.spawn==b.spawn&&a.team==b.team&&a.model==b.model&&a.meshMask==b.meshMask&&a.agentEpoch==b.agentEpoch&&
        a.arms==b.arms&&a.armsScene==b.armsScene&&a.armsHandle==b.armsHandle&&a.armsModel==b.armsModel;
}
inline uint64_t ModelHash(const char* text){
    if(!text||!*text)return 0;uint64_t hash=14695981039346656037ull;
    for(unsigned n=0;n<240;++n){auto c=static_cast<unsigned char>(text[n]);if(!c)return hash;hash=(hash^c)*1099511628211ull;}
    return 0;
}
struct Refresh {
    Appearance appearance{};bool available{};unsigned remaining{};uint64_t lastNotification{};
    void Reset(){*this={};}
    void Require(){remaining=4;lastNotification=0;}
    bool Observe(const Appearance& value){
        if(!Valid(value)){available=false;return false;}
        bool changed=!available||!Same(appearance,value);appearance=value;available=true;
        if(changed)Require();return changed;
    }
    bool Due(uint64_t now)const{return available&&remaining&&(!lastNotification||now>lastNotification);}
    void Acknowledge(uint64_t now){if(remaining)--remaining;lastNotification=now;}
};
}
