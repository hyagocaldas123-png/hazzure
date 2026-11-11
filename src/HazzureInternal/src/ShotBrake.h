// Ordinary frame-based counter-strafe for trigger shots. No sub-tick packet modification.
#pragma once
#include "MovementTech.h"
#include "CombatAccuracy.h"
#include "RuntimeStability.h"
namespace HazzureBrake {
enum class Status {Idle,Waiting,Braking,Ready,Unsupported,Edge,TimedOut};
struct Request {uintptr_t pawn{},weapon{},target{};uint64_t time{};float limit=75;bool active{},edgeGuard=true;};
inline SRWLOCK lock=SRWLOCK_INIT;inline Request request;
inline std::atomic<Status> status{Status::Idle};
inline uint64_t revision{},evaluatedRevision{},completed{},workingRevision{};
inline uint64_t since{};inline Request current;inline bool owned{};inline uintptr_t ownerClient{};
inline HazzureStable::StopGate speedGate;
inline uintptr_t Address(int i){switch(i){case 0:return HazzureOffsets::Buttons::forward;case 1:return HazzureOffsets::Buttons::back;
    case 2:return HazzureOffsets::Buttons::left;default:return HazzureOffsets::Buttons::right;}}
inline constexpr const char* names[]={"forward","back","left","right"};
inline constexpr int keys[]={'W','S','A','D'};
inline bool Intent(const Request& a,const Request& b){return a.pawn==b.pawn&&a.weapon==b.weapon&&a.target==b.target&&a.limit==b.limit&&a.edgeGuard==b.edgeGuard;}
inline void Configure(const Request& value){AcquireSRWLockExclusive(&lock);
    if(!Intent(request,value)||request.active!=value.active){++revision;completed=0;status.store(value.active?Status::Waiting:Status::Idle);}
    request=value;ReleaseSRWLockExclusive(&lock);}
inline void Result(Status value,uint64_t generation,uint64_t now){AcquireSRWLockExclusive(&lock);
    if(generation==revision){evaluatedRevision=generation;completed=now;status.store(value);}
    ReleaseSRWLockExclusive(&lock);}
inline void Cancel(){Configure({});}
inline void Release(){
    if(!owned)return;
    bool complete=true;
    for(int i=0;i<4;++i){
        bool physical=(GetAsyncKeyState(keys[i])&0x8000)!=0;
        complete=HazzureMove::Button(ownerClient+Address(i),names[i],physical?65537:256)&&complete;
    }
    if(complete){owned=false;ownerClient=0;}
}
inline void Stop(){Cancel();Release();since=0;current={};speedGate.Reset();}
inline bool Ready(const Request& wanted,uint64_t now){
    AcquireSRWLockShared(&lock);auto live=request;
    bool ready=live.active&&Intent(wanted,live)&&now>=live.time&&now-live.time<=100&&completed&&
        evaluatedRevision==revision&&now>=completed&&now-completed<=32&&status.load()==Status::Ready;
    ReleaseSRWLockShared(&lock);return ready;
}
inline void Frame(uintptr_t client){
    AcquireSRWLockShared(&lock);auto wanted=request;auto generation=revision;ReleaseSRWLockShared(&lock);auto now=GetTickCount64();
    auto report=[&](Status value){Result(value,generation,now);};
    if(!wanted.active||HazzureMove::lineupBusy.load()||now<wanted.time||now-wanted.time>100){Release();since=0;current={};speedGate.Reset();report(Status::Idle);return;}
    if(workingRevision!=generation||!Intent(current,wanted)){Release();if(owned)return;workingRevision=generation;current=wanted;since=now;speedGate.Reset();}
    DWORD foreground{};GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
    HazzureGame::Pawn local{},enemy{};uint8_t move{};int flags{};uintptr_t list{},services{};uint32_t handle{};
    if(foreground!=GetCurrentProcessId()||!HazzureVisibility::BuildMatches(client)||HazzureGame::LocalPawn(client)!=wanted.pawn||
       !HazzureGame::ReadPawn(wanted.pawn,local)||!local.velocityValid||!local.anglesValid||
       !HazzureGame::ReadPawn(wanted.target,enemy)||enemy.team==local.team||
       !HazzureGame::Read(wanted.pawn+HazzureMove::moveTypeOffset,move)||move!=2||
       !HazzureGame::Read(wanted.pawn+HazzureOffsets::Schema::m_fFlags,flags)||(flags&1)==0||
       !HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)||
       !HazzureGame::Read(wanted.pawn+HazzureOffsets::Schema::m_pWeaponServices,services)||!services||
       !HazzureGame::Read(services+HazzureOffsets::Schema::m_hActiveWeapon,handle)||HazzureGame::Entity(list,handle)!=wanted.weapon){
        Release();report(Status::Unsupported);return;
    }
    float stopLimit=wanted.limit==0?HazzureStable::restSpeed:wanted.limit;
    auto correction=HazzureAccuracy::CounterStrafe(local.velocity,local.angles.y,stopLimit);
    bool stable=speedGate.Step(correction.speed,wanted.limit);
    if(stable)correction.brake=false;
    if(!correction.valid){Release();report(Status::Unsupported);return;}
    // Keep the directions suppressed until the shot or cancellation; physical input is
    // restored when ownership ends. Never claim an input vector changed velocity instantly.
    if(now<since||(!stable&&now-since>1000)){Release();report(Status::TimedOut);return;}
    if(correction.brake&&wanted.edgeGuard&&HazzureMove::ReadSupport(wanted.pawn,local.scene,local.origin,local.velocity,now)!=HazzureMove::Support::Ground){
        Release();report(Status::Edge);return;
    }
    if(!correction.brake&&!owned){report(stable?Status::Ready:Status::Waiting);return;}
    int states[4]={256,256,256,256};
    if(correction.brake){
        if(correction.forward>.15f)states[0]=65537;else if(correction.forward<-.15f)states[1]=65537;
        if(correction.side<-.15f)states[2]=65537;else if(correction.side>.15f)states[3]=65537;
    }
    owned=true;ownerClient=client;bool written=true;
    for(int i=0;i<4;++i)written=HazzureMove::Button(client+Address(i),names[i],states[i])&&written;
    if(!written){Release();report(Status::Unsupported);return;}
    report(correction.brake?Status::Braking:(stable?Status::Ready:Status::Waiting));
}
inline const char* Label(){switch(status.load()){
    case Status::Waiting:return "Aguardando frame";case Status::Braking:return "Freando";case Status::Ready:return "Velocidade dentro do limite";
    case Status::Unsupported:return "Indisponivel neste estado";case Status::Edge:return "Borda ou apoio nao confirmado";
    case Status::TimedOut:return "Tempo de frenagem esgotado";default:return "Inativo";}}
}
