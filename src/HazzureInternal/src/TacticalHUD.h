// Build 14194 (0x6AEBC4DA): read-only C4/spectator snapshots; native damage runs on the frame callback.
#pragma once
#include "Visibility.h"
#include <array>
#include <cstdio>

namespace HazzureTactical {
using HazzureGame::Vec3;
// client.dll 0x6AEBC4DA: real gpGlobals used by C_PlantedC4 (0xCB58B5 -> 0x75E860, 0xD17B92).
// offsets_generated.h dwGlobalVars (0x22320E8) points to a .rdata vtable where +0x30 is 0.0f.
inline constexpr uintptr_t realGlobalVarsRva = 0x2235108;
inline bool ReadCurrentTime(uintptr_t client,float& current) {
    current=0.f;uintptr_t globals{};
    if(HazzureVisibility::BuildMatches(client)&&HazzureGame::Read(client+realGlobalVarsRva,globals)&&globals&&
       HazzureGame::Read(globals+0x30,current)&&std::isfinite(current)&&current>0.f)return true;
    if(HazzureGame::Read(client+HazzureOffsets::Globals::dwGlobalVars,globals)&&globals&&
       HazzureGame::Read(globals+0x30,current)&&std::isfinite(current)){
        if(current>0.f)return true;
        uintptr_t fallback{};float alt{};
        if(HazzureGame::Read(client+realGlobalVarsRva,fallback)&&fallback&&
           HazzureGame::Read(fallback+0x30,alt)&&std::isfinite(alt)&&alt>0.f){current=alt;return true;}
        return true;
    }
    return false;
}
inline uintptr_t ExactEntity(uintptr_t list,uint32_t handle) {
    if(!handle||handle==0xffffffffu)return 0;
    uintptr_t entity=HazzureGame::Entity(list,handle),identity{};uint32_t actual{};
    return entity&&HazzureGame::Read(entity+0x10,identity)&&identity&&
        HazzureGame::Read(identity+0x10,actual)&&actual==handle?entity:0;
}
inline uint32_t Handle(uintptr_t list,uintptr_t entity) {
    uintptr_t identity{};uint32_t handle{};
    return entity&&HazzureGame::Read(entity+0x10,identity)&&identity&&
        HazzureGame::Read(identity+0x10,handle)&&ExactEntity(list,handle)==entity?handle:0;
}
inline bool DesignerName(uintptr_t entity,const char* expected) {
    uintptr_t identity{},name{};char text[32]{};
    if(!entity||!HazzureGame::Read(entity+0x10,identity)||!identity)return false;
    if((HazzureGame::Read(identity+0x20,name)&&name&&HazzureGame::ReadBytes(name,text,16))||
       (HazzureGame::Read(identity+0x18,name)&&name&&HazzureGame::ReadBytes(name,text,16))){
        text[sizeof(text)-1]=0;
        if(!_stricmp(text,expected)||!_stricmp(text,"c_plantedc4"))return true;
    }
    return false;
}
inline bool Origin(uintptr_t entity,Vec3& out) {
    uintptr_t scene{};
    return entity&&HazzureGame::Read(entity+HazzureOffsets::Schema::m_pGameSceneNode,scene)&&scene&&
        HazzureGame::Read(scene+HazzureOffsets::Schema::m_vecAbsOrigin,out)&&HazzureGame::Finite(out);
}
struct Bomb {
    uintptr_t address{},list{};uint32_t handle{};
    bool isTicking{},isDefused{},beingDefused{};int site{};
    float remaining{},defuseLeft{},timerLength=40.f,defuseLength=10.f;
};
inline bool DefuseHasMargin(float remaining,float duration) {
    return std::isfinite(remaining)&&std::isfinite(duration)&&duration>=0.f&&duration+1.f/64.f<remaining;
}
inline bool ReadBombEntity(uintptr_t entity,uintptr_t list,float current,Bomb& out) {
    out={};
    uint8_t ticking{},defused{},exploded{},defusing{};float blow{},length{},end{},duration{};int site{};
    uint32_t handle=Handle(list,entity);
    if(!handle||!std::isfinite(current)||current<0.f||current>1e8f||
       !HazzureGame::Read(entity+0x1288,ticking)||ticking>1||
       !HazzureGame::Read(entity+0x12dc,defused)||defused>1||
       !HazzureGame::Read(entity+0x12bd,exploded)||exploded||(!ticking&&!defused)||
       !HazzureGame::Read(entity+0x12b8,blow)||!std::isfinite(blow)||
       !HazzureGame::Read(entity+0x12c0,length)||!std::isfinite(length)||length<5.f||length>120.f||
       blow-current>length+2.f||blow-current<-10.f||
       !HazzureGame::Read(entity+0x128c,site)||(site!=0&&site!=1)||
       !HazzureGame::Read(entity+0x12c4,defusing)||defusing>1)return false;
    Bomb value{};value.address=entity;value.list=list;value.handle=handle;value.site=site;
    value.isTicking=ticking!=0;value.isDefused=defused!=0;value.beingDefused=defusing&&!defused;
    value.timerLength=length;value.remaining=std::clamp(blow-current,0.f,length);
    if(value.beingDefused){
        if(!HazzureGame::Read(entity+0x12d4,duration)||!std::isfinite(duration)||duration<1.f||duration>15.f||
           !HazzureGame::Read(entity+0x12d8,end)||!std::isfinite(end)||end<current-.25f||end-current>duration+.25f)return false;
        value.defuseLength=duration;value.defuseLeft=std::clamp(end-current,0.f,duration);
    }
    out=value;return true;
}
struct BombLocator {
    uintptr_t lastList{};uint32_t cached{};uint64_t nextScan{};
    const char* lastLoc="none";bool lastHandleOk{},lastReadBomb{};
    float lastSimDelta{},lastCurrent{},lastSim{};uintptr_t lastGlobal{},lastCandidate{};
    bool Read(uintptr_t client,Bomb& out,uint64_t now=GetTickCount64()) {
        out={};lastLoc="none";lastHandleOk=false;lastReadBomb=false;
        lastSimDelta=0.f;lastCurrent=0.f;lastSim=0.f;lastGlobal=0;lastCandidate=0;
        uintptr_t list{},global{},bomb{};float current{};
        if(!client||!HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)||!list){
            lastList=0;cached=0;nextScan=0;return false;
        }
        if(lastList!=list){lastList=list;cached=0;nextScan=0;}
        HazzureGame::Read(client+HazzureOffsets::Globals::dwPlantedC4,global);
        lastGlobal=global;
        if(global){
            lastLoc="global";lastCandidate=global;
            if(Handle(list,global)){
                bomb=global;lastHandleOk=true;
                // client.dll+0xCB58D6 / 0xE93D99: C_PlantedC4 nodes link next via +0x1280.
                uint8_t exploded{};
                for(int step=0;step<8&&bomb&&HazzureGame::Read(bomb+0x12bd,exploded)&&exploded!=0;++step){
                    uintptr_t next{};
                    if(!HazzureGame::Read(bomb+0x1280,next)||!next||!Handle(list,next))break;
                    bomb=next;lastCandidate=bomb;
                }
            }
        }
        if(!bomb&&global){
            uintptr_t indirect{};
            if(HazzureGame::Read(global,indirect)&&indirect){
                lastLoc="indirect";lastCandidate=indirect;
                if(Handle(list,indirect)){bomb=indirect;lastHandleOk=true;}
            }
        }
        if(!bomb&&cached){
            lastLoc="cached";bomb=ExactEntity(list,cached);lastCandidate=bomb;
            if(bomb)lastHandleOk=true;else cached=0;
        }
        if(!bomb&&now>=nextScan){nextScan=now+500;
            for(uint32_t i=1;i<0x8000&&!bomb;++i){auto candidate=HazzureGame::Entity(list,i);
                if(DesignerName(candidate,"planted_c4")){
                    lastLoc="scan";lastCandidate=candidate;
                    if(Handle(list,candidate)){bomb=candidate;lastHandleOk=true;}
                }}
        }
        if(!bomb||!ReadCurrentTime(client,current))return false;
        lastCurrent=current;
        uintptr_t local=HazzureGame::LocalPawn(client);float simulation{};
        if(local&&HazzureGame::Read(local+0x3b8,simulation)&&std::isfinite(simulation)){
            if(std::fabs(current-simulation)>5.f){
                int hp{};float bombSim{};
                if((!HazzureGame::Read(local+HazzureOffsets::Schema::m_iHealth,hp)||hp<=0)&&
                   HazzureGame::Read(bomb+0x3b8,bombSim)&&std::isfinite(bombSim)&&bombSim>0.f)simulation=bombSim;
            }
            lastSim=simulation;lastSimDelta=std::fabs(current-simulation);
            if(std::isfinite(current)&&lastSimDelta>5.f)return false;
        }
        lastReadBomb=ReadBombEntity(bomb,list,current,out);
        if(!lastReadBomb)return false;
        cached=out.handle;return true;
    }
};

