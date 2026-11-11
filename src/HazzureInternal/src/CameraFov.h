// Effective camera FOV: preserve native computation, then override only the live local camera.
#pragma once
#include "BasicTuning.h"
#include "DynamicSchema.h"
#include "WorldSession.h"
#include <atomic>
namespace HazzureCamera {
inline constexpr uintptr_t getterRva=0x88bed0;
inline constexpr unsigned char getterCode[]={0x40,0x53,0x48,0x83,0xec,0x50,0x48,0x8b,0xd9,0xe8,0xa2,0x83,0x42,0x00,0x48,0x85};
inline bool Supported(uintptr_t client){
    return HazzureVisibility::BuildMatches(client)&&HazzureVisibility::Prefix(client+getterRva,getterCode,sizeof(getterCode));
}
enum class Status {Waiting,Disabled,Applied,Scoped,Unavailable};
inline std::atomic<bool> installed{false};
inline std::atomic<Status> status{Status::Waiting};
inline std::atomic<float> delivered{},requested{};
inline std::atomic<bool> aspectRatioOverride{false};
inline std::atomic<float> aspectRatio{1.3333f};
inline std::atomic<uint64_t> lastDelivery{},appliedCalls{};
inline float Resolve(uintptr_t client,uintptr_t services,float native,bool allowed,
                     HazzureTuning::ClassCheck check=HazzureTuning::PlayerClass){
    AcquireSRWLockShared(&HazzureTuning::settingsLock);auto option=HazzureTuning::settings;
    ReleaseSRWLockShared(&HazzureTuning::settingsLock);
    if(!option.camera){status.store(Status::Disabled);return native;}
    if(!allowed||!std::isfinite(native)||native<=0||native>=180){status.store(Status::Waiting);return native;}
    if(!HazzureVisibility::BuildMatches(client)||!std::isfinite(option.cameraFov)||option.cameraFov<60||option.cameraFov>140){
        status.store(Status::Unavailable);return native;
    }
    auto pawn=HazzureGame::LocalPawn(client);HazzureGame::Pawn local{};
    uintptr_t camera{},list{},identity{};uint32_t raw{},flags{};uint8_t scoped=1;
    if(!services||!HazzureGame::ReadPawn(pawn,local)||!check(pawn)||
       !HazzureGame::Read(pawn+HazzureOffsets::Schema::m_pCameraServices,camera)||camera!=services||
       !HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)||
       !HazzureGame::Read(pawn+0x10,identity)||!identity||!HazzureGame::Read(identity+0x10,raw)||
       !HazzureGame::Read(identity+0x30,flags)||!HazzureTuning::Matches(list,HazzureGame::CanonicalHandle(raw,flags),pawn)){
        return native; // Foreign/retiring cameras cannot overwrite local-camera telemetry.
    }
    uintptr_t scopedOff = HazzureSchema::g_mIsScoped.offset ? HazzureSchema::g_mIsScoped.offset : 0x1ea0;
    if(!HazzureGame::Read(pawn+scopedOff,scoped)){status.store(Status::Unavailable);return native;}
    requested.store(option.cameraFov);delivered.store(scoped?native:option.cameraFov);
    lastDelivery.store(GetTickCount64(),std::memory_order_release);
    if(scoped){status.store(Status::Scoped);return native;}
    status.store(Status::Applied);appliedCalls.fetch_add(1);return option.cameraFov;
}
inline const char* Label(uint64_t now){
    if(!installed.load())return "Indisponivel: camera nao conectada";
    auto state=status.load();
    if(state==Status::Disabled)return "Desativado";
    if(state==Status::Unavailable)return "Indisponivel neste build";
    auto last=lastDelivery.load(std::memory_order_acquire);
    if(!last||now<last||now-last>1000||state==Status::Waiting)return "Aguardando camera local";
    return state==Status::Scoped?"Zoom nativo preservado":"Aplicado na camera";
}
}
