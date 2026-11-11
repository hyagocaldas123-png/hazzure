// C++ / VisualRecoil.h / MSVC x64 C++17 / temporary render-stage punch suppression
#pragma once
#include "GameData.h"
#include <atomic>
namespace HazzureVisual {
using Vec3=HazzureGame::Vec3;
inline std::atomic<bool> enabled{false};
inline std::atomic<uint64_t> renderedFrames{0};
struct Field {uintptr_t address{};Vec3 value{};bool applied{};};
inline Field fields[4]{};
inline uintptr_t pawn{},camera{},punch{};
inline DWORD thread{};
inline SRWLOCK lock=SRWLOCK_INIT;
inline bool active{};
inline bool Write(uintptr_t at,const Vec3& value){
    if(at<0x10000)return false;
    __try{memcpy(reinterpret_cast<void*>(at),&value,sizeof(value));return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline void End(){
    AcquireSRWLockExclusive(&lock);
    if(active&&thread==GetCurrentThreadId()){
        uintptr_t actualCamera{},actualPunch{};
        bool identity=HazzureGame::Read(pawn+HazzureOffsets::Schema::m_pCameraServices,actualCamera)&&actualCamera==camera&&
            HazzureGame::Read(pawn+HazzureOffsets::Schema::m_pAimPunchServices,actualPunch)&&actualPunch==punch;
        for(auto& field:fields)if(field.applied&&identity){
            Vec3 current{};
            // Preserve a fresh engine update rather than restoring over it.
            if(HazzureGame::Read(field.address,current)&&current.x==0&&current.y==0&&current.z==0)Write(field.address,field.value);
        }
        active=false;for(auto& field:fields)field={};
    }
    ReleaseSRWLockExclusive(&lock);
}
inline bool Begin(uintptr_t local){
    if(!enabled.load()||!local)return false;
    HazzureGame::Pawn state{};uintptr_t cam{},svc{};
    if(!HazzureGame::ReadPawn(local,state)||
       !HazzureGame::Read(local+HazzureOffsets::Schema::m_pCameraServices,cam)||!cam||
       !HazzureGame::Read(local+HazzureOffsets::Schema::m_pAimPunchServices,svc)||!svc)return false;
    Field snapshot[4]={{cam+0x48},{svc+HazzureOffsets::Schema::m_predictableBaseAngle},
        {svc+0x5c},{svc+HazzureOffsets::Schema::m_unpredictableBaseAngle}};
    for(auto& field:snapshot)if(!HazzureGame::Read(field.address,field.value)||!HazzureGame::Finite(field.value,3600.f))return false;
    AcquireSRWLockExclusive(&lock);
    if(active){ReleaseSRWLockExclusive(&lock);return false;}
    pawn=local;camera=cam;punch=svc;thread=GetCurrentThreadId();active=true;
    for(unsigned i=0;i<4;i++){fields[i]=snapshot[i];fields[i].applied=Write(fields[i].address,{});}
    bool ok=fields[0].applied&&fields[1].applied&&fields[2].applied&&fields[3].applied;
    ReleaseSRWLockExclusive(&lock);if(!ok)End();else ++renderedFrames;return ok;
}
inline bool OriginalPunch(uintptr_t local,float& pitch,float& yaw){
    AcquireSRWLockShared(&lock);bool ok=active&&pawn==local;
    if(ok){const auto& p=fields[1].value;const auto& u=fields[3].value;
        pitch=std::fabs(p.x)>.001f||std::fabs(p.y)>.001f?p.x:u.x;
        yaw=std::fabs(p.x)>.001f||std::fabs(p.y)>.001f?p.y:u.y;}
    ReleaseSRWLockShared(&lock);return ok;
}
} // namespace HazzureVisual
