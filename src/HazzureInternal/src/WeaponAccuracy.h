// Read-only native getter dispatch on the existing frame callback, never from Present.
// Installed-PE inspection: work/combat-completion-20261026/accuracy-native*.txt.
#pragma once
#include "Visibility.h"
#include <atomic>
namespace HazzureWeaponAccuracy {
struct Snapshot {
    uintptr_t pawn{},weapon{};uint32_t handle{};uint64_t time{};
    float inaccuracy{},spread{},maxSpeed{},range{};int mode{},bullets{};bool valid{};
};
inline SRWLOCK lock=SRWLOCK_INIT;
inline Snapshot snapshot;
inline std::atomic<bool> enabled{};
inline void Publish(const Snapshot& value){AcquireSRWLockExclusive(&lock);snapshot=value;ReleaseSRWLockExclusive(&lock);}
inline void Clear(){Publish({});}
inline bool Fresh(const Snapshot& v,uintptr_t pawn,uintptr_t weapon,uint32_t handle,uint64_t now){
    return v.valid&&pawn&&weapon&&v.pawn==pawn&&v.weapon==weapon&&v.handle==handle&&v.time&&now>=v.time&&now-v.time<=32;
}
inline Snapshot Read(uintptr_t pawn,uintptr_t weapon,uint32_t handle,uint64_t now){
    AcquireSRWLockShared(&lock);auto out=snapshot;ReleaseSRWLockShared(&lock);
    return Fresh(out,pawn,weapon,handle,now)?out:Snapshot{};
}
inline bool Values(const Snapshot& v){
    return v.mode>=0&&v.mode<=1&&v.bullets==1&&std::isfinite(v.inaccuracy)&&v.inaccuracy>=0&&v.inaccuracy<=1&&
        std::isfinite(v.spread)&&v.spread>=0&&v.spread<=1&&std::isfinite(v.maxSpeed)&&v.maxSpeed>=1&&v.maxSpeed<=350&&
        std::isfinite(v.range)&&v.range>0&&v.range<=32768;
}
inline void Frame(uintptr_t client){
#ifdef INTERNAL_SELF_TEST
    (void)client;return;
#else
    using namespace HazzureGame;
    if(!enabled.load()||!HazzureVisibility::BuildMatches(client)){Clear();return;}
    Snapshot next{};next.pawn=LocalPawn(client);Pawn local{};uintptr_t list{},services{},table{},getter{},spreadGetter{},vdata{};
    if(!ReadPawn(next.pawn,local)||!HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)||!list||
       !HazzureGame::Read(next.pawn+HazzureOffsets::Schema::m_pWeaponServices,services)||!services||
       !HazzureGame::Read(services+HazzureOffsets::Schema::m_hActiveWeapon,next.handle)||
       !(next.weapon=Entity(list,next.handle))||!HazzureGame::Read(next.weapon,table)||!table||
       !HazzureGame::Read(table+0xcf8,getter)||getter!=client+0x80cfd0||!HazzureGame::Read(table+0xbd0,spreadGetter)||spreadGetter!=client+0x80e2c0||
       !HazzureGame::Read(next.weapon+0x388,vdata)||!vdata||!HazzureGame::Read(next.weapon+0x1a00,next.mode)||next.mode<0||next.mode>1){Clear();return;}
    constexpr unsigned char inaccuracyCode[]={0x48,0x89,0x5c,0x24,0x10,0x55,0x56,0x57,0x48,0x81,0xec,0xb0,0,0,0,0x44};
    constexpr unsigned char spreadCode[]={0x48,0x63,0x91,0,0x1a,0,0,0x48,0x8b,0x81,0x88,0x03,0,0,0x85,0xd2};
    if(!HazzureVisibility::Prefix(getter,inaccuracyCode,sizeof(inaccuracyCode))||!HazzureVisibility::Prefix(spreadGetter,spreadCode,sizeof(spreadCode))||
       !HazzureGame::Read(vdata+0x748+next.mode*4,next.maxSpeed)||!HazzureGame::Read(vdata+0x830,next.range)||!HazzureGame::Read(vdata+0x730,next.bullets)){Clear();return;}
    // The native inaccuracy method takes two optional float output pointers; XMM0 is the result.
    // No UpdateAccuracyPenalty call, writes, hook installation, or command-packet access here.
    using Inaccuracy=float(*)(uintptr_t,float*,float*);using Spread=float(*)(uintptr_t);
    __try{next.inaccuracy=reinterpret_cast<Inaccuracy>(getter)(next.weapon,nullptr,nullptr);
        next.spread=reinterpret_cast<Spread>(spreadGetter)(next.weapon);}
    __except(EXCEPTION_EXECUTE_HANDLER){Clear();return;}
    next.valid=Values(next);next.time=GetTickCount64();Publish(next);
#endif
}
}
