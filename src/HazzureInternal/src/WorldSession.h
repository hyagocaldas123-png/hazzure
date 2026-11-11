#pragma once
#include "GameData.h"
#include <atomic>
namespace HazzureWorld {
struct Gate {
    uintptr_t list{},globals{};float clock{};uint64_t stableSince{};bool present{},ready{};
    bool Observe(uintptr_t entities,uintptr_t variables,bool valid,float simulation,uint64_t now){
        bool live=valid&&entities&&variables&&std::isfinite(simulation);
        bool changed=live!=present||(live&&(list!=entities||globals!=variables||simulation+.5f<clock))||now<stableSince;
        if(changed){ready=false;stableSince=now;}
        present=live;list=live?entities:0;globals=live?variables:0;clock=live?simulation:0;
        ready=live&&now>=stableSince&&now-stableSince>=250;return changed;
    }
};
inline Gate gate;
inline std::atomic<bool> ready{false};
inline std::atomic<uint64_t> epoch{};
inline bool Observe(uintptr_t client){
    uintptr_t list{},variables{};HazzureGame::Pawn pawn{};float time{};
    bool valid=HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)&&
        HazzureGame::Read(client+HazzureOffsets::Globals::dwGlobalVars,variables)&&
        HazzureGame::ReadPawn(HazzureGame::LocalPawn(client),pawn,false)&&HazzureGame::Read(pawn.address+0x3b8,time);
    bool changed=gate.Observe(list,variables,valid,time,GetTickCount64());
    ready.store(gate.ready,std::memory_order_release);if(changed)epoch.fetch_add(1);return changed;
}
inline bool MaterialNameMatches(const char* actual,const char* requested){
    if(!actual||!requested)return false;
    size_t a=strlen(actual),b=strlen(requested);
    if(a==b+2&&actual[a-2]=='_'&&actual[a-1]=='c')a-=2;
    return a==b&&_strnicmp(actual,requested,b)==0;
}
}
