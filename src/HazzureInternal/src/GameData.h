// C++ / GameData.h / MSVC x64 C++17 / validated gameplay data and pure math
#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include "../../Compartilhado/offsets_generated.h"

// Climate schema override flag (referenced by Climate.h)
inline bool g_cfg_climateForceSchema = false;

namespace HazzureGame {
inline uint32_t CanonicalHandle(uint32_t raw,uint32_t flags){
    return raw==0xffffffffu?raw:(((raw>>15)-(flags&1u))<<15)|(raw&0x7fffu);
}
struct Vec3 { float x{}, y{}, z{}; };
struct Vec2 { float x{}, y{}; };
struct Matrix4 { float m[4][4]{}; };
inline bool Finite(const Vec3& v, float bound=1000000.f) {
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&
        std::fabs(v.x)<=bound&&std::fabs(v.y)<=bound&&std::fabs(v.z)<=bound;
}
inline bool ReadBytes(uintptr_t address, void* output, size_t size) {
    if(address<0x10000||!output||!size||size>1048576)return false;
    __try { memcpy(output,reinterpret_cast<const void*>(address),size); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline bool WriteBytes(uintptr_t address, const void* input, size_t size) {
    if(address<0x10000||!input||!size||size>1048576)return false;
    __try { memcpy(reinterpret_cast<void*>(address),input,size); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
template<class T> inline bool Write(uintptr_t address, const T& value) { return WriteBytes(address,&value,sizeof(value)); }
template<class T> inline bool Read(uintptr_t address,T& output) { return ReadBytes(address,&output,sizeof(output)); }
inline uintptr_t Entity(uintptr_t list,uint32_t handle) {
    if(!list||handle==0xFFFFFFFFu)return 0;
    uint32_t index=handle&0x7FFFu; uintptr_t chunk{},entity{};
    if(!index||!Read(list+0x10+(index>>9)*8,chunk)||!chunk)return 0;
    // CEntityIdentity entries are inline in the chunk. Their first member is the entity.
    if(!Read(chunk+(index&0x1FFu)*0x70,entity)||entity<0x10000)return 0;
    return entity;
}
struct Pawn {
    uintptr_t address{},scene{}; int health{}; uint8_t team{},life{};
    Vec3 origin{},velocity{},eye{},angles{}; bool velocityValid{},anglesValid{};
};
inline bool ReadPawn(uintptr_t address,Pawn& out,bool requireAlive=true) {
    using namespace HazzureOffsets::Schema;
    Pawn p{};p.address=address;
    if(!Read(address+m_iHealth,p.health)||p.health<0||p.health>10000||
       !Read(address+m_iTeamNum,p.team)||(p.team!=2&&p.team!=3)||
       !Read(address+m_lifeState,p.life)||p.life>2)return false;
    if(requireAlive&&(p.health==0||p.life!=0))return false;
    bool haveOrigin=Read(address+m_pGameSceneNode,p.scene)&&p.scene&&
        Read(p.scene+m_vecAbsOrigin,p.origin)&&Finite(p.origin);
    if(!haveOrigin&&(!Read(address+m_vOldOrigin,p.origin)||!Finite(p.origin)))return false;
    p.velocityValid=Read(address+m_vecVelocity,p.velocity)&&Finite(p.velocity,20000.f);
    if(!p.velocityValid)p.velocity={};
    Vec3 view{};
    if(!Read(address+m_vecViewOffset,view)||!Finite(view,128.f)||view.z<0||view.z>100.f)view={0,0,64};
    p.eye={p.origin.x+view.x,p.origin.y+view.y,p.origin.z+view.z};
    p.anglesValid=Read(address+m_angEyeAngles,p.angles)&&Finite(p.angles,36000.f)&&std::fabs(p.angles.x)<=90.f;
    out=p;return true;
}
inline uintptr_t LocalPawn(uintptr_t client) {
    uintptr_t candidate{},controller{},list{};Pawn p{};
    if(Read(client+HazzureOffsets::Globals::dwLocalPlayerPawn,candidate)&&ReadPawn(candidate,p,false))return candidate;
    if(!Read(client+HazzureOffsets::Globals::dwLocalPlayerController,controller)||!controller||
       !Read(client+HazzureOffsets::Globals::dwEntityList,list))return 0;
    uint32_t handle{};
    if(Read(controller+HazzureOffsets::Schema::m_hPlayerPawn,handle)) {
        candidate=Entity(list,handle); if(ReadPawn(candidate,p,false))return candidate;
    }
    return 0;
}
inline bool Project(const Vec3& p,const Matrix4& matrix,int width,int height,Vec2& result) {
    if(width<=0||height<=0||!Finite(p))return false;
    const auto& m=matrix.m;
    const float w=m[3][0]*p.x+m[3][1]*p.y+m[3][2]*p.z+m[3][3];
    if(!std::isfinite(w)||w<.001f)return false;
    const float x=(m[0][0]*p.x+m[0][1]*p.y+m[0][2]*p.z+m[0][3])/w;
    const float y=(m[1][0]*p.x+m[1][1]*p.y+m[1][2]*p.z+m[1][3])/w;
    result={(1.f+x)*width*.5f,(1.f-y)*height*.5f};
    return std::isfinite(result.x)&&std::isfinite(result.y);
}
inline float NormalizeAngle(float value) {
    if(!std::isfinite(value))return 0.f;
    return std::remainder(value,360.f);
}
inline bool AimAngles(const Vec3& eye,const Vec3& target,const Vec3& current,Vec2& delta) {
    if(!Finite(eye)||!Finite(target)||!Finite(current,36000.f))return false;
    Vec3 d{target.x-eye.x,target.y-eye.y,target.z-eye.z};
    float horizontal=std::hypot(d.x,d.y);
    if(horizontal<.001f&&std::fabs(d.z)<.001f)return false;
    constexpr float radToDeg=57.2957795131f;
    delta={NormalizeAngle(-std::atan2(d.z,horizontal)*radToDeg-current.x),
           NormalizeAngle(std::atan2(d.y,d.x)*radToDeg-current.y)};
    return true;
}
inline float FovRadius(float degrees,const Matrix4& matrix,int width) {
    const auto& row=matrix.m[0];
    float focal=std::sqrt(row[0]*row[0]+row[1]*row[1]+row[2]*row[2])*width*.5f;
    if(!std::isfinite(focal)||focal<=0)return 0;
    return std::tan(std::clamp(degrees,0.f,45.f)*.01745329252f)*focal;
}
inline float SmoothAlpha(float smooth,float seconds) {
    if(!std::isfinite(smooth)||!std::isfinite(seconds)||seconds<=0)return 0;
    smooth=std::clamp(smooth,1.f,30.f);
    return smooth==1.f?1.f:1.f-std::pow(1.f-1.f/smooth,std::clamp(seconds,0.f,.05f)*60.f);
}
struct MouseRemainder {
    float x{},y{};
    void Reset(){x=y=0;}
    bool Quantize(float dx,float dy,LONG& outX,LONG& outY) {
        if(!std::isfinite(dx)||!std::isfinite(dy)){Reset();outX=outY=0;return false;}
        x+=std::clamp(dx,-1000.f,1000.f); y+=std::clamp(dy,-1000.f,1000.f);
        outX=static_cast<LONG>(std::trunc(x));outY=static_cast<LONG>(std::trunc(y));
        x-=outX;y-=outY;return outX!=0||outY!=0;
    }
};
struct TriggerClock {
    uintptr_t target{}; uint64_t since{},nextShot{}; uint32_t delay{};
    void Reset(){target=0;since=nextShot=0;delay=0;}
    bool Ready(uintptr_t current,uint64_t now,uint32_t requestedDelay) {
        if(!current){Reset();return false;}
        if(target!=current){target=current;since=now;delay=requestedDelay;}
        return now>=nextShot&&now>=since&&now-since>=delay;
    }
    void Fired(uint64_t now,uint32_t nextDelay){since=now;delay=nextDelay;nextShot=now+45;}
};
// One input channel only: a pending press is released before another is issued.
// No synthetic SPACE events may feed back into the physical activation test.
struct JumpPulse {
    bool owned{},pressed{};uint64_t nextPress{};
    int Update(bool active,bool held,bool grounded,uint64_t now) {
        if(!active||!held){bool release=owned;owned=pressed=false;nextPress=0;return release?256:-1;}
        owned=true;
        if(!grounded||pressed){pressed=false;return 256;}
        if(now<nextPress)return 256;
        pressed=true;nextPress=now+16;return 65537;
    }
};
} // namespace HazzureGame