struct DamageRequest {
    uintptr_t list{},bomb{},pawn{};uint32_t bombHandle{},pawnHandle{};
    Vec3 origin{},angles{},bombOrigin{};
};
struct Damage {bool available{};uint8_t amount{};};
inline bool SameIdentity(const DamageRequest& a,const DamageRequest& b) {
    return a.list==b.list&&a.bomb==b.bomb&&a.pawn==b.pawn&&a.bombHandle==b.bombHandle&&a.pawnHandle==b.pawnHandle;
}
inline bool SamePose(const DamageRequest& a,const DamageRequest& b) {
    return HazzureVisibility::Near(a.origin,b.origin)&&HazzureVisibility::Near(a.bombOrigin,b.bombOrigin)&&
        std::fabs(HazzureGame::NormalizeAngle(a.angles.x-b.angles.x))<=1.f&&
        std::fabs(HazzureGame::NormalizeAngle(a.angles.y-b.angles.y))<=1.f;
}
inline bool MakeDamageRequest(const Bomb& bomb,uintptr_t pawn,DamageRequest& out) {
    out={};HazzureGame::Pawn local{};Vec3 position{};auto handle=Handle(bomb.list,pawn);
    if(!bomb.isTicking||bomb.isDefused||bomb.remaining<=0||!handle||
       !HazzureGame::ReadPawn(pawn,local)||!local.anglesValid||!Origin(bomb.address,position))return false;
    out={bomb.list,bomb.address,pawn,bomb.handle,handle,local.origin,local.angles,position};return true;
}
inline Damage NativeDamage(uintptr_t client,const DamageRequest& request) {
    using namespace HazzureGame;
    // ABI verified at the native health HUD call client+0xE93D77 (14194: 0x80DD30; 14192: 0x80D310):
    // bomb, pawn, uint8 damage output, Vector blast-direction output. No armor formula.
    constexpr unsigned char prefix[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x18,0x55,0x57,0x41,0x54,0x41,0x56,0x41,0x57,0x48,0x8d,0x6c,0x24,0xc0,0x48,0x81,0xec,0x40,0x01,0,0};
    uintptr_t fnRva=0;
    if(HazzureVisibility::Prefix(client+0x80dd30,prefix,sizeof(prefix)))fnRva=0x80dd30;
    else if(HazzureVisibility::Prefix(client+0x80d310,prefix,sizeof(prefix)))fnRva=0x80d310;
    uintptr_t list{};Pawn local{};Vec3 bombOrigin{};uint8_t ticking{},defused{},exploded{};
    if(!HazzureVisibility::BuildMatches(client)||!fnRva||
       !Read(client+HazzureOffsets::Globals::dwEntityList,list)||list!=request.list||
       ExactEntity(list,request.bombHandle)!=request.bomb||ExactEntity(list,request.pawnHandle)!=request.pawn||
       LocalPawn(client)!=request.pawn||!ReadPawn(request.pawn,local)||!local.anglesValid||
       !Origin(request.bomb,bombOrigin)||
       !Read(request.bomb+0x1288,ticking)||ticking!=1||!Read(request.bomb+0x12dc,defused)||defused||
       !Read(request.bomb+0x12bd,exploded)||exploded)return {};
    auto current=request;current.origin=local.origin;current.angles=local.angles;current.bombOrigin=bombOrigin;
    if(!SamePose(request,current))return {};
    uint8_t amount{};Vec3 direction{};
    using Query=bool(__fastcall*)(uintptr_t,uintptr_t,uint8_t*,Vec3*);
    __try{if(reinterpret_cast<Query>(client+fnRva)(request.bomb,request.pawn,&amount,&direction))return {true,amount};}
    __except(EXCEPTION_EXECUTE_HANDLER){}
    return {};
}
struct DamageQueue {
    SRWLOCK lock=SRWLOCK_INIT;DamageRequest requested{},evaluated{};Damage result{};
    uint64_t completed{},generation{};bool pending{},hasResult{};
    void Reset(){AcquireSRWLockExclusive(&lock);requested={};evaluated={};result={};completed=0;
        pending=hasResult=false;++generation;ReleaseSRWLockExclusive(&lock);}
    Damage Submit(const DamageRequest& request,uint64_t now) {
        AcquireSRWLockExclusive(&lock);Damage value{};
        if(!SameIdentity(requested,request)){hasResult=false;++generation;}
        requested=request;pending=true;
        if(hasResult&&now>=completed&&now-completed<=32&&SameIdentity(evaluated,request)&&SamePose(evaluated,request))value=result;
        ReleaseSRWLockExclusive(&lock);return value;
    }
    using Query=Damage(*)(uintptr_t,const DamageRequest&);
    void Process(uintptr_t client,Query query=NativeDamage,uint64_t now=GetTickCount64()) {
        AcquireSRWLockExclusive(&lock);auto request=requested;auto token=generation;bool work=pending;pending=false;
        ReleaseSRWLockExclusive(&lock);if(!work)return;
        auto value=query(client,request);
        AcquireSRWLockExclusive(&lock);
        if(token==generation&&SameIdentity(requested,request)){evaluated=request;result=value;completed=now;hasResult=true;}
        ReleaseSRWLockExclusive(&lock);
    }
};
inline DamageQueue damageQueue;

