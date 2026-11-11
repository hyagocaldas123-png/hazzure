#pragma once
#include "GameData.h"
#include <random>
namespace HazzureCombatRefine {
struct KillNotice {uintptr_t pawn{};uint32_t handle{};uint64_t epoch{},time{};uint32_t victim{};};
inline SRWLOCK killLock=SRWLOCK_INIT;inline KillNotice lastKill;
inline void ConfirmKill(KillNotice notice){AcquireSRWLockExclusive(&killLock);
    bool duplicate=notice.victim&&notice.victim==lastKill.victim&&notice.pawn==lastKill.pawn&&notice.handle==lastKill.handle&&
        notice.epoch==lastKill.epoch&&notice.time>=lastKill.time&&notice.time-lastKill.time<500;
    if(!duplicate)lastKill=notice;ReleaseSRWLockExclusive(&killLock);}
inline KillNotice ReadKill(){AcquireSRWLockShared(&killLock);auto out=lastKill;ReleaseSRWLockShared(&killLock);return out;}
enum class AimWait {None,Kill,Switch};
struct AimDelay {
    uintptr_t pawn{},weapon{},target{};uint32_t handle{};uint64_t epoch{},since{},last{};AimWait wait{};
    void Reset(){*this={};}
    bool Ready(bool active,uintptr_t owner,uint32_t serial,uintptr_t gun,uintptr_t candidate,uint64_t session,
               uint64_t now,const KillNotice& kill,uint32_t killMs,uint32_t switchMs){
        if(!active||!owner||!serial||!gun||!candidate||killMs>1000||switchMs>1000){
            if(!active||!owner||!serial||!gun)Reset();return false;
        }
        if(pawn!=owner||handle!=serial||weapon!=gun||epoch!=session||(last&&now<last)){
            Reset();pawn=owner;handle=serial;weapon=gun;epoch=session;
        }
        wait=AimWait::None;last=now;
        if(target!=candidate){bool switching=target!=0;target=candidate;since=switching?now:0;}
        if(killMs&&kill.pawn==owner&&kill.handle==serial&&kill.epoch==session&&kill.time&&
           now>=kill.time&&now-kill.time<killMs){wait=AimWait::Kill;return false;}
        if(since&&now-since<switchMs){wait=AimWait::Switch;return false;}
        return true;
    }
};
struct DelayWindow {uint32_t minimum{},maximum{};};
enum class TriggerMode {Balanced,Distance,Manual};
inline TriggerMode TriggerDelayMode(bool variable,bool distance){
    return !variable?TriggerMode::Manual:(distance?TriggerMode::Distance:TriggerMode::Balanced);
}
inline constexpr DelayWindow BalancedWindow(){return {40,90};}
inline DelayWindow ReactionWindow(float metres){
    if(!std::isfinite(metres)||metres<0)return {};
    const float t=std::clamp(metres/30.f,0.f,1.f);
    return {static_cast<uint32_t>(std::lround(60.f+90.f*t)),static_cast<uint32_t>(std::lround(90.f+130.f*t))};
}
struct TriggerTiming {
    uintptr_t target{},weapon{};TriggerMode mode{};DelayWindow window{};
    uint64_t since{};uint32_t delay{};
    void Reset(){target=weapon=0;since=0;delay=0;window={};}
    template<class Random> bool Ready(uintptr_t current,uintptr_t activeWeapon,TriggerMode selected,
                                     uint64_t now,DelayWindow range,Random& random){
        if(!current||!activeWeapon||range.minimum>range.maximum||range.maximum>1000){Reset();return false;}
        // A pending reaction belongs to this target, weapon and selected mode.
        // Sample once per acquisition, never once per render frame.
        bool manualChanged=selected==TriggerMode::Manual&&
            (window.minimum!=range.minimum||window.maximum!=range.maximum);
        if(target!=current||weapon!=activeWeapon||mode!=selected||now<since||manualChanged){
            target=current;weapon=activeWeapon;mode=selected;window=range;since=now;
            delay=std::uniform_int_distribution<uint32_t>(range.minimum,range.maximum)(random);
        }
        return now>=since&&now-since>=delay;
    }
};
inline float Health(float previous,int current,float dt){
    const float value=static_cast<float>(std::clamp(current,0,100));
    if(!std::isfinite(previous)||previous<0||previous>100||!std::isfinite(dt)||dt<0||dt>.25f)return value;
    // Healing snaps to the new level; damage drains without overshoot.
    if(value>=previous)return value;
    return value+(previous-value)*std::exp(-14.f*dt);
}
struct Colour {int red{},green{},blue{};};
inline Colour HealthColour(float health){
    if(!std::isfinite(health))health=0;
    float t=std::clamp(health/100.f,0.f,1.f);
    return t<.5f?Colour{240,static_cast<int>(70+340*t),65}:Colour{static_cast<int>(240-320*(t-.5f)),240,65};
}
} // namespace HazzureCombatRefine