inline uintptr_t ObservedTarget(uintptr_t list,uintptr_t observer,uint8_t& mode) {
    mode=0;uintptr_t services{};uint32_t target{};
    if(!observer||!HazzureGame::Read(observer+0x1308,services)||!services||
       !HazzureGame::Read(services+0x48,mode)||(mode!=4&&mode!=5)||
       !HazzureGame::Read(services+0x4c,target))return 0;
    return ExactEntity(list,target);
}
inline uintptr_t ObserverPawn(uintptr_t list,uintptr_t controller) {
    uint32_t handle{};
    if(HazzureGame::Read(controller+HazzureOffsets::Schema::m_hObserverPawn,handle))
        if(auto pawn=ExactEntity(list,handle))return pawn;
    if(HazzureGame::Read(controller+0x6bc,handle))return ExactEntity(list,handle);
    return 0;
}
struct Spectator {char name[128]{};uint8_t mode{};};
struct Spectators {std::array<Spectator,64> entries{};size_t count{};};
inline Spectators CollectSpectators(uintptr_t list,uintptr_t localPawn,uintptr_t localController) {
    Spectators out{};if(!list)return out;
    uintptr_t camera=localPawn,localObserver{};HazzureGame::Pawn local{};uint8_t mode{};
    if(!HazzureGame::ReadPawn(localPawn,local)){
        if(!localController)return out;
        localObserver=ObserverPawn(list,localController);
        camera=ObservedTarget(list,localObserver,mode);
        if(!camera)return out;
    }
    for(uint32_t i=1;i<=64;++i){
        auto controller=HazzureGame::Entity(list,i);
        if(!controller||controller==localController)continue;
        auto pawn=ObserverPawn(list,controller);
        if(!pawn||pawn==localPawn||pawn==localObserver||ObservedTarget(list,pawn,mode)!=camera)continue;
        auto& entry=out.entries[out.count];entry.mode=mode;
        uintptr_t name{};
        if(HazzureGame::Read(controller+0x878,name)&&name)
            HazzureGame::ReadBytes(name,entry.name,sizeof(entry.name)-1);
        if(!entry.name[0])HazzureGame::ReadBytes(controller+HazzureOffsets::Schema::m_iszPlayerName,entry.name,sizeof(entry.name)-1);
        for(char& c:entry.name)if(c&&static_cast<unsigned char>(c)<32)c=' ';
        entry.name[sizeof(entry.name)-1]=0;
        if(!entry.name[0])sprintf_s(entry.name,"Jogador #%u",i);
        ++out.count;
    }
    return out;
}
} // namespace HazzureTactical
